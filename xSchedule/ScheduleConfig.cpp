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
#include <wx/config.h>
#include <wx/tokenzr.h>
#include <wx/filename.h>
#include <wx/msgdlg.h>
#include <wx/stdpaths.h>

#include <log.h>

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>

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

// Where xLights keeps settings.json (the same place xLights' GetSettingsFilePath uses)
static std::filesystem::path XLightsSettingsFile()
{
    std::filesystem::path dir;
#if defined(_WIN32) || defined(__WXMSW__)
    const char* appData = std::getenv("APPDATA");
    dir = std::filesystem::path(appData && *appData ? appData : ".") / "xLights";
#elif defined(__APPLE__)
    const char* home = std::getenv("HOME");
    dir = std::filesystem::path(home && *home ? home : ".") / "Library" / "Application Support" / "xLights";
#else
    const char* xdgConfig = std::getenv("XDG_CONFIG_HOME");
    if (xdgConfig && *xdgConfig) {
        dir = std::filesystem::path(xdgConfig) / "xLights";
    } else {
        const char* home = std::getenv("HOME");
        dir = std::filesystem::path(home && *home ? home : ".") / ".config" / "xLights";
    }
#endif
    return dir / "settings.json";
}

// The "main" section of xLights' settings.json, re-read only when the file changes: the status bar asks for the
// xLights show folder on every update.
static const nlohmann::json* XLightsMainSettings()
{
    static nlohmann::json main;
    static std::filesystem::file_time_type lastWrite;
    static bool loaded = false;

    std::error_code ec;
    const auto file = XLightsSettingsFile();
    const auto written = std::filesystem::last_write_time(file, ec);
    if (ec) return nullptr;

    if (!loaded || written != lastWrite) {
        std::ifstream in(file);
        // xLights may be part way through writing it; keep what was read last time
        auto parsed = nlohmann::json::parse(in, nullptr, false);
        if (parsed.is_discarded() || !parsed.is_object()) return loaded ? &main : nullptr;
        main = parsed.contains("main") && parsed["main"].is_object() ? parsed["main"] : nlohmann::json::object();
        lastWrite = written;
        loaded = true;
    }
    return &main;
}

std::string GetXLightsSetting(const std::string& key)
{
    if (const auto* main = XLightsMainSettings(); main != nullptr && main->contains(key)) {
        const auto& value = (*main)[key];
        if (value.is_string()) return value.get<std::string>();
        if (!value.is_null()) return value.dump();
    }

    wxConfig config("xLights");
    wxString value;
    config.Read(wxString(key), &value, "");
    return value.ToStdString();
}

std::list<std::string> GetXLightsMediaDirs()
{
    std::list<std::string> dirs;
    wxStringTokenizer tokens(wxString(GetXLightsSetting("MediaDir")), "|");
    while (tokens.HasMoreTokens()) {
        std::string dir = tokens.GetNextToken().ToStdString();
        if (!dir.empty() && std::find(dirs.begin(), dirs.end(), dir) == dirs.end()) {
            dirs.push_back(dir);
        }
    }
    return dirs;
}

bool IsSameFolder(const std::string& a, const std::string& b)
{
    if (a == b) return true;
    if (a.empty() || b.empty()) return false;
    return wxFileName::DirName(a).SameAs(wxFileName::DirName(b));
}

} // namespace ScheduleConfig
