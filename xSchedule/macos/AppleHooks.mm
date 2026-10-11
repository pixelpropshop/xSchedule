/***************************************************************
 * This source files comes from the xLights project
 * https://www.xlights.org
 * https://github.com/xLightsSequencer/xLights
 * See the github commit history for a record of contributing
 * developers.
 * Copyright claimed based on commit dates recorded in Github
 * License: https://github.com/xLightsSequencer/xLights/blob/master/License.txt
 **************************************************************/

// The one Apple hook the xLights core calls that xSchedule's macOS build provides itself: xLights' own version
// lives beside code that needs its Swift module.

#import <Foundation/Foundation.h>

#include <string>

std::string DescribeCurrentAppleException() {
    // Must be called from inside a catch handler; the rethrow recovers the type.
    try {
        throw;
    } catch (NSException* e) {
        std::string res = "Objective-C exception \"";
        res += (e.name != nil) ? e.name.UTF8String : "(no name)";
        res += "\": \"";
        res += (e.reason != nil) ? e.reason.UTF8String : "(no reason)";
        res += "\".";
        NSArray<NSString*>* frames = e.callStackSymbols;
        if (frames.count > 0) {
            res += " Raised at:";
            for (NSUInteger i = 0; i < frames.count && i < 12; ++i) {
                res += "\n    ";
                res += frames[i].UTF8String;
            }
        }
        return res;
    } catch (...) {
        return "";
    }
}
