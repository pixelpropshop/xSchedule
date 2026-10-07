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
}
