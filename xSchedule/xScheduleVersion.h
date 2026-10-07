#pragma once

#include <string>
#include "../xlights/src-core/xLightsVersion.h"

// Written by a fork's release build, never committed. Without it the defaults below give the standard build.
#if __has_include("xschedule_build_info.h")
#include "xschedule_build_info.h"
#endif
#ifndef XSCHEDULE_BUILD_LABEL
#define XSCHEDULE_BUILD_LABEL ""                             // shown after the version, e.g. "JBD Build"
#endif
#ifndef XSCHEDULE_BUILD_VERSION
#define XSCHEDULE_BUILD_VERSION ""                           // what the update check compares, e.g. "2026.06.1-jbd"
#endif
#ifndef XSCHEDULE_RELEASES_REPO
#define XSCHEDULE_RELEASES_REPO "xLightsSequencer/xSchedule" // GitHub repo for the update check and Download menu
#endif

static const std::string xschedule_version_string = "2026.06";
static const std::string xschedule_build_date     = __DATE__;
static const std::string xschedule_build_label    = XSCHEDULE_BUILD_LABEL;
static const std::string xschedule_releases_repo  = XSCHEDULE_RELEASES_REPO;

// no longer in xLightsVersion.h
inline const std::string& GetBitness() {
    static const std::string bits = sizeof(size_t) == 8 ? "64bit" : "32bit";
    return bits;
}

inline std::string GetXScheduleDisplayVersionString() {
#ifndef __WXOSX__
    std::string v = xschedule_version_string + " " + GetBitness();
#else
    std::string v = xschedule_version_string;
#endif
    if (!xschedule_build_label.empty()) v += " (" + xschedule_build_label + ")";
    return v;
}

inline std::string GetXScheduleUpdateVersion() {
    const std::string v = XSCHEDULE_BUILD_VERSION;
    return v.empty() ? xschedule_version_string : v;
}

// crash reports from a labeled build carry the label in the app name
inline std::string GetXScheduleAppName() {
    std::string name = "xSchedule";
    if (!xschedule_build_label.empty()) {
        name += "-" + xschedule_build_label;
        for (auto& c : name) {
            if (c == ' ') c = '-';
        }
    }
    return name;
}
