/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

// The wx file hooks wxUtilities leaves to xLights' macOS UI library on Apple, which xSchedule doesn't build.

#include <wx/filename.h>
#include <wx/string.h>

#include "../../xlights/src-core/utils/ExternalHooks.h"

bool FileExists(const wxString& s, bool waitForDownload) {
    return FileExists(s.ToStdString(), waitForDownload);
}

bool FileExists(const wxFileName& fn, bool waitForDownload) {
    return FileExists(fn.GetFullPath().ToStdString(), waitForDownload);
}
