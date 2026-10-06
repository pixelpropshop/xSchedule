/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "Holidays.h"

namespace Holidays {

namespace {

wxDateTime Fixed(int year, wxDateTime::Month month, int day) {
    return wxDateTime((wxDateTime::wxDateTime_t)day, month, year);
}

// the nth (1 based) given weekday of a month; n = -1 means the last one
wxDateTime NthWeekday(int year, wxDateTime::Month month, wxDateTime::WeekDay weekday, int n) {
    wxDateTime d;
    d.SetToWeekDay(weekday, n, month, year);
    return d.GetDateOnly();
}

// Gregorian Easter Sunday (anonymous Gregorian algorithm)
wxDateTime Easter(int year) {
    const int a = year % 19;
    const int b = year / 100;
    const int c = year % 100;
    const int d = b / 4;
    const int e = b % 4;
    const int f = (b + 8) / 25;
    const int g = (b - f + 1) / 3;
    const int h = (19 * a + b - d - g + 15) % 30;
    const int i = c / 4;
    const int k = c % 4;
    const int l = (32 + 2 * e + 2 * i - h - k) % 7;
    const int m = (a + 11 * h + 22 * l) / 451;
    const int month = (h + l - 7 * m + 114) / 31; // 3 = March, 4 = April
    const int day = ((h + l - 7 * m + 114) % 31) + 1;
    return Fixed(year, (wxDateTime::Month)(month - 1), day);
}

} // namespace

const std::vector<Holiday>& All() {
    static const std::vector<Holiday> holidays = {
        { "newyearsday", "New Year's Day (Jan 1)" },
        { "epiphany", "Epiphany (Jan 6)" },
        { "valentines", "Valentine's Day (Feb 14)" },
        { "stpatricks", "St. Patrick's Day (Mar 17)" },
        { "easter", "Easter Sunday" },
        { "mothersday_us", "Mother's Day (US, 2nd Sun in May)" },
        { "memorialday_us", "Memorial Day (US, last Mon in May)" },
        { "fathersday_us", "Father's Day (US, 3rd Sun in Jun)" },
        { "canadaday", "Canada Day (Jul 1)" },
        { "independenceday_us", "Independence Day (US, Jul 4)" },
        { "laborday_us", "Labor Day (US, 1st Mon in Sep)" },
        { "thanksgiving_ca", "Thanksgiving (Canada, 2nd Mon in Oct)" },
        { "halloween", "Halloween (Oct 31)" },
        { "veteransday_us", "Veterans Day (US, Nov 11)" },
        { "thanksgiving_us", "Thanksgiving (US, 4th Thu in Nov)" },
        { "christmaseve", "Christmas Eve (Dec 24)" },
        { "christmas", "Christmas Day (Dec 25)" },
        { "boxingday", "Boxing Day (Dec 26)" },
        { "newyearseve", "New Year's Eve (Dec 31)" },
    };
    return holidays;
}

wxDateTime DateFor(const std::string& id, int year) {
    if (id == "newyearsday") return Fixed(year, wxDateTime::Jan, 1);
    if (id == "epiphany") return Fixed(year, wxDateTime::Jan, 6);
    if (id == "valentines") return Fixed(year, wxDateTime::Feb, 14);
    if (id == "stpatricks") return Fixed(year, wxDateTime::Mar, 17);
    if (id == "easter") return Easter(year);
    if (id == "mothersday_us") return NthWeekday(year, wxDateTime::May, wxDateTime::Sun, 2);
    if (id == "memorialday_us") return NthWeekday(year, wxDateTime::May, wxDateTime::Mon, -1);
    if (id == "fathersday_us") return NthWeekday(year, wxDateTime::Jun, wxDateTime::Sun, 3);
    if (id == "canadaday") return Fixed(year, wxDateTime::Jul, 1);
    if (id == "independenceday_us") return Fixed(year, wxDateTime::Jul, 4);
    if (id == "laborday_us") return NthWeekday(year, wxDateTime::Sep, wxDateTime::Mon, 1);
    if (id == "thanksgiving_ca") return NthWeekday(year, wxDateTime::Oct, wxDateTime::Mon, 2);
    if (id == "halloween") return Fixed(year, wxDateTime::Oct, 31);
    if (id == "veteransday_us") return Fixed(year, wxDateTime::Nov, 11);
    if (id == "thanksgiving_us") return NthWeekday(year, wxDateTime::Nov, wxDateTime::Thu, 4);
    if (id == "christmaseve") return Fixed(year, wxDateTime::Dec, 24);
    if (id == "christmas") return Fixed(year, wxDateTime::Dec, 25);
    if (id == "boxingday") return Fixed(year, wxDateTime::Dec, 26);
    if (id == "newyearseve") return Fixed(year, wxDateTime::Dec, 31);
    return wxInvalidDateTime;
}

std::string NameFor(const std::string& id) {
    for (const auto& h : All()) {
        if (h.id == id) return h.name;
    }
    return id;
}

} // namespace Holidays
