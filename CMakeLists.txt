# Reference: http://qt-project.org/doc/qt-5.0/qtdoc/cmake-manual.html

cmake_minimum_required(VERSION 3.1)
# https://doc.qt.io/qt-5.10/cmake-manual.html
# CMake 3.1.0 adds the right compiler flags for us, require it

# Add folder where are supportive functions
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)
# Find includes in corresponding build directories
set(CMAKE_INCLUDE_CURRENT_DIR ON)

# ccache doesn't play well with travis, disable it?
#find_program(CCACHE_PROGRAM ccache)
#if(CCACHE_PROGRAM)
#    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE "${CCACHE_PROGRAM}")
#endif()

include(QtCommon) # cmake/QtCommon.cmake

include(CMakeProjectConfig.cmake)
set(PRODUCT_VERSION ${VERSION_MAJOR}.${VERSION_MINOR}.${VERSION_REVISION}.${VERSION_BUILD})
project(TimeCampDesktop VERSION ${PRODUCT_VERSION})

if (DEFINED ENV{APPVEYOR_API_URL})
    set(APPVEYOR_VERSION ${PRODUCT_VERSION}-b$ENV{APPVEYOR_BUILD_NUMBER})
    message("Calling $ENV{APPVEYOR_API_URL}/api/build with Version: ${APPVEYOR_VERSION}")
#    execute_process(COMMAND curl -v -H "Accept: application/json" -H "Content-Type: application/json" -X PUT -d "'{\"version\": \"${APPVEYOR_VERSION}\"}'" $ENV{APPVEYOR_API_URL}api/build)
    execute_process(COMMAND appveyor UpdateBuild -Version "${APPVEYOR_VERSION}")
    set(ENV{APPVEYOR_BUILD_VERSION} "${APPVEYOR_VERSION}")
endif()

if (NOT DEFINED PRODUCT_NAME)
    message(FATAL_ERROR "You haven't prepared CMakeProjectConfig.cmake file.
    See README.md/Developing for details.")
endif ()

# Set PROJECT_VERSION_PATCH and PROJECT_VERSION_TWEAK to 0 if not present, needed by add_project_meta
fix_project_version()

configure_file (
    "src/Versions.h.in"
    "${PROJECT_BINARY_DIR}/Versions.h"
)

set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if (WIN32)
    if ("${CMAKE_GENERATOR}" MATCHES "MinGW")
        message(FATAL_ERROR "There's no QtWebEngine (no chromium) for MinGW, you have to use MSVC on win32")
    elseif ("${CMAKE_GENERATOR}" MATCHES "Visual Studio" OR "${CMAKE_GENERATOR}" MATCHES "NMake" OR "${CMAKE_GENERATOR}" MATCHES "Ninja")
        LIST(APPEND CMAKE_PREFIX_PATH "C:\\WinDDK\\7600.16385.1\\lib\\win7\\i386")
    endif ()
elseif (UNIX AND NOT APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-rdynamic")
elseif (APPLE)
    set(CMAKE_EXE_LINKER_FLAGS "-framework Carbon -framework AppKit -framework IOKit -framework Foundation -framework CoreFoundation")
# -syslibroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk
endif ()

find_package(Qt5Core REQUIRED)
find_package(Qt5Gui REQUIRED)
find_package(Qt5Network REQUIRED)
find_package(Qt5Widgets REQUIRED)
find_package(Qt5WebEngineWidgets REQUIRED)
find_package(Qt5Sql REQUIRED)

if (UNIX AND NOT APPLE)
    find_package(Qt5X11Extras REQUIRED)
    find_package(Qt5DBus REQUIRED)
    message("x11 extras loaded")
elseif (APPLE)
    find_package(qt5macextras REQUIRED)
    message("Mac extras loaded")
elseif (WIN32)
    find_package(qt5winextras REQUIRED)
    message("Win extras loaded")
endif ()

foreach (plugin ${Qt5Sql_PLUGINS})
    get_target_property(_loc ${plugin} LOCATION)
    message("Plugin ${plugin} is at location ${_loc}")
    #    if(${plugin} MATCHES "Qt5::QSQLiteDriverPlugin")
    #        list(APPEND TC_LIBS ${_loc})
    #    endif()
endforeach ()

ADD_DEFINITIONS(-DUNICODE -D_UNICODE -DPSAPI_VERSION=1 -DQHOTKEY_LIB -DQHOTKEY_LIB_BUILD)

if ("${CMAKE_CXX_COMPILER}" MATCHES "clang")
    set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} -fno-limit-debug-info")
endif()

list(APPEND SOURCE_FILES
        "src/Settings.h" # a header without cpp file
        "src/Versions.h"
        "src/main.cpp"
        "src/DbManager.cpp"
        "src/MainWidget.cpp"
        "src/Overrides/TCRequestInterceptor.cpp"
        "src/Overrides/TCNavigationInterceptor.cpp"
        "src/Overrides/TCWebEngineView.cpp"
        "src/Overrides/ClickableLabel.cpp"
        "src/ApiHelper.cpp"
        "src/Comms.cpp"
        "src/AppData.cpp"
        "src/Task.cpp"
        "src/Keyword.cpp"
        "src/KeywordUtils.h"
        "src/AutoTracking.cpp"
        "src/Autorun.cpp"
        "src/WindowEventsManager.cpp"
        "src/TrayManager.cpp"
        "src/TCTimer.cpp"
        "src/Formatting.cpp"
        "third-party/lz4/lib/lz4.c"
        "third-party/QTLogRotation/logutils.cpp"
        "src/DataCollector/WindowEvents.cpp"
        "src/Widget/Widget.cpp"
        "src/Widget/FloatingWidget.cpp"
        )


if (WIN32)
    list(APPEND SOURCE_FILES
            "src/DataCollector/WindowEvents_W.cpp"
            "src/ControlIterator/IControlIterator.cpp"
            "src/ControlIterator/AccControlIterator.cpp"
            "src/ControlIterator/UIAControlIterator.cpp"
            )

    list(APPEND TC_LIBS
            "oleacc.lib"
            "psapi.lib"
            "Iphlpapi.lib"
            "OleAut32.lib"
            "Ole32.lib"
            "user32.lib"
            "comsuppw.lib"
            )
    set(Qt5_OS_LIBRARIES Qt5::WinExtras)
endif ()

if (UNIX AND NOT APPLE)
    list(APPEND SOURCE_FILES
            "src/ChromeUtils.cpp"
            "src/FirefoxUtils.cpp"
            "src/DataCollector/WindowEvents_U.cpp"
            )

    list(APPEND TC_LIBS
            "-lX11 -lXss"
            )
    set(Qt5_OS_LIBRARIES Qt5::X11Extras Qt5::DBus)
endif ()

if (APPLE)
    list(APPEND SOURCE_FILES
            "src/ChromeUtils.cpp"
            "src/FirefoxUtils.cpp"
            "src/DataCollector/WindowEvents_M.mm"
            "src/Utils_M.mm"
            "src/Widget/Widget_M.mm"
            "src/Widget/Widget_M_Cocoa.mm"
            )
    set_source_files_properties(src/*.mm PROPERTIES COMPILE_FLAGS "-x objective-c++")

    set(Qt5_OS_LIBRARIES Qt5::MacExtras)
endif ()


# QPM
set(QPM_PATH "third-party/vendor")
include_directories(${QPM_PATH})

# Qt useful modules dev
set(SkyCoder_PATH "${QPM_PATH}/de/skycoder42")
include_directories(${SkyCoder_PATH})


set(QHotkey_PATH "${SkyCoder_PATH}/qhotkey/QHotkey")
list(APPEND SOURCE_FILES
        "${QHotkey_PATH}/QHotkey"
        "${QHotkey_PATH}/qhotkey.cpp"
        "${QHotkey_PATH}/qhotkey_p.h"
        )

set(QSingleInstance_PATH "${SkyCoder_PATH}/qsingleinstance/QSingleInstance")
list(APPEND SOURCE_FILES
        "${QSingleInstance_PATH}/clientinstance.cpp"
        "${QSingleInstance_PATH}/qsingleinstance.cpp"
        "${QSingleInstance_PATH}/qsingleinstance_p.cpp"
        )

set(QTaskbarControl_PATH "${SkyCoder_PATH}/qtaskbarcontrol")
list(APPEND SOURCE_FILES
    "${QTaskbarControl_PATH}/qtaskbarcontrol.cpp"
    "${QTaskbarControl_PATH}/qtaskbarcontrol_p.h"
    )

set(QAutoStart_PATH "${SkyCoder_PATH}/qautostart")
list(APPEND SOURCE_FILES
    "${QAutoStart_PATH}/qautostart.cpp"
    "${QAutoStart_PATH}/qautostart_p.h"
    )


if (WIN32)
    list(APPEND SOURCE_FILES
            "${QHotkey_PATH}/qhotkey_win.cpp"
            "${QTaskbarControl_PATH}/qtaskbarcontrol_win.cpp"
            "${QAutoStart_PATH}/qautostart_win.cpp"
            )
elseif (UNIX AND NOT APPLE)
    list(APPEND SOURCE_FILES
            "${QHotkey_PATH}/qhotkey_x11.cpp"
            "${QTaskbarControl_PATH}/qtaskbarcontrol_x11.cpp"
            "${QAutoStart_PATH}/qautostart_x11.cpp"
            )
elseif (APPLE)
    list(APPEND SOURCE_FILES
            "${QHotkey_PATH}/qhotkey_mac.cpp"
            "${QTaskbarControl_PATH}/qtaskbarcontrol_mac.mm"
            "${QAutoStart_PATH}/qautostart_mac.cpp"
            )
endif ()

#if(CMAKE_BUILD_TYPE MATCHES Debug)
#    file(GLOB POCO_LIBS "path/to/debug/libs/*.a")
#elseif(CMAKE_BUILD_TYPE MATCHES Release)
#    file(GLOB POCO_LIBS "path/to/RELEASE/libs/*.a")
#endif()


add_project_meta(META_FILES_TO_INCLUDE)

set(RESOURCE_FILES res/TimeCampDesktop.qrc)


add_executable(${PROJECT_NAME} ${OS_BUNDLE} # Expands to WIN32 or MACOS_BUNDLE depending on OS
        ${SOURCE_FILES} ${META_FILES_TO_INCLUDE} ${RESOURCE_FILES}
        )

# add "Versions.h"
include_directories("${PROJECT_BINARY_DIR}")

if (WIN32)
    # hide windows console...
    if ("${CMAKE_GENERATOR}" MATCHES "MinGW")
        target_compile_options(${PROJECT_NAME} PUBLIC -Wl,--subsystem,windows -mwindows)
    elseif ("${CMAKE_GENERATOR}" MATCHES "Visual Studio" OR "${CMAKE_GENERATOR}" MATCHES "NMake" OR "${CMAKE_GENERATOR}" MATCHES "Ninja")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_DEBUG "/SUBSYSTEM:CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_DEBUG "_CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELWITHDEBINFO "/SUBSYSTEM:WINDOWS")
        set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_DEFINITIONS_RELWITHDEBINFO "_CONSOLE")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_RELEASE "/SUBSYSTEM:WINDOWS")
        set_target_properties(${PROJECT_NAME} PROPERTIES LINK_FLAGS_MINSIZEREL "/SUBSYSTEM:WINDOWS")
    endif ()
elseif(APPLE)
#     set_target_properties(${PROJECT_NAME} PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
     target_compile_options(${PROJECT_NAME} PUBLIC -isysroot /Applications/Xcode.app/Contents/Developer/Platforms/MacOSX.platform/Developer/SDKs/MacOSX.sdk)
endif ()

set(Qt5_LIBRARIES Qt5::Core Qt5::Gui Qt5::Network Qt5::Widgets Qt5::WebEngineWidgets Qt5::Sql)
target_link_libraries(${PROJECT_NAME} ${TC_LIBS} ${Qt5_LIBRARIES} ${Qt5_OS_LIBRARIES})
