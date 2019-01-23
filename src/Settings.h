#pragma once
#ifndef THEGUI_SETTINGS_H
#define THEGUI_SETTINGS_H

#include "Versions.h"

#define ORGANIZATION_DOMAIN "TimeCamp.com"

#define WINDOW_NAME "TimeCamp"
#define LOGIN_URL "https://www.timecamp.com/auth/login"
#define APIKEY_URL "https://www.timecamp.com/auth/token"
#define API_URL "https://desktop.timecamp.com/third_party/api"
#define APPLICATION_URL "https://www.timecamp.com/app#/timesheets/timer"
#define OFFLINE_URL "https://www.timecamp.com/helper/setdate/today/offline"
#define CONTACT_SUPPORT_URL "https://www.timecamp.com/kb/"

#define CONTACT_EMAIL "desktopapp@timecamp.com"

#define NO_TIMER_TEXT "0:00"

#define MAIN_ICON ":/Icons/AppIcon.ico"
#define MAIN_BG ":/Icons/AppIcon.ico"

// db params
#define DB_FILENAME "localdb.sqlite"
#define APP_ICON "AppIcon.png"
#define DESKTOP_FILE_PATH "desktopFilePath"

// connection params
#define CONN_USER_AGENT "TC Desktop App 2.0" // hardcoded, frontend checks
#define CONN_CUSTOM_HEADER_NAME "X-DAPP"
#define CONN_CUSTOM_HEADER_VALUE APPLICATION_VERSION
#define SETT_API_SERVICE_FIELD "tc_dapp_2_api"

// settings fields
#define SETT_TRACK_PC_ACTIVITIES "TRACK_PC_ACTIVITIES"
#define SETT_TRACK_AUTO_SWITCH "TRACK_AUTO_SWITCH"
#define SETT_SHOW_WIDGET "SHOW_WIDGET"
#define SETT_STOP_TIMER_ON_SHUTDOWN "STOP_TIMER_ON_SHUTDOWN"

#define SETT_APIKEY "API_KEY"
#define SETT_LAST_SYNC "LAST_SYNC"
#define SETT_WAS_WINDOW_LEFT_OPENED "WAS_WINDOW_LEFT_OPENED"
#define SETT_IS_FIRST_RUN "IS_FIRST_RUN"

#define SETT_USER_ID "USER_ID"
#define SETT_ROOT_GROUP_ID "ROOT_GROUP_ID"
#define SETT_PRIMARY_GROUP_ID "PRIMARY_GROUP_ID"
#define SETT_WEB_SETTINGS_PREFIX "SETT_WEB_"

#define SETT_HIDDEN_COMPUTER_ACTIVITIES_CONST_NAME "computer activity"

#define MAX_ACTIVITIES_BATCH_SIZE 400
#define MAX_LOG_TEXT_LENGTH 150

#define KB_SHORTCUTS_START_TIMER "ctrl+alt+shift+."
#define KB_SHORTCUTS_STOP_TIMER "ctrl+alt+shift+,"
#define KB_SHORTCUTS_OPEN_WINDOW "ctrl+alt+shift+/"

#define ACTIVITY_DEFAULT_APP_NAME "explorer2"
#define ACTIVITY_IDLE_NAME "IDLE"

#define ACTIVITIES_SYNC_INTERVAL 30 // seconds
#define WEBPAGE_DATA_SYNC_INTERVAL 1 // seconds

#endif //THEGUI_SETTINGS_H
