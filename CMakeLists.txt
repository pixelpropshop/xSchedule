# The macOS build of xSchedule. Linux builds with the Makefile (from xSchedule/xSchedule.cbp) and Windows with the
# Visual Studio solution; this file is for macOS only.
#
#   cd xlights/macOS && scripts/download_deps          # the xLights macOS dependency bundle
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build                                 # build/xSchedule.app
cmake_minimum_required(VERSION 3.24)
project(xSchedule LANGUAGES C CXX OBJC OBJCXX)

if(NOT APPLE)
    message(FATAL_ERROR "This CMake build is for macOS. Linux uses the Makefile and Windows the Visual Studio solution.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)
if(NOT CMAKE_OSX_DEPLOYMENT_TARGET)
    set(CMAKE_OSX_DEPLOYMENT_TARGET 12.0)
endif()
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

set(XS_DEPS "${CMAKE_SOURCE_DIR}/xlights/macOS/dependencies" CACHE PATH "The xLights macOS dependency bundle")
if(NOT EXISTS "${XS_DEPS}/include/wx-3.3/wx/wx.h")
    message(FATAL_ERROR "The xLights macOS dependency bundle is missing from ${XS_DEPS}; run scripts/download_deps in xlights/macOS")
endif()

# The sources are the ones the Linux project lists, so a new file is added in one place for both
file(READ "${CMAKE_SOURCE_DIR}/xSchedule/xSchedule.cbp" XS_CBP)
string(REGEX MATCHALL "Unit filename=\"[^\"]+\\.(cpp|c)\"" XS_UNITS "${XS_CBP}")
set(XS_SOURCES)
foreach(unit ${XS_UNITS})
    string(REGEX REPLACE "Unit filename=\"([^\"]+)\"" "\\1" file "${unit}")
    list(APPEND XS_SOURCES "${CMAKE_SOURCE_DIR}/xSchedule/${file}")
endforeach()
# Linux links the system libltc; macOS compiles the copy Windows uses
file(GLOB XS_LTC "${CMAKE_SOURCE_DIR}/xSchedule/libltc/*.c")
list(APPEND XS_SOURCES ${XS_LTC})

# On Apple the xLights core decodes and plays audio and video with Apple's frameworks, through these bridges
set(XS_APPLE "${CMAKE_SOURCE_DIR}/xlights/macOS/src-apple-core")
list(APPEND XS_SOURCES
    "${CMAKE_SOURCE_DIR}/xlights/src-core/media/AudioToolboxDecoder.cpp"
    "${CMAKE_SOURCE_DIR}/xlights/src-core/media/AVAudioEngineOutput.cpp"
    "${CMAKE_SOURCE_DIR}/xlights/src-core/media/AVFoundationVideoReader.cpp"
    "${XS_APPLE}/media/AudioToolboxBridge.mm"
    "${XS_APPLE}/media/AVAudioEngineOutputBridge.mm"
    "${XS_APPLE}/media/AVFoundationVideoBridge.mm"
    "${XS_APPLE}/osxUtils/xlVideoToolboxUtils.mm"
    "${CMAKE_SOURCE_DIR}/xSchedule/macos/AppleHooks.mm"
)

# PortMidi: only its Windows files are kept here, so the macOS build takes it from upstream
include(FetchContent)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)
set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
FetchContent_Declare(portmidi GIT_REPOSITORY https://github.com/PortMidi/portmidi.git GIT_TAG v2.0.4 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(portmidi)

add_executable(xSchedule MACOSX_BUNDLE ${XS_SOURCES})

target_include_directories(xSchedule PRIVATE
    "${CMAKE_SOURCE_DIR}/xSchedule/include"
    "${CMAKE_SOURCE_DIR}/xSchedule"
    "${CMAKE_SOURCE_DIR}/xlights/include"
    "${CMAKE_SOURCE_DIR}/xlights/src-core"
    "${CMAKE_SOURCE_DIR}/xlights/src-core/utils"
    "${CMAKE_SOURCE_DIR}/xlights/src-core/render"
    "${CMAKE_SOURCE_DIR}/xlights/src-ui-wx"
    "${CMAKE_SOURCE_DIR}/xlights/src-ui-wx/shared/utils"
    "${CMAKE_SOURCE_DIR}/xlights/common"
    "${CMAKE_SOURCE_DIR}/dependencies"
    "${CMAKE_SOURCE_DIR}/xlights/dependencies"
    "${CMAKE_SOURCE_DIR}/dependencies/pugixml/src"
    "${CMAKE_SOURCE_DIR}/dependencies/spdlog/include"
    # not osxUtils itself: with it on the path the core takes xLights' Apple file hooks, which need its Swift module
    "${XS_APPLE}"
    "${XS_DEPS}/lib/wx/include/osx_cocoa-unicode-static-3.3"
    "${XS_DEPS}/include/wx-3.3"
    "${XS_DEPS}/include"
)
target_compile_definitions(xSchedule PRIVATE
    __WXOSX_COCOA__ __WXMAC__ __WXOSX__ _FILE_OFFSET_BITS=64 wxUSE_GUI=1
    $<$<CONFIG:Release>:NDEBUG>
)
target_compile_options(xSchedule PRIVATE -Wno-deprecated-declarations $<$<COMPILE_LANGUAGE:OBJCXX>:-fobjc-arc>)

set(XS_FRAMEWORKS
    Cocoa AppKit Foundation Carbon IOKit CoreFoundation CoreServices ApplicationServices Security SystemConfiguration
    AudioToolbox CoreAudio CoreMIDI CoreMedia CoreVideo AVFoundation VideoToolbox QuartzCore OpenGL Metal
    GameController CoreHaptics ForceFeedback UniformTypeIdentifiers CoreImage Accelerate AVFAudio
)
set(XS_FRAMEWORK_LINKS)
foreach(fw ${XS_FRAMEWORKS})
    list(APPEND XS_FRAMEWORK_LINKS "-framework ${fw}")
endforeach()

set(XS_STATIC
    wx_osx_cocoau_adv-3.3 wx_osx_cocoau_core-3.3 wx_baseu_net-3.3 wx_baseu_xml-3.3 wx_baseu-3.3
    wxpng-3.3 wxjpeg-3.3 wxwebp-3.3 wxregexu-3.3 wxexpat-3.3 wxzlib-3.3
    avformat avcodec avfilter avdevice swscale swresample avutil
    SDL2 curl ssl crypto zstd
)
set(XS_STATIC_PATHS)
foreach(lib ${XS_STATIC})
    list(APPEND XS_STATIC_PATHS "${XS_DEPS}/lib/lib${lib}.a")
endforeach()

target_link_libraries(xSchedule PRIVATE
    ${XS_STATIC_PATHS}
    portmidi
    ${XS_FRAMEWORK_LINKS}
    iconv bz2 z
)

set_target_properties(xSchedule PROPERTIES
    OUTPUT_NAME "xSchedule"
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_SOURCE_DIR}/build_scripts/macos/Info.plist.in"
    MACOSX_BUNDLE_BUNDLE_NAME "xSchedule"
    MACOSX_BUNDLE_GUI_IDENTIFIER "org.xlights.xSchedule"
)

# the web interface and the controller definitions live in Contents/Resources, where wxStandardPaths looks for them
add_custom_command(TARGET xSchedule POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_BUNDLE_CONTENT_DIR:xSchedule>/Resources"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/bin/xScheduleWeb" "$<TARGET_BUNDLE_CONTENT_DIR:xSchedule>/Resources/xScheduleWeb"
    COMMAND ${CMAKE_COMMAND} -E copy_directory "${CMAKE_SOURCE_DIR}/xlights/resources/controllers" "$<TARGET_BUNDLE_CONTENT_DIR:xSchedule>/Resources/controllers"
)
if(EXISTS "${CMAKE_BINARY_DIR}/xSchedule.icns")
    add_custom_command(TARGET xSchedule POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy "${CMAKE_BINARY_DIR}/xSchedule.icns" "$<TARGET_BUNDLE_CONTENT_DIR:xSchedule>/Resources/xSchedule.icns")
endif()
