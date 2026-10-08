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

#include <cmath>
#include <cstdio>
#include <initializer_list>
#include <string>
#include <utility>

#include "../City.h"
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
    // every year: the current or next season, so the year depends on today
    wxDateTime season = s.GetEffectiveStartDate();
    CHECK(season.IsSameDate(Holidays::DateFor("thanksgiving_us", season.GetYear()) + wxDateSpan::Day()));
    CHECK(s.GetEffectiveEndDate().IsSameDate(D(season.GetYear() + 1, 1, 6)));
    CHECK(s.GetEffectiveEndDate() >= wxDateTime::Today());

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
    CHECK(n->GetAttribute("StartDate") == holiday.GetEffectiveStartDate().Format("%Y-%m-%d"));
    CHECK(n->GetAttribute("EndDate") == wxString::Format("%d-01-06", holiday.GetEffectiveStartDate().GetYear() + 1));
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


// holiday offsets that cross New Year, and overnight windows with skip dates
void TestReviewFindings() {
    {
        // Christmas + 10 days lands in the next year; saving and reloading must not move it again
        Schedule s = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-25" }, { "StartTime", "17:00" }, { "EndTime", "22:00" },
                            { "EndHoliday", "christmas" }, { "EndHolidayOffset", "10" } });
        CHECK(s.GetEffectiveEndDate().IsSameDate(D(2027, 1, 4)));
        wxXmlNode* n = s.Save();
        CHECK(n->GetAttribute("EndDate") == "2027-01-04");
        Schedule reloaded(n);
        CHECK(reloaded.GetEffectiveEndDate().IsSameDate(D(2027, 1, 4)));
        wxXmlNode* n2 = reloaded.Save();
        CHECK(n2->GetAttribute("EndDate") == "2027-01-04");
        delete n2;
        delete n;
    }
    {
        // New Year's Day - 5 days lands in the previous year
        Schedule s = Make({ { "StartDate", "2027-01-01" }, { "EndDate", "2027-01-10" }, { "StartTime", "17:00" }, { "EndTime", "22:00" },
                            { "StartHoliday", "newyearsday" }, { "StartHolidayOffset", "-5" } });
        CHECK(s.GetEffectiveStartDate().IsSameDate(D(2026, 12, 27)));
        wxXmlNode* n = s.Save();
        Schedule reloaded(n);
        CHECK(reloaded.GetEffectiveStartDate().IsSameDate(D(2026, 12, 27)));
        delete n;
    }
    {
        // every year, from 5 days before New Year's Day to Epiphany
        Schedule s = Make({ { "StartDate", "2026-01-01" }, { "EndDate", "2026-01-06" }, { "EveryYear", "TRUE" }, { "StartTime", "17:00" }, { "EndTime", "22:00" },
                            { "StartHoliday", "newyearsday" }, { "StartHolidayOffset", "-5" }, { "EndHoliday", "epiphany" } });
        CHECK(!s.IsActiveAt(D(2026, 12, 26, 18, 0)));
        CHECK(s.IsActiveAt(D(2026, 12, 27, 18, 0)));
        CHECK(s.IsActiveAt(D(2026, 12, 28, 18, 0)));
        CHECK(s.IsActiveAt(D(2027, 1, 3, 18, 0)));
        CHECK(!s.IsActiveAt(D(2027, 1, 7, 18, 0)));
        CHECK(s.IsActiveAt(D(2025, 12, 30, 18, 0)));
    }
    {
        // the last night of an every year season still runs past midnight
        Schedule s = Make({ { "StartDate", "2026-11-26" }, { "EndDate", "2027-01-06" }, { "EveryYear", "TRUE" }, { "StartTime", "18:00" }, { "EndTime", "02:00" } });
        CHECK(s.IsActiveAt(D(2027, 1, 6, 23, 0)));
        CHECK(s.IsActiveAt(D(2027, 1, 7, 1, 0)));
        CHECK(!s.IsActiveAt(D(2027, 1, 7, 3, 0)));
        CHECK(!s.IsActiveAt(D(2027, 1, 7, 19, 0)));
        CHECK(s.IsActiveAt(D(2027, 11, 26, 19, 0)));
    }
    {
        // a skipped night includes the part of its window after midnight, and only that night
        Schedule s = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "18:00" }, { "EndTime", "02:00" }, { "SkipDates", "2026-12-24" } });
        CHECK(s.IsActiveAt(D(2026, 12, 24, 1, 0)));   // still the night of the 23rd
        CHECK(!s.IsActiveAt(D(2026, 12, 24, 19, 0)));
        CHECK(!s.IsActiveAt(D(2026, 12, 25, 0, 30)));
        CHECK(s.IsActiveAt(D(2026, 12, 25, 19, 0)));
    }
    {
        // a date range that is over needs no long search
        Schedule s = Make({ { "StartDate", "2020-01-01" }, { "EndDate", "2020-12-31" }, { "StartTime", "17:00" }, { "EndTime", "22:00" } });
        CHECK(s.GetUpcomingWindows(D(2026, 1, 1), 1).empty());
        Schedule later = Make({ { "StartDate", "2030-06-01" }, { "EndDate", "2030-06-30" }, { "StartTime", "17:00" }, { "EndTime", "22:00" } });
        auto w = later.GetUpcomingWindows(D(2026, 1, 1), 1, 5000);
        CHECK(w.size() == 1 && w[0].first == D(2030, 6, 1, 17, 0));
    }
}

void TestOvernightDays() {
    // Friday and Saturday shows that run past midnight: the hours after midnight belong to the night before
    Schedule s = Make({ { "DOW", "FriSat" }, { "StartDate", "2026-01-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "17:00" }, { "EndTime", "00:30" } });
    CHECK(!s.IsActiveAt(D(2026, 10, 8, 23, 0)));   // Thursday night
    CHECK(!s.IsActiveAt(D(2026, 10, 9, 0, 15)));   // still Thursday night
    CHECK(s.IsActiveAt(D(2026, 10, 9, 18, 0)));    // Friday
    CHECK(s.IsActiveAt(D(2026, 10, 10, 0, 15)));   // Friday night after midnight
    CHECK(s.IsActiveAt(D(2026, 10, 11, 0, 15)));   // Saturday night after midnight
    CHECK(!s.IsActiveAt(D(2026, 10, 11, 0, 45)));
    CHECK(!s.IsActiveAt(D(2026, 10, 11, 18, 0)));  // Sunday

    // the Upcoming tab agrees with what plays
    auto w = s.GetUpcomingWindows(D(2026, 10, 8, 12, 0), 2);
    CHECK(w.size() == 2);
    if (w.size() == 2) {
        CHECK(w[0].first == D(2026, 10, 9, 17, 0) && w[0].second == D(2026, 10, 10, 0, 30));
        CHECK(w[1].first == D(2026, 10, 10, 17, 0) && w[1].second == D(2026, 10, 11, 0, 30));
    }

    // every other day counts the night too
    Schedule n = Make({ { "NthDay", "2" }, { "NthDayOffset", "0" }, { "StartDate", "2026-01-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "20:00" }, { "EndTime", "01:00" } });
    wxDateTime on = D(2026, 3, 1, 21, 0);
    if (!n.IsActiveAt(on)) on += wxDateSpan::Day();
    CHECK(n.IsActiveAt(on));
    wxDateTime after = on + wxDateSpan::Day();
    after.SetHour(0);
    after.SetMinute(30);
    CHECK(n.IsActiveAt(after));                    // the same night, after midnight
}

void TestJSONAndState() {
    // names with quotes or backslashes still produce valid JSON
    Schedule s = Make({ { "Name", R"(Kids "Request" Hour \ 2)" }, { "StartTime", "17:00" }, { "EndTime", "22:00" } });
    // a schedule loaded from a file starts inactive (checked before GetJSON, which evaluates the live state)
    CHECK(!s.IsActive());
    std::string json = s.GetJSON("ref\"1");
    CHECK(json.find(R"("name":"Kids \"Request\" Hour \\ 2")") != std::string::npos);
    CHECK(json.find("\"reference\":\"ref\\\"1\"") != std::string::npos);
}

// minutes past midnight UTC of a local time of day on the given date
int UtcMinutes(const wxDateTime& local) {
    wxDateTime noon = local.GetDateOnly();
    noon.SetHour(12);
    int offset = (noon.FromUTC() - noon).GetMinutes();
    return ((local.GetHour() * 60 + local.GetMinute() - offset) % 1440 + 1440) % 1440;
}

// a longitude where the sun keeps this computer's clock time, so sun times land in the evening whatever its time zone
double LocalLongitude() {
    wxDateTime noon = D(2026, 12, 21, 12, 0);
    return (noon.FromUTC() - noon).GetMinutes() / 4.0;
}

void TestSunTimes() {
    // New York on 21 December 2026 (NOAA): civil dawn 11:46, sunrise 12:17, sunset 21:32, civil dusk 22:01 UTC
    const double lat = 40.7128;
    const double lon = -74.0060;
    wxDateTime day = D(2026, 12, 21);
    auto closeTo = [](int actual, int expected) { return std::abs(actual - expected) <= 3; };
    CHECK(closeTo(UtcMinutes(City::GetSunTime(lat, lon, day, City::SunEvent::Dawn)), 11 * 60 + 46));
    CHECK(closeTo(UtcMinutes(City::GetSunTime(lat, lon, day, City::SunEvent::Sunrise)), 12 * 60 + 17));
    CHECK(closeTo(UtcMinutes(City::GetSunTime(lat, lon, day, City::SunEvent::Sunset)), 21 * 60 + 32));
    CHECK(closeTo(UtcMinutes(City::GetSunTime(lat, lon, day, City::SunEvent::Dusk)), 22 * 60 + 1));

    // where it never gets dark enough for dusk there is still a time
    wxDateTime tromso = City::GetSunTime(69.65, 18.96, D(2026, 6, 21), City::SunEvent::Dusk);
    CHECK(tromso.IsValid());

    CHECK(City::GetNearestCity(40.6, -73.9) == "New York");
    CHECK(City::GetNearestCity(-33.0, 151.0) == "Sydney");

    // a schedule from dusk to sunrise
    double localLon = LocalLongitude();
    Schedule::SetLocation(true, 40.0, localLon);
    wxDateTime dusk = City::GetSunTime(40.0, localLon, day, City::SunEvent::Dusk);
    Schedule s = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "Dusk" }, { "EndTime", "sunrise" } });
    CHECK(s.GetStartTimeAsString() == "dusk");
    CHECK(!s.IsActiveAt(dusk - wxTimeSpan(0, 5)));
    CHECK(s.IsActiveAt(dusk + wxTimeSpan(0, 5)));
    auto w = s.GetUpcomingWindows(D(2026, 12, 21, 12, 0), 1);
    CHECK(w.size() == 1 && w[0].first == dusk);

    // saved as sunset plus a flag, which older versions read as sunset
    wxXmlNode* n = s.Save();
    CHECK(n->GetAttribute("StartTime") == "sunset");
    CHECK(n->GetAttribute("StartTwilight") == "TRUE");
    CHECK(n->GetAttribute("EndTime") == "sunrise");
    CHECK(!HasAttribute(n, "EndTwilight"));
    Schedule reloaded(n);
    CHECK(reloaded.GetStartTimeAsString() == "dusk" && reloaded.GetEndTimeAsString() == "sunrise");
    delete n;

    Schedule dawn = Make({ { "StartTime", "sunup" }, { "EndTime", "SUNRISE" }, { "EndTwilight", "TRUE" } });
    CHECK(dawn.GetStartTimeAsString() == "sunup" && dawn.GetEndTimeAsString() == "dawn");

    Schedule plain = Make({ { "StartTime", "sunset" }, { "EndTime", "22:00" } });
    n = plain.Save();
    CHECK(!HasAttribute(n, "StartTwilight") && !HasAttribute(n, "EndTwilight"));
    delete n;

    // an unknown location falls back to fixed times
    Schedule::SetLocation(false, 0, 0);
    Schedule unknown = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "dusk" }, { "EndTime", "23:00" } });
    CHECK(!unknown.IsActiveAt(D(2026, 12, 21, 19, 55)));
    CHECK(unknown.IsActiveAt(D(2026, 12, 21, 20, 5)));
    Schedule::SetLocation(true, -33.861481, 151.205475);
}

void TestExtendEndTime() {
    double localLon = LocalLongitude();
    Schedule::SetLocation(true, 40.0, localLon);
    wxDateTime sunset = City::GetSunTime(40.0, localLon, D(2026, 12, 21), City::SunEvent::Sunset);

    Schedule s = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "12:00" }, { "EndTime", "sunset" }, { "OffOffsetMins", "15" } });
    wxDateTime after = sunset + wxTimeSpan(0, 25);
    CHECK(!s.IsActiveAt(after));
    s.AddMinsToEndTime(30);
    CHECK(s.IsActiveAt(after));
    CHECK(!s.IsActiveAt(sunset + wxTimeSpan(0, 50)));

    Schedule fixed = Make({ { "StartDate", "2026-12-01" }, { "EndDate", "2026-12-31" }, { "StartTime", "17:00" }, { "EndTime", "22:00" } });
    CHECK(!fixed.IsActiveAt(D(2026, 12, 21, 22, 15)));
    fixed.AddMinsToEndTime(30);
    CHECK(fixed.IsActiveAt(D(2026, 12, 21, 22, 15)));

    Schedule::SetLocation(true, -33.861481, 151.205475);
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
    TestReviewFindings();
    TestOvernightDays();
    TestJSONAndState();
    TestSunTimes();
    TestExtendEndTime();

    std::printf("%d checks, %d failed\n", g_checks, g_failures);
    return g_failures == 0 ? 0 : 1;
}
