/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

#include "ScheduleConfig.h"
#include "../xlights/src-core/settings/XLightsSettings.h"

#include <wx/confbase.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

#include <log.h>

namespace ScheduleConfig {

void MirrorIntoCore()
{
    // the core store has no file behind it in xSchedule, so nothing here is ever written to disk
    auto& core = XLightsSettings::Get();
    core.Load();
    core.DeleteAll("main");

    wxConfigBase* config = wxConfigBase::Get();
    if (config == nullptr) return;

    wxString key;
    long index = 0;
    int count = 0;
    for (bool more = config->GetFirstEntry(key, index); more; more = config->GetNextEntry(key, index)) {
        const std::string k = key.ToStdString();
        switch (config->GetEntryType(key)) {
        case wxConfigBase::Type_Boolean:
            core.WriteBool(k, config->ReadBool(key, false));
            break;
        case wxConfigBase::Type_Integer:
            core.WriteLong(k, config->ReadLong(key, 0));
            break;
        case wxConfigBase::Type_Float:
            core.WriteDouble(k, config->ReadDouble(key, 0.0));
            break;
        default:
            core.WriteString(k, config->Read(key, "").ToStdString());
            break;
        }
        ++count;
    }
    spdlog::debug("Mirrored {} settings into the xLights core.", count);
}

bool GetBool(const std::string& key, bool defaultValue)
{
    wxConfigBase* config = wxConfigBase::Get();
    return config != nullptr ? config->ReadBool(key, defaultValue) : defaultValue;
}

void SetBool(const std::string& key, bool value)
{
    wxConfigBase* config = wxConfigBase::Get();
    if (config != nullptr) {
        config->Write(key, value);
        config->Flush();
    }
    XLightsSettings::Get().WriteBool(key, value);
}

bool IsSuppressDarkMode()
{
    return GetBool("SuppressDarkMode", false);
}

void SetSuppressDarkMode(bool suppress)
{
    if (IsSuppressDarkMode() != suppress) {
        SetBool("SuppressDarkMode", suppress);
        wxMessageBox("Restart " + wxFileName(wxStandardPaths::Get().GetExecutablePath()).GetName() + " to enable/disable dark mode properly.");
    }
}

} // namespace ScheduleConfig
