#ifndef WINDOWEVENTS_W_H
#define WINDOWEVENTS_W_H

#include "WindowEvents.h"
#include <Windows.h>
#include <oleacc.h>
#include <OleAuto.h>
#include <objbase.h>
#include <tchar.h>
#include <Psapi.h>
#include <comutil.h>
#include <UIAutomation.h>

#include "src/ControlIterator/IControlIterator.h"
#include "src/ControlIterator/AccControlIterator.h"
#include "src/ControlIterator/UIAControlIterator.h"

typedef struct
{
    DWORD ownerpid;
    DWORD childpid;
} windowinfo;

class WindowEvents_W : public WindowEvents
{
public:
    void static logAppName(QString appName, QString windowName, HWND passedHwnd);
    static double getWindowsVersion();

protected:
    void run() override;
    unsigned long getIdleTime() override;

private:
    HWINEVENTHOOK appNameChangeEventHook;
    HWINEVENTHOOK appChangeEventHook;
    MSG winApiMsg;

    static void tryToSaveApp(IAccessible *pAcc, VARIANT varChild, HWND hwnd);

    static void
    HandleWinEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread,
                   DWORD dwmsEventTime);
    static void
    HandleWinNameEvent(HWINEVENTHOOK hook, DWORD event, HWND hwnd, LONG idObject, LONG idChild, DWORD dwEventThread,
                       DWORD dwmsEventTime);

    static BOOL EnumChildAppHostWindowsCallback(HWND hWnd, LPARAM lp);
    static void GetProcessName(HWND hWnd, TCHAR *procName);
    static void ParseProcessName(HANDLE hProcess, TCHAR *processName);

    void InitializeWindowsHook(HWINEVENTHOOK appChangeEventHook, HWINEVENTHOOK appNameChangeEventHook);
    void ShutdownWindowsHook(HWINEVENTHOOK appChangeEventHook, HWINEVENTHOOK appNameChangeEventHook);
};

class WindowDetails
{
    Q_DISABLE_COPY(WindowDetails)
public:
//    WindowDetails();
    static WindowDetails &instance();
    HWND currenthwnd;

    bool standardAccCallback(IControlItem *node, void *userData);
    bool chromeAccCallback(IControlItem *node, void *userData);
    bool operaAccCallback(IControlItem *node, void *userData);

    QString GetInfoFromBrowser(HWND passedHwnd);
    QString GetInfoFromFirefox(HWND passedHwnd);
    bool startsWithAnAllowedProtocol(QString checkedStr);
    bool isBrowser(QString processName);

    typedef bool(WindowDetails::*DetailsCallback)(IControlItem *node, void *userData);
    DetailsCallback pointerMagic = &WindowDetails::standardAccCallback;

    const QRegExp &getURL_REGEX() const;
    void setURL_REGEX(const QRegExp &URL_REGEX);

protected:
    explicit WindowDetails();

private:
    QString URL_REGEX_STR;
    QRegExp URL_REGEX;
};

class FirefoxURL
{
public:
    static std::wstring GetFirefoxURL(HWND hwnd);

private:
    static HRESULT
    GetControlCondition(IUIAutomation *automation, const long controlType, IUIAutomationCondition **controlCondition);

    static IUIAutomation *_automation;
    static IUIAutomationElement *firefoxElement;
    static IUIAutomationCondition *toolbarCondition;
    static IUIAutomationElementArray *toolbars;
    static IUIAutomationElement *toolbarElement;
    static IUIAutomationCondition *comboCondition;
    static IUIAutomationElement *comboElement;
    static IUIAutomationCondition *editCondition;
    static IUIAutomationElement *urlElement;
    static IUnknown *patternInter;
    static IUIAutomationValuePattern *valuePattern;

    static void releaseAutomation();
    static std::wstring onFail(QString message, HRESULT hr);
};

#endif // WINDOWEVENTS_W_H
