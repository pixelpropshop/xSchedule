/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

// Unit tests for the schedule logic (Schedule, Holidays). Builds as a small
// console program; see build_tests_msw.cmd and Makefile in this folder.

#include <wx/init.h>
#include <wx/xml/xml.h>

#include <cstdio>
#include <initializer_list>
#include <string>
#include <utility>

#include "../Holidays.h"
#include "../Schedule.h"

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_failures;                                                 \
            std::printf("FAIL %s:%d  %s\n", __FILE__, __LINE__, #cond);   \
        }                                                                 \
    } while (0)

wxDateTime D(int y, int m, int d, int h = 0, int mi = 0) {
    return wxDateTime((wxDateTime::wxDateTime_t)d, (wxDateTime::Month)(m - 1), y, h, mi);
}

using Attrs = std::initializer_list<std::pair<const char*, const char*>>;

Schedule Make(Attrs attrs) {
    wxXmlNode node(nullptr, wxXML_ELEMENT_NODE, "Schedule");
    node.AddAttribute("Enabled", "TRUE");
    for (const auto& a : attrs) {
        node.AddAttribute(a.first, a.second);
    }
    return Schedule(&node);
}

bool HasAttribute(wxXmlNode* node, const char* name) {
    return node->HasAttribute(name);
}

void TestHolidays() {
    CHECK(Holidays::DateFor("thanksgiving_us", 2026).IsSameDate(D(2026, 11, 26)));
    CHECK(Holidays::DateFor("thanksgiving_us", 2027).IsSameDate(D(2027, 11, 25)));
    CHECK(Holidays::DateFor("thanksgiving_us", 2028).IsSameDate(D(2028, 11, 23)));
    CHECK(Holidays::DateFor("thanksgiving_ca", 2026).IsSameDate(D(2026, 10, 12)));
    CHECK(Holidays::DateFor("easter", 2025).IsSameDate(D(2025, 4, 20)));
    CHECK(Holidays::DateFor("easter", 2026).IsSameDate(D(2026, 4, 5)));
    CHECK(Holidays::DateFor("easter", 2027).IsSameDate(D(2027, 3, 28)));
    CHECK(Holidays::DateFor("mothersday_us", 2026).IsSameDate(D(2026, 5, 10)));
    CHECK(Holidays::DateFor("memorialday_us", 2026).IsSameDate(D(2026, 5, 25)));
    CHECK(Holidays::DateFor("fathersday_us", 2026).IsSameDate(D(2026, 6, 21)));
    CHECK(Holidays::DateFor("laborday_us", 2026).IsSameDate(D(2026, 9, 7)));
    CHECK(Holidays::DateFor("halloween", 2026).IsSameDate(D(2026, 10, 31)));
    CHECK(Holidays::DateFor("christmas", 2026).IsSameDate(D(2026, 12, 25)));
    CHECK(!Holidays::DateFor("not_a_holiday", 2026).IsValid());
    for (const auto& h : Holidays::All()) {
        CHECK(Holidays::DateFor(h.id, 2026).IsValid());
    }
}

// the cases from the original Schedule::Test, through the public interface
void TestOriginalBehavior() {
    {
        Schedule s = Make({ { "StartDate", "2016-12-05" }, { "EndDate", "2017-01-12" }, { "StartTime", "22:00" }, { "EndTime", "02:00" }, { "DOW", "MonTue" } });
        CHECK(!s.IsActiveAt(D(2016, 12, 5, 21, 0)));
        CHECK(s.IsActiveAt(D(2016, 12, 5, 22, 0)));
        CHECK(s.IsActiveAt(D(2016, 12, 6, 1, 0)));
        CHECK(!s.IsActiveAt(D(2016, 12, 6, 3, 0)));
        CHECK(!s.IsActiveAt(D(2017, 1, 16, 23, 0)));
        CHECK(!s.IsActiveAt(D(2017, 1, 11, 22, 30))); // a Wednesday
    }
    {
        Schedule s = Make({ { "StartDate", "2017-12-26" }, { "EndDate", "2018-01-01" }, { "StartTime", "19:00" }, { "EndTime", "22:00" } });
        CHECK(s.IsActiveAt(D(2018, 1, 1, 19, 0)));
        CHECK(!s.IsActiveAt(D(2019, 1, 1, 19, 0)));
        CHECK(!s.IsActiveAt(D(2018, 1, 2, 19, 0)));
        CHECK(s.IsActiveAt(D(2017, 12, 26, 19, 0)));
        CHECK(!s.IsActiveAt(D(2018, 12, 26, 19, 0)));
        CHECK(!s.IsActiveAt(D(2017, 12, 25, 19, 0)));
    }
    {
        Schedule s = Make({ { "StartDate", "2017-12-26" }, { "EndDate", "2018-01-01" }, { "StartTime", "19:00" }, { "EndTime", "19:00" } });
        CHECK(!s.IsActiveAt(D(2017, 12, 26, 16, 0)));
        CHECK(s.IsActiveAt(D(2017, 12, 26, 19, 0)));
        CHECK(!s.IsActiveAt(D(2018, 12, 26, 19, 0)));
        CHECK(s.IsActiveAt(D(2017, 12, 27, 1, 0)));
        CHECK(s.IsActiveAt(D(2018, 1, 1, 1, 0)));
        CHECK(!s.IsActiveAt(D(2019, 1, 1, 1, 0)));
        CHECK(!s.IsActiveAt(D(2018, 1, 1, 20, 0)));
    }
    {
        Schedule s = Make({ { "StartDate", "2017-12-26" }, { "EndDate", "2018-01-01" }, { "StartTime", "19:00" }, { "EndTime", "19:00" }, { "EveryYear", "TRUE" } });
        CHECK(s.IsActiveAt(D(2019, 1, 1, 1, 0)));
        CHECK(s.IsActiveAt(D(2019, 12, 26, 19, 0)));
        CHECK(!s.IsActiveAt(D(2018, 1, 2, 19, 0)));
    }
    {
        Schedule s = Make({ { "StartDate", "2026-01-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "19:00" }, { "EndTime", "22:00" } });
        s.SetEnabled(false);
        CHECK(!s.IsActiveAt(D(2026, 6, 1, 20, 0)));
        CHECK(s.GetUpcomingWindows(D(2026, 6, 1), 3).empty());
    }
}

void TestHolidaySeason() {
    // the day after Thanksgiving through Epiphany, every year
    Schedule s = Make({ { "StartDate", "2026-01-01" }, { "EndDate", "2026-01-06" }, { "EveryYear", "TRUE" }, { "StartTime", "17:00" }, { "EndTime", "22:00" },
                        { "StartHoliday", "thanksgiving_us" }, { "StartHolidayOffset", "1" }, { "EndHoliday", "epiphany" } });
    CHECK(s.GetStartHoliday() == "thanksgiving_us");
    CHECK(s.GetStartHolidayOffset() == 1);
    CHECK(s.GetEndHoliday() == "epiphany");
    CHECK(!s.IsActiveAt(D(2026, 11, 26, 18, 0)));
    CHECK(s.IsActiveAt(D(2026, 11, 27, 18, 0)));
    CHECK(s.IsActiveAt(D(2026, 12, 31, 18, 0)));
    CHECK(s.IsActiveAt(D(2027, 1, 6, 18, 0)));
    CHECK(!s.IsActiveAt(D(2027, 1, 7, 18, 0)));
    CHECK(!s.IsActiveAt(D(2027, 11, 25, 18, 0))); // Thanksgiving 2027 itself
    CHECK(s.IsActiveAt(D(2027, 11, 26, 18, 0)));
    CHECK(!s.IsActiveAt(D(2026, 11, 27, 23, 0))); // after the end time
    CHECK(s.GetEffectiveStartDate().IsSameDate(D(2026, 11, 27)));

    // more than a week ahead is found
    auto windows = s.GetUpcomingWindows(D(2026, 10, 6, 12, 0), 3);
    CHECK(windows.size() == 3);
    if (windows.size() == 3) {
        CHECK(windows[0].first == D(2026, 11, 27, 17, 0));
        CHECK(windows[0].second == D(2026, 11, 27, 22, 0));
        CHECK(windows[1].first == D(2026, 11, 28, 17, 0));
        CHECK(windows[2].first == D(2026, 11, 29, 17, 0));
    }
}

void TestSkipDates() {
    {
        Schedule s = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-31" }, { "EveryYear", "TRUE" }, { "StartTime", "17:00" }, { "EndTime", "22:00" },
                            { "SkipDates", "2026-12-24" } });
        CHECK(s.IsActiveAt(D(2026, 12, 23, 18, 0)));
        CHECK(!s.IsActiveAt(D(2026, 12, 24, 18, 0)));
        CHECK(!s.IsActiveAt(D(2027, 12, 24, 18, 0))); // every year: month and day only
        CHECK(s.IsActiveAt(D(2026, 12, 25, 18, 0)));
        auto w = s.GetUpcomingWindows(D(2026, 12, 23, 18, 0), 1);
        CHECK(w.size() == 1 && w[0].first == D(2026, 12, 25, 17, 0));
    }
    {
        Schedule s = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2027-12-31" }, { "StartTime", "17:00" }, { "EndTime", "22:00" }, { "SkipDates", "2026-12-24" } });
        CHECK(!s.IsActiveAt(D(2026, 12, 24, 18, 0)));
        CHECK(s.IsActiveAt(D(2027, 12, 24, 18, 0))); // one off: only that year
    }

    std::vector<wxDateTime> dates;
    CHECK(Schedule::ParseSkipDates("2026-12-24, 2026-12-31", dates));
    CHECK(dates.size() == 2);
    dates.clear();
    CHECK(!Schedule::ParseSkipDates("2026-13-01", dates));
    dates.clear();
    CHECK(!Schedule::ParseSkipDates("Dec 24", dates));
    dates.clear();
    CHECK(Schedule::ParseSkipDates("", dates) && dates.empty());

    Schedule s = Make({});
    s.SetSkipDates({ D(2026, 12, 31), D(2026, 12, 24), D(2026, 12, 24) });
    CHECK(s.GetSkipDatesAsString() == "2026-12-24, 2026-12-31");
}

void TestWindows() {
    Schedule overnight = Make({ { "StartDate", "2026-01-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "22:00" }, { "EndTime", "02:00" }, { "DOW", "FriSat" } });
    auto w = overnight.GetUpcomingWindows(D(2026, 10, 6, 12, 0), 2); // a Tuesday
    CHECK(w.size() == 2);
    if (w.size() == 2) {
        CHECK(w[0].first == D(2026, 10, 9, 22, 0));
        CHECK(w[0].second == D(2026, 10, 10, 2, 0));
        CHECK(w[1].first == D(2026, 10, 10, 22, 0));
    }

    Schedule allDay = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-10" }, { "StartTime", "00:00" }, { "EndTime", "00:00" } });
    auto a = allDay.GetUpcomingWindows(D(2026, 11, 1), 5);
    CHECK(a.size() == 1);
    if (a.size() == 1) {
        CHECK(a[0].first == D(2026, 12, 1, 0, 0));
        CHECK(a[0].second == D(2026, 12, 10, 0, 0));
    }

    // a bad nth day in a file must not divide by zero
    Schedule nth = Make({ { "StartDate", "2026-01-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "17:00" }, { "EndTime", "22:00" }, { "NthDay", "0" } });
    CHECK(nth.IsActiveAt(D(2026, 6, 1, 18, 0)));
}

void TestFireOnceIsNotShared() {
    // each schedule keeps its own fire state
    wxDateTime start = wxDateTime::Now() + wxTimeSpan(0, 30);
    std::string startText = start.Format("%H:%M").ToStdString();
    Schedule hourly = Make({ { "StartTime", startText.c_str() }, { "EndTime", "23:59" }, { "FireFrequency", "Fire every hour" } });
    Schedule once = Make({ { "StartTime", startText.c_str() }, { "EndTime", "23:59" } });
    CHECK(!hourly.ShouldFire());
    CHECK(once.ShouldFire());
}

void TestFileCompatibility() {
    // a schedule that uses none of the new settings saves exactly the old attributes
    Schedule plain = Make({ { "Name", "Nightly" }, { "StartDate", "2026-11-01" }, { "EndDate", "2027-01-06" }, { "StartTime", "17:00" }, { "EndTime", "22:00" }, { "HardStop", "TRUE" } });
    wxXmlNode* n = plain.Save();
    CHECK(n->GetAttribute("StartDate") == "2026-11-01");
    CHECK(n->GetAttribute("EndDate") == "2027-01-06");
    CHECK(n->GetAttribute("HardStop") == "TRUE");
    for (const char* attr : { "StopAtEndOfLoop", "StartHoliday", "StartHolidayOffset", "EndHoliday", "EndHolidayOffset", "SkipDates" }) {
        CHECK(!HasAttribute(n, attr));
    }
    delete n;

    // the new settings round trip, and the resolved dates are written for older versions
    Schedule holiday = Make({ { "Name", "Season" }, { "StartDate", "2026-01-01" }, { "EndDate", "2026-01-06" }, { "EveryYear", "TRUE" }, { "StartHoliday", "thanksgiving_us" },
                              { "StartHolidayOffset", "1" }, { "EndHoliday", "epiphany" }, { "SkipDates", "2026-12-24" }, { "StopAtEndOfLoop", "TRUE" } });
    n = holiday.Save();
    CHECK(n->GetAttribute("StartDate") == "2026-11-27");
    CHECK(n->GetAttribute("EndDate") == "2027-01-06");
    CHECK(n->GetAttribute("StartHoliday") == "thanksgiving_us");
    CHECK(n->GetAttribute("StartHolidayOffset") == "1");
    CHECK(n->GetAttribute("EndHoliday") == "epiphany");
    CHECK(!HasAttribute(n, "EndHolidayOffset"));
    CHECK(n->GetAttribute("SkipDates") == "2026-12-24");
    CHECK(n->GetAttribute("StopAtEndOfLoop") == "TRUE");
    Schedule reloaded(n);
    CHECK(reloaded.IsStopAtEndOfLoop());
    CHECK(reloaded.GetSkipDates().size() == 1);
    CHECK(reloaded.IsActiveAt(D(2027, 11, 26, 19, 0)) == holiday.IsActiveAt(D(2027, 11, 26, 19, 0)));
    delete n;

    // copies keep the new settings
    Schedule copy(holiday, true);
    CHECK(copy.GetStartHoliday() == "thanksgiving_us" && copy.IsStopAtEndOfLoop() && copy.GetSkipDates().size() == 1);
}

} // namespace

int main(int argc, char** argv) {
    wxInitializer init(argc, argv);
    if (!init.IsOk()) {
        std::printf("could not initialize wxWidgets\n");
        return 2;
    }

    TestHolidays();
    TestOriginalBehavior();
    TestHolidaySeason();
    TestSkipDates();
    TestWindows();
    TestFireOnceIsNotShared();
    TestFileCompatibility();

    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
