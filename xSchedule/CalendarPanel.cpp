/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "CalendarPanel.h"

#include "DateField.h"
#include "Holidays.h"
#include "ModernUI.h"
#include "PlayList/PlayList.h"
#include "Schedule.h"
#include "ScheduleManager.h"

#include <wx/button.h>
#include <wx/checkbox.h>
#include <wx/dcbuffer.h>
#include <wx/dialog.h>
#include <wx/graphics.h>
#include <wx/bmpbuttn.h>
#include <wx/listctrl.h>
#include <wx/menu.h>
#include <wx/sizer.h>
#include <wx/simplebook.h>
#include <wx/stattext.h>

#include <algorithm>
#include <cmath>
#include <memory>

namespace {

struct Colors {
    wxColour bar;
    wxColour soft;
    wxColour text;
};

// the same eight colors as the web calendar, by the schedule's place in the list
Colors PaletteFor(int index)
{
    static const char* light[8][3] = {
        { "#2563c9", "#e6eefb", "#1f56b0" }, { "#0f766e", "#ddf2ef", "#0b5d57" }, { "#7c3aed", "#efe7fd", "#5b21b6" },
        { "#c2410c", "#fdebdd", "#9a3412" }, { "#be185d", "#fce7f1", "#9d174d" }, { "#334155", "#e5e8ec", "#1e293b" },
        { "#15803d", "#dcf2e3", "#14532d" }, { "#92400e", "#f6e9d9", "#78350f" }
    };
    static const char* dark[8][3] = {
        { "#6ea8fe", "#22344f", "#a9cbff" }, { "#4fd1c5", "#17332f", "#8de5db" }, { "#b794f6", "#2e2445", "#cdb5fa" },
        { "#fb923c", "#3b2616", "#fdba74" }, { "#f472b6", "#3a1e2c", "#f9a8d4" }, { "#94a3b8", "#2c323a", "#cbd5e1" },
        { "#4ade80", "#173322", "#86efac" }, { "#f59e0b", "#3a2c14", "#fcd34d" }
    };
    const auto& p = ModernUI::GetTheme().dark ? dark[index % 8] : light[index % 8];
    return { wxColour(p[0]), wxColour(p[1]), wxColour(p[2]) };
}

Colors PaletteFor(const ScheduleCalendar::Window& w)
{
    return PaletteFor(std::max(0, w.order - 1));
}

wxColour OnBar()
{
    return ModernUI::GetTheme().dark ? wxColour(16, 21, 28) : *wxWHITE;
}

wxColour Mix(const wxColour& a, const wxColour& b, double t)
{
    return wxColour((unsigned char)(a.Red() + (b.Red() - a.Red()) * t), (unsigned char)(a.Green() + (b.Green() - a.Green()) * t),
                    (unsigned char)(a.Blue() + (b.Blue() - a.Blue()) * t));
}

long DayNumber(const wxDateTime& d)
{
    return std::lround(d.GetDateOnly().GetJDN());
}

// the computer's first day of the week: Monday in much of the world, Sunday in the US
int FirstWeekDay()
{
    wxDateTime::WeekDay day;
    return wxDateTime::GetFirstWeekDay(&day) ? (int)day : 0;
}

wxString LongDate(const wxDateTime& d)
{
    return d.Format("%A, %B ") + wxString::Format("%d", d.GetDay());
}

wxString ShortDate(const wxDateTime& d)
{
    return d.Format("%a, %b ") + wxString::Format("%d", d.GetDay());
}

void Text(wxGraphicsContext* gc, wxDC& dc, const wxFont& font, const wxColour& colour, const wxString& text, double x, double y, double maxWidth)
{
    if (maxWidth <= 4) return;
    dc.SetFont(font);
    wxString shown = wxControl::Ellipsize(text, dc, wxELLIPSIZE_END, (int)maxWidth);
    gc->SetFont(font, colour);
    gc->DrawText(shown, x, y);
}

double TextHeight(wxGraphicsContext* gc, const wxFont& font)
{
    double w, h;
    gc->SetFont(font, *wxBLACK);
    gc->GetTextExtent("Ag", &w, &h);
    return h;
}

double TextWidth(wxGraphicsContext* gc, const wxFont& font, const wxString& text)
{
    double w, h;
    gc->SetFont(font, *wxBLACK);
    gc->GetTextExtent(text, &w, &h);
    return w;
}

void Fill(wxGraphicsContext* gc, const wxColour& fill, const wxPen& pen, double x, double y, double w, double h, double r)
{
    gc->SetBrush(fill.IsOk() ? wxBrush(fill) : *wxTRANSPARENT_BRUSH);
    gc->SetPen(pen);
    gc->DrawRoundedRectangle(x, y, w, h, r);
}

void Hatch(wxGraphicsContext* gc, const wxColour& colour, double x, double y, double w, double h, double step)
{
    gc->Clip(x, y, w, h);
    gc->SetPen(wxPen(colour, std::max(1, (int)(step / 2.5))));
    for (double sx = x - h; sx < x + w; sx += step) {
        gc->StrokeLine(sx, y + h, sx + h, y);
    }
    gc->ResetClip();
}

wxString StateText(const ScheduleCalendar::Detail& d, const ScheduleCalendar::Segment& g)
{
    if (g.state == ScheduleCalendar::State::Plays) return ScheduleCalendar::FormatRange(g.start, g.end);
    if (g.state == ScheduleCalendar::State::Done) return "Playlist finished";
    wxString by = g.by >= 0 ? wxString(d.windows[g.by].schedule->GetName()) : wxString();
    return (g.state == ScheduleCalendar::State::Hidden ? "Waiting: " : "Clash with ") + by;
}

} // namespace

// a month of nights, one cell each: click a night to choose it, double-click it to open it, double-click a schedule
// to edit it
class CalendarMonth : public wxWindow {
public:
    CalendarMonth(wxWindow* parent, std::function<void(const wxDateTime&)> select, std::function<void(const wxDateTime&)> open, CalendarPanel::EditSchedule edit,
                  std::function<void(const wxDateTime&)> menu) :
        wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE), _select(select), _open(open), _edit(edit), _menu(menu) {
        SetName("Calendar month");
        Bind(wxEVT_RIGHT_UP, [this](wxMouseEvent& e) {
            int c = CellAt(e.GetPosition());
            if (c >= 0) _menu(_nights[c].date);
        });
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(FromDIP(wxSize(500, 150)));
        Bind(wxEVT_PAINT, &CalendarMonth::OnPaint, this);
        Bind(wxEVT_LEFT_DOWN, &CalendarMonth::OnClick, this);
        Bind(wxEVT_LEFT_DCLICK, &CalendarMonth::OnDoubleClick, this);
        Bind(wxEVT_MOTION, &CalendarMonth::OnMotion, this);
    }

    void Set(const std::vector<ScheduleCalendar::Night>& nights, const wxDateTime& month, const wxDateTime& selected) {
        _nights = nights;
        _month = month;
        _selected = selected;
        Refresh();
    }

private:
    struct Hit {
        wxRect rect;
        int night = 0;
        Schedule* schedule = nullptr;
        wxString tip;
    };

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        const auto& t = ModernUI::GetTheme();
        dc.SetBackground(wxBrush(t.panel));
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc || _nights.empty()) return;
        _hits.clear();
        _cells.clear();

        wxFont base = GetFont();
        wxFont bold = base.Bold();
        wxFont smallFont = base.Scaled(0.88f);
        wxFont smallBold = smallFont.Bold();
        const wxSize size = GetClientSize();
        const double gap = FromDIP(4);
        const double headH = TextHeight(gc.get(), smallFont) + FromDIP(6);
        const int rows = (int)(_nights.size() / 7);
        const double cellW = (size.x - gap * 6) / 7.0;
        const double cellH = (size.y - headH - gap * (rows - 1)) / rows;
        const double chipH = TextHeight(gc.get(), smallFont) + FromDIP(3);
        const double chipGap = FromDIP(1);
        // one look for the whole month: chips when two fit in a night, otherwise rows of colored bars
        const double chipRoom = cellH - FromDIP(5) * 2 - TextHeight(gc.get(), bold) - FromDIP(3);
        const bool compact = chipRoom < chipH * 2 + chipGap;
        const double pad = FromDIP(5);

        for (int c = 0; c < 7; ++c) {
            wxString name = wxDateTime::GetWeekDayName((wxDateTime::WeekDay)((FirstWeekDay() + c) % 7), wxDateTime::Name_Abbr);
            Text(gc.get(), dc, smallBold, t.muted, name, c * (cellW + gap) + pad, 0, cellW - pad);
        }

        const long today = DayNumber(wxDateTime::Today());
        for (int i = 0; i < (int)_nights.size(); ++i) {
            const auto& n = _nights[i];
            const double x = (i % 7) * (cellW + gap);
            const double y = headH + (i / 7) * (cellH + gap);
            _cells.push_back(wxRect((int)x, (int)y, (int)cellW, (int)cellH));
            const bool inMonth = n.date.GetMonth() == _month.GetMonth();
            const bool selected = DayNumber(n.date) == DayNumber(_selected);
            wxColour bg = !n.holiday.empty() ? t.waitBg : inMonth ? t.panel : Mix(t.panel, t.toolbar, t.dark ? 0.6 : 1.0);
            Fill(gc.get(), bg, selected ? wxPen(t.accent, FromDIP(2)) : wxPen(t.line), x + 0.5, y + 0.5, cellW - 1, cellH - 1, FromDIP(4));

            double cy = y + pad - FromDIP(1);
            wxString num = wxString::Format("%d", n.date.GetDay());
            double numW = TextWidth(gc.get(), bold, num);
            double numH = TextHeight(gc.get(), bold);
            if (DayNumber(n.date) == today) {
                double d = numH + FromDIP(2);
                double w = std::max(d, numW + FromDIP(8));
                Fill(gc.get(), t.accent, *wxTRANSPARENT_PEN, x + pad - FromDIP(2), cy - FromDIP(1), w, d, d / 2);
                Text(gc.get(), dc, bold, t.onAccent, num, x + pad - FromDIP(2) + (w - numW) / 2, cy, w);
                numW = w - FromDIP(4);
            } else {
                Text(gc.get(), dc, bold, inMonth ? t.text : t.muted, num, x + pad, cy, cellW);
            }
            double right = x + cellW - pad;
            if (!ScheduleCalendar::ClashesOf(n.windows).empty()) {
                wxString c = "Clash";
                double cw = TextWidth(gc.get(), smallBold, c) + FromDIP(10);
                double ch = TextHeight(gc.get(), smallBold) + FromDIP(1);
                Fill(gc.get(), wxNullColour, wxPen(t.badText, FromDIP(1)), right - cw, cy, cw, ch, ch / 2);
                Text(gc.get(), dc, smallBold, t.badText, c, right - cw + FromDIP(5), cy + FromDIP(0.5), cw);
                right -= cw + FromDIP(4);
            }
            if (!n.holiday.empty()) {
                double hx = x + pad + numW + FromDIP(8);
                Text(gc.get(), dc, smallBold, t.waitText, n.holiday, hx, cy + (numH - TextHeight(gc.get(), smallBold)) / 2, right - hx);
            }

            cy += numH + FromDIP(3);
            int fits = std::max(0, (int)((y + cellH - pad - cy + chipGap) / (chipH + chipGap)));
            int need = (int)n.windows.size();
            if (compact) {
                // too short for chips: the night's schedules side by side in one row of their colors, details in tooltips
                if (need == 0) continue;
                const double barH = std::min<double>(FromDIP(8), std::max(3.0, y + cellH - cy - FromDIP(4)));
                const double segGap = FromDIP(2);
                const double segW = (cellW - pad * 2 - segGap * (need - 1)) / need;
                double sx = x + pad;
                for (const auto& win : n.windows) {
                    Colors col = PaletteFor(win);
                    if (win.Competes()) Fill(gc.get(), col.bar, *wxTRANSPARENT_PEN, sx, cy, segW, barH, barH / 2);
                    else Fill(gc.get(), wxNullColour, wxPen(t.muted, 1, wxPENSTYLE_SHORT_DASH), sx, cy, segW, barH, barH / 2);
                    wxString tip = win.schedule->GetName() + ", " + ScheduleCalendar::FormatRange(win.start, win.end) + (win.skipped ? "\nSkipped this night" : "") +
                                   (!win.enabled ? "\nInactive" : "") + "\nDouble-click to edit";
                    _hits.push_back({ wxRect((int)sx, (int)cy - 2, (int)segW, (int)barH + 4), i, win.schedule, tip });
                    sx += segW + segGap;
                }
                continue;
            }
            int shown = need <= fits ? need : std::max(0, fits - 1);
            for (int w = 0; w < shown; ++w) {
                const auto& win = n.windows[w];
                Colors col = PaletteFor(win);
                double cw = cellW - pad * 2;
                bool dashed = win.skipped || !win.enabled;
                if (dashed) {
                    Fill(gc.get(), wxNullColour, wxPen(t.muted, 1, wxPENSTYLE_SHORT_DASH), x + pad, cy, cw, chipH, FromDIP(3));
                } else {
                    Fill(gc.get(), col.soft, *wxTRANSPARENT_PEN, x + pad, cy, cw, chipH, FromDIP(3));
                    Fill(gc.get(), col.bar, *wxTRANSPARENT_PEN, x + pad, cy, FromDIP(3), chipH, 0);
                }
                wxString time = ScheduleCalendar::FormatMinutes(win.start);
                double tw = TextWidth(gc.get(), smallBold, time);
                double ty = cy + (chipH - TextHeight(gc.get(), smallFont)) / 2;
                wxColour tc = win.enabled ? col.text : t.muted;
                Text(gc.get(), dc, smallBold, tc, time, x + pad + FromDIP(6), ty, cw - FromDIP(8));
                wxFont nameFont = win.skipped ? smallFont.Strikethrough() : smallFont;
                Text(gc.get(), dc, nameFont, tc, win.schedule->GetName(), x + pad + FromDIP(10) + tw, ty, cw - tw - FromDIP(14));
                wxString tip = win.schedule->GetName() + ", " + ScheduleCalendar::FormatRange(win.start, win.end) + "\nPlaylist " + win.playlist->GetNameNoTime() +
                               ", priority " + wxString::Format("%d", win.schedule->GetPriority()) + (win.skipped ? "\nSkipped this night" : "") + (!win.enabled ? "\nInactive" : "") +
                               "\nDouble-click to edit";
                _hits.push_back({ wxRect((int)(x + pad), (int)cy, (int)cw, (int)chipH), i, win.schedule, tip });
                cy += chipH + chipGap;
            }
            if (shown < (int)n.windows.size()) {
                Text(gc.get(), dc, smallFont, t.muted, wxString::Format("+%d more", (int)n.windows.size() - shown), x + pad + FromDIP(4), cy, cellW - pad * 2);
            }
        }
    }

    int CellAt(const wxPoint& p) const {
        for (size_t i = 0; i < _cells.size(); ++i) {
            if (_cells[i].Contains(p)) return (int)i;
        }
        return -1;
    }

    void OnClick(wxMouseEvent& event) {
        SetFocus();
        int c = CellAt(event.GetPosition());
        if (c >= 0) _select(_nights[c].date);
    }

    void OnDoubleClick(wxMouseEvent& event) {
        for (const auto& h : _hits) {
            if (h.rect.Contains(event.GetPosition())) {
                _edit(h.schedule);
                return;
            }
        }
        int c = CellAt(event.GetPosition());
        if (c >= 0) _open(_nights[c].date);
    }

    void OnMotion(wxMouseEvent& event) {
        wxString tip;
        for (const auto& h : _hits) {
            if (h.rect.Contains(event.GetPosition())) tip = h.tip;
        }
        if (tip != _tip) {
            _tip = tip;
            if (tip.empty()) UnsetToolTip(); else SetToolTip(tip);
        }
    }

    std::function<void(const wxDateTime&)> _select;
    std::function<void(const wxDateTime&)> _open;
    CalendarPanel::EditSchedule _edit;
    std::function<void(const wxDateTime&)> _menu;
    std::vector<ScheduleCalendar::Night> _nights;
    wxDateTime _month;
    wxDateTime _selected;
    std::vector<Hit> _hits;
    std::vector<wxRect> _cells;
    wxString _tip;
};

// the chosen night as a strip: what is on the lights, then a row per schedule
class CalendarDay : public wxWindow {
public:
    CalendarDay(wxWindow* parent, CalendarPanel::EditSchedule edit, std::function<void(const wxDateTime&)> menu) :
        wxWindow(parent, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxWANTS_CHARS | wxFULL_REPAINT_ON_RESIZE), _edit(edit) {
        SetName("Calendar night");
        Bind(wxEVT_RIGHT_UP, [this, menu](wxMouseEvent&) {
            if (_has) menu(_detail.night.date);
        });
        Bind(wxEVT_LEFT_DOWN, [this](wxMouseEvent& e) {
            SetFocus();
            e.Skip();
        });
        SetBackgroundColour(ModernUI::GetTheme().panel);
        SetBackgroundStyle(wxBG_STYLE_PAINT);
        SetMinSize(FromDIP(wxSize(500, 150)));
        Bind(wxEVT_PAINT, &CalendarDay::OnPaint, this);
        Bind(wxEVT_LEFT_DCLICK, &CalendarDay::OnDoubleClick, this);
        Bind(wxEVT_MOTION, &CalendarDay::OnMotion, this);
    }

    void Set(const ScheduleCalendar::Detail& detail) {
        _detail = detail;
        _has = true;
        Refresh();
    }

    void Clear() {
        _detail = ScheduleCalendar::Detail();
        _has = false;
        Refresh();
    }

private:
    struct Hit {
        wxRect rect;
        Schedule* schedule = nullptr;
        wxString tip;
    };

    void OnPaint(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(this);
        const auto& t = ModernUI::GetTheme();
        dc.SetBackground(wxBrush(t.panel));
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc || !_has) return;
        _hits.clear();

        const auto& d = _detail;
        wxFont base = GetFont();
        wxFont bold = base.Bold();
        wxFont title = base.Scaled(1.15f).Bold();
        wxFont smallFont = base.Scaled(0.88f);
        wxFont smallBold = smallFont.Bold();
        const wxSize size = GetClientSize();
        const double labelW = FromDIP(210);
        const double x0 = labelW + FromDIP(10);
        const double trackW = std::max(10.0, size.x - x0 - FromDIP(4));
        const double span = std::max(1, d.axisEnd - d.axisStart);
        auto X = [&](int m) { return x0 + trackW * std::clamp((m - d.axisStart) / span, 0.0, 1.0); };

        double y = FromDIP(8);
        wxString heading = LongDate(d.night.date);
        Text(gc.get(), dc, title, t.text, heading, 0, y, size.x);
        double hx = TextWidth(gc.get(), title, heading) + FromDIP(10);
        double titleH = TextHeight(gc.get(), title);
        if (!d.night.holiday.empty()) {
            double pw = TextWidth(gc.get(), smallBold, d.night.holiday) + FromDIP(16);
            double ph = TextHeight(gc.get(), smallBold) + FromDIP(4);
            Fill(gc.get(), t.waitBg, *wxTRANSPARENT_PEN, hx, y + (titleH - ph) / 2, pw, ph, ph / 2);
            Text(gc.get(), dc, smallBold, t.waitText, d.night.holiday, hx + FromDIP(8), y + (titleH - ph) / 2 + FromDIP(2), pw);
            hx += pw + FromDIP(10);
        }
        wxString sun = "Sunset " + ScheduleCalendar::FormatMinutes(d.night.sunset) + wxString::FromUTF8("  \xC2\xB7  Dusk ") + ScheduleCalendar::FormatMinutes(d.night.dusk);
        Text(gc.get(), dc, base, t.muted, sun, hx, y + (titleH - TextHeight(gc.get(), base)) / 2, size.x - hx);
        wxString hint = "Double-click a bar to edit its schedule";
        double hintW = TextWidth(gc.get(), smallFont, hint);
        if (hx + TextWidth(gc.get(), base, sun) + hintW + FromDIP(30) < size.x) {
            Text(gc.get(), dc, smallFont, t.muted, hint, size.x - hintW - FromDIP(4), y + (titleH - TextHeight(gc.get(), smallFont)) / 2, hintW + 2);
        }
        y += titleH + FromDIP(12);

        if (d.windows.empty()) {
            Text(gc.get(), dc, base, t.muted, "Nothing is scheduled this night.", 0, y, size.x);
            return;
        }

        const double smallH = TextHeight(gc.get(), smallFont);
        for (int m = (d.axisStart + 59) / 60 * 60; m <= d.axisEnd; m += 60) {
            wxString label = ScheduleCalendar::FormatMinutes(m);
            if (label.EndsWith(":00 AM") || label.EndsWith(":00 PM")) label = label.Left(label.length() - 6) + label.Right(3);
            double lw = TextWidth(gc.get(), smallFont, label);
            double lx = std::clamp(X(m) - lw / 2, x0, x0 + trackW - lw);
            Text(gc.get(), dc, smallFont, t.muted, label, lx, y, lw + 2);
            gc->SetPen(wxPen(t.line));
            gc->StrokeLine(X(m), y + smallH + FromDIP(2), X(m), size.y);
        }
        y += smallH + FromDIP(6);

        const double barH = FromDIP(20);
        const double rowH = barH + FromDIP(2);
        Text(gc.get(), dc, bold, t.text, "On the lights", 0, y + (barH - TextHeight(gc.get(), bold)) / 2, labelW);
        Fill(gc.get(), t.track, *wxTRANSPARENT_PEN, x0, y, trackW, barH, FromDIP(4));
        for (const auto& g : d.onTheLights) {
            const auto& w = d.windows[g.by];
            Colors col = PaletteFor(w);
            double sx = X(g.start);
            double sw = X(g.end) - sx;
            Fill(gc.get(), col.bar, *wxTRANSPARENT_PEN, sx, y, sw, barH, 0);
            Text(gc.get(), dc, smallBold, OnBar(), w.schedule->GetName(), sx + FromDIP(6), y + (barH - smallH) / 2, sw - FromDIP(10));
            _hits.push_back({ wxRect((int)sx, (int)y, (int)sw, (int)barH), w.schedule, w.schedule->GetName() + ", " + ScheduleCalendar::FormatRange(g.start, g.end) });
        }
        const wxDateTime now = wxDateTime::Now();
        if (DayNumber(now) == DayNumber(d.night.date)) {
            int nm = now.GetHour() * 60 + now.GetMinute();
            if (nm >= d.axisStart && nm <= d.axisEnd) {
                gc->SetPen(wxPen(t.badText, FromDIP(2)));
                gc->StrokeLine(X(nm), y - FromDIP(3), X(nm), y + barH + FromDIP(3));
            }
        }
        y += rowH + FromDIP(6);

        const double legendH = TextHeight(gc.get(), smallFont) + FromDIP(10);
        int shownLanes = 0;
        for (const auto& lane : d.lanes) {
            if (y + rowH > size.y) break;
            const auto& w = d.windows[lane.window];
            Colors col = PaletteFor(w);
            ++shownLanes;
            double r = FromDIP(4);
            Fill(gc.get(), col.bar, *wxTRANSPARENT_PEN, FromDIP(2), y + barH / 2 - r, r * 2, r * 2, r);
            wxString from = DayNumber(w.night) != DayNumber(d.night.date) ? " (" + w.night.Format("%a") + ")" : "";
            wxString pri = wxString::Format("  P%d", w.schedule->GetPriority());
            double priW = TextWidth(gc.get(), smallFont, pri);
            Text(gc.get(), dc, smallBold, t.text, w.schedule->GetName() + from, FromDIP(14), y + (barH - smallH) / 2, labelW - FromDIP(14) - priW);
            Text(gc.get(), dc, smallFont, t.muted, pri, labelW - priW, y + (barH - smallH) / 2, priW + 2);
            wxString tipBase = w.schedule->GetName() + ", " + ScheduleCalendar::FormatRange(w.start, w.end) + "\nPlaylist " + w.playlist->GetNameNoTime() + ", priority " +
                               wxString::Format("%d", w.schedule->GetPriority());
            if (!w.Competes()) {
                double sx = X(w.start);
                double sw = X(w.end) - sx;
                Fill(gc.get(), wxNullColour, wxPen(t.muted, 1, wxPENSTYLE_SHORT_DASH), sx, y, sw, barH, FromDIP(4));
                wxString what = w.skipped ? "Skipped this night (skip date)" : "Inactive";
                Text(gc.get(), dc, smallFont, t.muted, what, sx + FromDIP(6), y + (barH - smallH) / 2, sw - FromDIP(10));
                _hits.push_back({ wxRect((int)sx, (int)y, (int)sw, (int)barH), w.schedule, tipBase + "\n" + what });
            }
            for (const auto& g : lane.segments) {
                double sx = X(g.start);
                double sw = X(g.end) - sx;
                wxString label = StateText(d, g);
                if (g.state == ScheduleCalendar::State::Plays) {
                    Fill(gc.get(), col.bar, *wxTRANSPARENT_PEN, sx, y, sw, barH, FromDIP(4));
                    Text(gc.get(), dc, smallBold, OnBar(), label, sx + FromDIP(6), y + (barH - smallH) / 2, sw - FromDIP(10));
                } else if (g.state == ScheduleCalendar::State::Done) {
                    Fill(gc.get(), wxNullColour, wxPen(t.muted, 1, wxPENSTYLE_SHORT_DASH), sx, y, sw, barH, FromDIP(4));
                    Text(gc.get(), dc, smallFont, t.muted, label, sx + FromDIP(6), y + (barH - smallH) / 2, sw - FromDIP(10));
                } else if (g.state == ScheduleCalendar::State::Hidden) {
                    Fill(gc.get(), wxNullColour, wxPen(col.soft), sx, y, sw, barH, FromDIP(4));
                    Hatch(gc.get(), col.soft, sx, y, sw, barH, FromDIP(9));
                    Text(gc.get(), dc, smallFont, col.text, label, sx + FromDIP(6), y + (barH - smallH) / 2, sw - FromDIP(10));
                } else {
                    Fill(gc.get(), col.soft, wxPen(t.badText, FromDIP(2)), sx + 1, y + 1, sw - 2, barH - 2, FromDIP(4));
                    Text(gc.get(), dc, smallBold, t.badText, label, sx + FromDIP(6), y + (barH - smallH) / 2, sw - FromDIP(10));
                }
                _hits.push_back({ wxRect((int)sx, (int)y, (int)sw, (int)barH), w.schedule, tipBase + "\n" + label });
            }
            _hits.push_back({ wxRect(0, (int)y, (int)labelW, (int)barH), w.schedule, tipBase });
            y += rowH + FromDIP(2);
        }
        if (shownLanes < (int)d.lanes.size()) {
            Text(gc.get(), dc, smallFont, t.muted, wxString::Format("+%d more schedules: make the window taller to see them", (int)d.lanes.size() - shownLanes), FromDIP(14), y, size.x);
        }

        // legend, when there is room under the lanes
        if (shownLanes < (int)d.lanes.size() || y + legendH > size.y) return;
        double ly = size.y - legendH + FromDIP(4);
        double lx = 0;
        auto key = [&](const wxString& text, std::function<void(double, double)> draw) {
            draw(lx, ly);
            lx += FromDIP(28);
            Text(gc.get(), dc, smallFont, t.muted, text, lx, ly, size.x - lx);
            lx += TextWidth(gc.get(), smallFont, text) + FromDIP(18);
        };
        const double kh = FromDIP(11);
        Colors grey{ t.muted, Mix(t.panel, t.muted, 0.45), t.muted };
        key("Plays", [&](double kx, double ky) { Fill(gc.get(), t.muted, *wxTRANSPARENT_PEN, kx, ky + FromDIP(2), FromDIP(22), kh, FromDIP(2)); });
        key("Waits for a higher priority", [&](double kx, double ky) { Hatch(gc.get(), grey.soft, kx, ky + FromDIP(2), FromDIP(22), kh, FromDIP(5)); });
        key("Same priority: a clash to fix", [&](double kx, double ky) { Fill(gc.get(), wxNullColour, wxPen(t.badText, FromDIP(2)), kx + 1, ky + FromDIP(3), FromDIP(20), kh - 2, FromDIP(2)); });
        key("Skipped, inactive or finished", [&](double kx, double ky) { Fill(gc.get(), wxNullColour, wxPen(t.muted, 1, wxPENSTYLE_SHORT_DASH), kx, ky + FromDIP(2), FromDIP(22), kh, FromDIP(2)); });
    }

    void OnDoubleClick(wxMouseEvent& event) {
        for (const auto& h : _hits) {
            if (h.rect.Contains(event.GetPosition())) {
                _edit(h.schedule);
                return;
            }
        }
    }

    void OnMotion(wxMouseEvent& event) {
        wxString tip;
        for (const auto& h : _hits) {
            if (h.rect.Contains(event.GetPosition())) tip = h.tip;
        }
        if (tip != _tip) {
            _tip = tip;
            if (tip.empty()) UnsetToolTip(); else SetToolTip(tip);
        }
    }

    CalendarPanel::EditSchedule _edit;
    ScheduleCalendar::Detail _detail;
    bool _has = false;
    std::vector<Hit> _hits;
    wxString _tip;
};

// nights with no show, clashes and skipped nights over a range, for checking a season before it starts
class SeasonCheckDialog : public wxDialog {
public:
    SeasonCheckDialog(wxWindow* parent, ScheduleManager* manager, const wxDateTime& from, const wxDateTime& to, CalendarPanel::EditSchedule edit) :
        wxDialog(parent, wxID_ANY, "Check the season", wxDefaultPosition, wxDefaultSize, wxDEFAULT_DIALOG_STYLE | wxRESIZE_BORDER), _manager(manager), _edit(edit) {
        auto* top = new wxBoxSizer(wxVERTICAL);
        auto* range = new wxBoxSizer(wxHORIZONTAL);
        range->Add(new wxStaticText(this, wxID_ANY, "From"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
        _from = new DateField(this, wxID_ANY, from);
        range->Add(_from, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));
        range->Add(new wxStaticText(this, wxID_ANY, "To"), 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(6));
        _to = new DateField(this, wxID_ANY, to);
        range->Add(_to, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(14));
        const wxColour background = GetBackgroundColour();
        auto* check = new wxBitmapButton(this, wxID_ANY, ModernUI::MakeTextButton(this, "Check", true, 30, &background), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | wxBORDER_NONE);
        check->SetLabel("Check");
        range->Add(check, 0, wxALIGN_CENTER_VERTICAL);
        top->Add(range, 0, wxALL, FromDIP(12));

        _strip = new wxWindow(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(600, 34)));
        _strip->SetBackgroundStyle(wxBG_STYLE_PAINT);
        _strip->Bind(wxEVT_PAINT, &SeasonCheckDialog::OnPaintStrip, this);
        top->Add(_strip, 0, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(12));
        _summary = new wxStaticText(this, wxID_ANY, "");
        top->Add(_summary, 0, wxALL, FromDIP(12));

        _list = new wxListView(this, wxID_ANY, wxDefaultPosition, FromDIP(wxSize(640, 300)), wxLC_REPORT | wxLC_SINGLE_SEL);
        _list->AppendColumn("Finding");
        _list->AppendColumn("When");
        _list->AppendColumn("Details");
        top->Add(_list, 1, wxLEFT | wxRIGHT | wxEXPAND, FromDIP(12));
        top->Add(new wxStaticText(this, wxID_ANY, "Double-click a clash or a skipped night to edit its schedule."), 0, wxALL, FromDIP(12));
        top->Add(CreateButtonSizer(wxCLOSE), 0, wxALL | wxEXPAND, FromDIP(8));
        SetSizerAndFit(top);
        SetEscapeId(wxID_CLOSE);

        check->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Check(); });
        _list->Bind(wxEVT_LIST_ITEM_ACTIVATED, [this](wxListEvent& e) {
            long i = e.GetIndex();
            if (i >= 0 && i < (long)_rowSchedules.size() && _rowSchedules[i] != nullptr) _edit(_rowSchedules[i]);
        });
        Check();
    }

private:
    void Check() {
        wxDateTime from = _from->GetDate();
        wxDateTime to = _to->GetDate();
        if (!from.IsValid() || !to.IsValid() || to < from) return;
        if ((to - from).GetDays() > 400) to = from + wxDateSpan::Days(400);
        _nights = ScheduleCalendar::Nights(*_manager, from, to, false);

        _list->DeleteAllItems();
        _rowSchedules.clear();
        const auto& t = ModernUI::GetTheme();
        int empty = 0;
        int clashes = 0;
        auto add = [this, &t](const wxString& finding, const wxString& when, const wxString& details, Schedule* s, bool bad) {
            long i = _list->InsertItem(_list->GetItemCount(), finding);
            _list->SetItem(i, 1, when);
            _list->SetItem(i, 2, details);
            if (bad) _list->SetItemTextColour(i, t.badText);
            _rowSchedules.push_back(s);
        };
        for (size_t i = 0; i < _nights.size();) {
            bool none = std::none_of(_nights[i].windows.begin(), _nights[i].windows.end(), [](const auto& w) { return w.Competes(); });
            if (!none) {
                ++i;
                continue;
            }
            size_t j = i;
            while (j + 1 < _nights.size() && std::none_of(_nights[j + 1].windows.begin(), _nights[j + 1].windows.end(), [](const auto& w) { return w.Competes(); })) ++j;
            empty += (int)(j - i + 1);
            wxString when = j == i ? ShortDate(_nights[i].date) : ShortDate(_nights[i].date) + " - " + ShortDate(_nights[j].date);
            add("No show", when, j == i ? "Nothing is scheduled." : wxString::Format("Nothing is scheduled for %d nights.", (int)(j - i + 1)), nullptr, true);
            i = j + 1;
        }
        for (const auto& n : _nights) {
            for (const auto& c : ScheduleCalendar::ClashesOf(n.windows)) {
                ++clashes;
                const auto& a = n.windows[c.a];
                const auto& b = n.windows[c.b];
                add("Clash", ShortDate(n.date) + ", " + ScheduleCalendar::FormatRange(c.start, c.end),
                    a.schedule->GetName() + " and " + b.schedule->GetName() + ", both priority " + wxString::Format("%d", a.schedule->GetPriority()),
                    b.schedule, true);
            }
        }
        for (const auto& n : _nights) {
            for (const auto& w : n.windows) {
                if (w.skipped) add("Skipped", ShortDate(n.date), w.schedule->GetName() + " skips this night (skip date).", w.schedule, false);
            }
        }
        if (_list->GetItemCount() == 0) add("All good", "", "A show every night, and no clashes.", nullptr, false);
        for (int c = 0; c < 3; ++c) _list->SetColumnWidth(c, wxLIST_AUTOSIZE);
        _summary->SetLabel(wxString::Format("%d nights checked: %d without a show, %d %s.", (int)_nights.size(), empty, clashes, clashes == 1 ? "clash" : "clashes"));
        _strip->Refresh();
        Layout();
    }

    void OnPaintStrip(wxPaintEvent&) {
        wxAutoBufferedPaintDC dc(_strip);
        const auto& t = ModernUI::GetTheme();
        dc.SetBackground(wxBrush(GetBackgroundColour()));
        dc.Clear();
        std::unique_ptr<wxGraphicsContext> gc(wxGraphicsContext::Create(dc));
        if (!gc || _nights.empty()) return;
        const wxSize size = _strip->GetClientSize();
        const double gap = _nights.size() > 120 ? 1 : FromDIP(2);
        const double w = std::max(2.0, (size.x - gap * (_nights.size() - 1)) / _nights.size());
        for (size_t i = 0; i < _nights.size(); ++i) {
            const auto& n = _nights[i];
            bool none = std::none_of(n.windows.begin(), n.windows.end(), [](const auto& win) { return win.Competes(); });
            bool clash = !ScheduleCalendar::ClashesOf(n.windows).empty();
            double x = i * (w + gap);
            if (none) Fill(gc.get(), t.badText, *wxTRANSPARENT_PEN, x, 0, w, size.y, FromDIP(2));
            else if (clash) Fill(gc.get(), wxNullColour, wxPen(t.badText, FromDIP(2)), x + 1, 1, w - 2, size.y - 2, FromDIP(2));
            else Fill(gc.get(), n.holiday.empty() ? t.accent : t.waitText, *wxTRANSPARENT_PEN, x, 0, w, size.y, FromDIP(2));
        }
    }

    ScheduleManager* _manager;
    CalendarPanel::EditSchedule _edit;
    DateField* _from = nullptr;
    DateField* _to = nullptr;
    wxWindow* _strip = nullptr;
    wxStaticText* _summary = nullptr;
    wxListView* _list = nullptr;
    std::vector<ScheduleCalendar::Night> _nights;
    std::vector<Schedule*> _rowSchedules;
};

CalendarPanel::CalendarPanel(wxWindow* parent, EditSchedule edit, AddSchedule add) :
    wxPanel(parent, wxID_ANY), _edit(edit), _add(add), _clock(this)
{
    _selected = wxDateTime::Today();
    _month = wxDateTime(1, _selected.GetMonth(), _selected.GetYear());
    const auto& t = ModernUI::GetTheme();
    SetBackgroundColour(t.panel);

    auto icon = [this, &t](ModernUI::Glyph glyph, const wxString& label, const wxString& tip) {
        auto* b = new wxBitmapButton(this, wxID_ANY, ModernUI::MakeIconButton(this, glyph, false, 30, &t.panel), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | wxBORDER_NONE);
        b->SetLabel(label); // read out by screen readers; only the picture shows
        b->SetToolTip(tip);
        return b;
    };
    auto text = [this, &t](const wxString& label, const wxString& tip) {
        auto* b = new wxBitmapButton(this, wxID_ANY, ModernUI::MakeTextButton(this, label, false, 30, &t.panel), wxDefaultPosition, wxDefaultSize, wxBU_AUTODRAW | wxBORDER_NONE);
        b->SetLabel(label);
        b->SetToolTip(tip);
        return b;
    };
    auto* top = new wxBoxSizer(wxVERTICAL);
    auto* bar = new wxBoxSizer(wxHORIZONTAL);
    _back = text("Back to month", "Back to the month (Esc)");
    _prev = icon(ModernUI::Glyph::ChevronLeft, "Previous", "Previous month (Page Up)");
    _next = icon(ModernUI::Glyph::ChevronRight, "Next", "Next month (Page Down)");
    _title = new wxStaticText(this, wxID_ANY, "");
    _title->SetFont(GetFont().Scaled(1.3f).Bold());
    _title->SetForegroundColour(t.text);
    _title->SetBackgroundColour(t.panel);
    auto* today = text("Today", "Go to today (Home)");
    _inactiveBox = new wxCheckBox(this, wxID_ANY, "Show inactive");
    _inactiveBox->SetForegroundColour(t.text);
    auto* season = text("Check season...", "Nights with no show, clashes and skipped nights");
    bar->Add(_back, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    bar->Add(_prev, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(4));
    bar->Add(_next, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(10));
    bar->Add(_title, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    bar->Add(today, 0, wxALIGN_CENTER_VERTICAL);
    bar->AddStretchSpacer();
    bar->Add(_inactiveBox, 0, wxALIGN_CENTER_VERTICAL | wxRIGHT, FromDIP(12));
    bar->Add(season, 0, wxALIGN_CENTER_VERTICAL);
    top->Add(bar, 0, wxEXPAND | wxALL, FromDIP(8));

    // the month uses the whole tab; a night opens in its place
    _book = new wxSimplebook(this, wxID_ANY);
    auto menu = [this](const wxDateTime& d) { NightMenu(d); };
    _monthView = new CalendarMonth(_book, [this](const wxDateTime& d) { Select(d); }, [this](const wxDateTime& d) { Select(d); ShowNight(true); }, _edit, menu);
    _dayView = new CalendarDay(_book, _edit, menu);
    _book->AddPage(_monthView, "Month");
    _book->AddPage(_dayView, "Night");
    top->Add(_book, 1, wxEXPAND | wxLEFT | wxRIGHT | wxBOTTOM, FromDIP(8));
    SetSizer(top);

    _back->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { ShowNight(false); });
    _prev->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Step(-1); });
    _next->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Step(1); });
    today->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) { Select(wxDateTime::Today()); });
    for (wxWindow* view : { (wxWindow*)_monthView, (wxWindow*)_dayView }) {
        view->Bind(wxEVT_KEY_DOWN, [this](wxKeyEvent& e) {
            if (!Keyboard(e)) e.Skip();
        });
    }
    ShowNight(false);
    _inactiveBox->Bind(wxEVT_CHECKBOX, [this](wxCommandEvent& e) { _inactive = e.IsChecked(); RefreshFrom(_manager); });
    season->Bind(wxEVT_BUTTON, [this](wxCommandEvent&) {
        if (_manager == nullptr) return;
        // the season: from the first night with a show to the last, looking a year ahead
        wxDateTime from = _month;
        wxDateTime to = _month + wxDateSpan::Months(2) - wxDateSpan::Day();
        auto year = ScheduleCalendar::Nights(*_manager, wxDateTime::Today(), wxDateTime::Today() + wxDateSpan::Year(), false);
        auto plays = [](const ScheduleCalendar::Night& n) { return std::any_of(n.windows.begin(), n.windows.end(), [](const auto& w) { return w.Competes(); }); };
        auto first = std::find_if(year.begin(), year.end(), plays);
        auto last = std::find_if(year.rbegin(), year.rend(), plays);
        if (first != year.end()) {
            from = first->date;
            to = last->date;
        }
        SeasonCheckDialog dlg(this, _manager, from, to, _edit);
        dlg.ShowModal();
    });
    Bind(wxEVT_TIMER, &CalendarPanel::OnClock, this);
    _clock.Start(30000);
}

void CalendarPanel::Select(const wxDateTime& date)
{
    _selected = date.GetDateOnly();
    _month = wxDateTime(1, _selected.GetMonth(), _selected.GetYear());
    RefreshFrom(_manager);
}

void CalendarPanel::ShowNight(bool night)
{
    _night = night;
    _book->ChangeSelection(night ? 1 : 0);
    _back->Show(night);
    _prev->SetToolTip(night ? "Previous night (Left)" : "Previous month (Page Up)");
    _next->SetToolTip(night ? "Next night (Right)" : "Next month (Page Down)");
    Layout();
    Refresh(); // the hidden Back button's area is left unpainted otherwise
    if (IsShownOnScreen()) (night ? (wxWindow*)_dayView : (wxWindow*)_monthView)->SetFocus();
}

// arrows move between nights, Enter opens one and Esc goes back, Page Up and Down change month, Home is today
bool CalendarPanel::Keyboard(wxKeyEvent& event)
{
    switch (event.GetKeyCode()) {
    case WXK_TAB:
        // the views take every key, so Tab has to move on by hand
        static_cast<wxWindow*>(event.GetEventObject())->Navigate(event.ShiftDown() ? wxNavigationKeyEvent::IsBackward : wxNavigationKeyEvent::IsForward);
        return true;
    case WXK_HOME:
        Select(wxDateTime::Today());
        return true;
    case WXK_PAGEUP:
        if (_night) Select(_selected - wxDateSpan::Month()); else Step(-1);
        return true;
    case WXK_PAGEDOWN:
        if (_night) Select(_selected + wxDateSpan::Month()); else Step(1);
        return true;
    case WXK_LEFT:
        Select(_selected - wxDateSpan::Day());
        return true;
    case WXK_RIGHT:
        Select(_selected + wxDateSpan::Day());
        return true;
    case WXK_UP:
        if (_night) return false;
        Select(_selected - wxDateSpan::Week());
        return true;
    case WXK_DOWN:
        if (_night) return false;
        Select(_selected + wxDateSpan::Week());
        return true;
    case WXK_RETURN:
    case WXK_NUMPAD_ENTER:
    case WXK_SPACE:
        if (!_night) ShowNight(true);
        return true;
    case WXK_ESCAPE:
    case WXK_BACK:
        if (!_night) return false;
        ShowNight(false);
        return true;
    }
    return false;
}

void CalendarPanel::NightMenu(const wxDateTime& night)
{
    if (_manager == nullptr) return;
    wxMenu menu;
    menu.Append(1, "Open " + night.Format("%A, %B ") + wxString::Format("%d", night.GetDay()));
    menu.AppendSeparator();
    wxMenu* add = new wxMenu();
    std::vector<PlayList*> playlists;
    for (const auto& pl : _manager->GetPlayLists()) {
        playlists.push_back(pl);
        add->Append(100 + (int)playlists.size() - 1, pl->GetNameNoTime() + "...");
    }
    auto* item = menu.AppendSubMenu(add, "Add a schedule starting this night for");
    if (playlists.empty()) item->Enable(false);
    int chosen = GetPopupMenuSelectionFromUser(menu);
    if (chosen == 1) {
        Select(night);
        ShowNight(true);
    } else if (chosen >= 100 && chosen - 100 < (int)playlists.size()) {
        _add(playlists[chosen - 100], night);
    }
}

void CalendarPanel::Step(int direction)
{
    if (_night) {
        Select(_selected + wxDateSpan::Days(direction));
    } else {
        _month += wxDateSpan::Months(direction);
        RefreshFrom(_manager);
    }
}

void CalendarPanel::Forget()
{
    _manager = nullptr;
    _signature.clear();
    _monthView->Set({}, _month, _selected);
    _dayView->Clear();
}

std::string CalendarPanel::Signature() const
{
    std::string sig = _month.FormatISODate().ToStdString() + _selected.FormatISODate().ToStdString() + wxDateTime::Today().FormatISODate().ToStdString() +
                      (_inactive ? "i" : "") + "|" + Holidays::GetCountry() + "|";
    double latitude;
    double longitude;
    Schedule::GetLocation(latitude, longitude);
    sig += std::to_string(latitude) + "," + std::to_string(longitude) + "|";
    for (const auto& pl : _manager->GetPlayLists()) {
        sig += std::to_string(pl->GetId()) + ":";
        for (const auto& s : pl->GetSchedules()) {
            sig += std::to_string(s->GetId()) + "." + std::to_string(s->GetChangeCount()) + (s->GetEnabled() ? "e" : "d") + ",";
        }
        sig += ";";
    }
    return sig;
}

void CalendarPanel::RefreshFrom(ScheduleManager* manager, bool force)
{
    _manager = manager;
    if (_manager == nullptr) return;
    std::string sig = Signature();
    if (!force && sig == _signature) return;
    _signature = sig;
    Recompute();
}

void CalendarPanel::Recompute()
{
    _title->SetLabel(_month.Format("%B %Y"));
    const int fw = FirstWeekDay();
    wxDateTime first = _month;
    wxDateTime start = first - wxDateSpan::Days(((int)first.GetWeekDay() - fw + 7) % 7);
    wxDateTime last = _month + wxDateSpan::Month() - wxDateSpan::Day();
    wxDateTime end = last + wxDateSpan::Days(6 - ((int)last.GetWeekDay() - fw + 7) % 7);
    _monthView->Set(ScheduleCalendar::Nights(*_manager, start, end, _inactive), _month, _selected);
    _dayView->Set(ScheduleCalendar::DetailOf(*_manager, _selected, _inactive));
    Layout();
}

void CalendarPanel::OnClock(wxTimerEvent&)
{
    // the "now" line moves with the clock, and at midnight today moves on
    if (IsShownOnScreen()) {
        RefreshFrom(_manager);
        _dayView->Refresh();
    }
}
