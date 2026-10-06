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

// Holidays a schedule's start or end date can follow. The ids are stored in show files and must
// never change; the names are only for display.

#include <wx/datetime.h>

#include <string>
#include <vector>

namespace Holidays {

struct Holiday {
    std::string id;
    std::string name;
};

const std::vector<Holiday>& All();

// The holiday's date in the given year (time 00:00), or an invalid date for an unknown id.
wxDateTime DateFor(const std::string& id, int year);

std::string NameFor(const std::string& id);

} // namespace Holidays
