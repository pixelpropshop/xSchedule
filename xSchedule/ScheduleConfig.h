#pragma once

/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include <list>
#include <string>

// xSchedule's settings stay in wxConfig (HKCU\Software\xSchedule on Windows). The shared xLights wx helpers
// (IsDarkMode, GetConfigBool, ...) read xLights' own settings store instead, so read and write xSchedule's
// settings here; MirrorIntoCore copies them into that store (in memory only) for the helpers to see.
namespace ScheduleConfig {
    void MirrorIntoCore();

    bool GetBool(const std::string& key, bool defaultValue);
    void SetBool(const std::string& key, bool value);

    bool IsSuppressDarkMode();
    void SetSuppressDarkMode(bool suppress);

    // xLights moved its settings out of the registry into settings.json in April 2026 and stopped updating the
    // registry. These read xLights' own settings from there, falling back to the registry for older xLights.
    std::string GetXLightsSetting(const std::string& key);
    std::list<std::string> GetXLightsMediaDirs();
    // Show folders can differ in slashes and case and still be the same folder
    bool IsSameFolder(const std::string& a, const std::string& b);
}
