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

#include <wx/config.h>

#include <map>
#include <memory>

// A plugin's own settings. Built as a library a plugin has its own wx, so wxConfigBase::Get() is its own; compiled
// into xSchedule (XSCHEDULE_STATIC_PLUGINS, macOS) that would be xSchedule's settings.
inline wxConfigBase* PluginConfig(const wxString& name)
{
#ifdef XSCHEDULE_STATIC_PLUGINS
    static std::map<wxString, std::unique_ptr<wxConfig>> configs;
    auto& config = configs[name];
    if (!config) config = std::make_unique<wxConfig>(name);
    return config.get();
#else
    (void)name;
    return wxConfigBase::Get();
#endif
}
