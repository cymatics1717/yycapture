#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QScreen>
#include <QQmlContext>

#include "obscapture.hpp"

#include <unordered_map>
#include <iostream>
#include <Windows.h>

#include <psapi.h>
#include <tlhelp32.h>

typedef struct {
    int type;
    std::string func;
} hookItem;

typedef struct {
    HWND winID;
    DWORD pid;
    DWORD tid;
    std::string title;
    std::string filepath;
} capsule;

typedef std::unordered_map<std::string, capsule> Pool;
static Pool pool;


static void do_log(int log_level, const char *msg, va_list args, void *param)
{
    char bla[4096];
    vsnprintf(bla, 4095, msg, args);
    OutputDebugStringA(bla);
    OutputDebugStringA("\n");

    //if (log_level < LOG_WARNING)
    //	__debugbreak();

    UNUSED_PARAMETER(param);
}


static BOOL CALLBACK EnumWindowsProc(HWND hWnd,LPARAM lParam)
{
    Pool *pool = reinterpret_cast<Pool*>(lParam);

    if (IsWindowVisible(hWnd)) {
        int length = GetWindowTextLength(hWnd);
        if(length != 0){
            char buffer[MAX_PATH]={0};
            GetWindowText(hWnd, buffer,MAX_PATH);
//            std::string windowTitle(buffer);

            DWORD hProcess;
            DWORD hThreadID = GetWindowThreadProcessId(hWnd, &hProcess);
            char filename[MAX_PATH]={0};
            HANDLE processHandle = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, hProcess);
//            if (processHandle != NULL) {
//                GetModuleFileNameEx(processHandle, NULL, filename, MAX_PATH);

//                HMODULE hMods[1024];
//                DWORD cbNeeded;
//                if(EnumProcessModules(processHandle, hMods, sizeof(hMods), &cbNeeded) && lParam!=NULL)
//                {
//                    std::cout <<"\t" <<(cbNeeded / sizeof(HMODULE))<<std::string(80,'*') << std::endl;
//                    for (int i = 0; i < (cbNeeded / sizeof(HMODULE)); i++ )
//                    {
//                        TCHAR szModName[MAX_PATH];
//                        if ( GetModuleFileNameEx( processHandle, hMods[i], szModName, sizeof(szModName) / sizeof(TCHAR)))
//                        {
//                            char tmp[1024]={0};
//                            snprintf(tmp,MAX_PATH,TEXT("\t[%p.%-5ld]:%s (0x%08X)"),hWnd,hProcess,szModName, hMods[i]);
//                            std::wcout << tmp << std::endl;
//                        }
//                    }
//                }
//                CloseHandle(processHandle);
//            }

            char tmp[MAX_PATH];
            snprintf(tmp,MAX_PATH,"[%p.%08lX.%08lX] %s(%s)",hWnd,hProcess,hThreadID,buffer,filename);
            std::cout << tmp << std::endl;
            pool->emplace(tmp+9,capsule{hWnd,hProcess,hThreadID,buffer,filename});
        }
    }

    return true;
}

int main(int argc, char *argv[])
{
    base_set_log_handler(do_log, nullptr);

#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    QCoreApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
#endif
    qputenv("QML2_IMPORT_PATH", "D:/Qt/5.15.2/msvc2019_64/qml");
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreated,
                     &app, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine.load(url);

    QQuickWindow *preview = static_cast<QQuickWindow*>(engine.rootObjects().first());
    HWND hWnd = HWND(preview->winId());

//    OBSCapture cap(hWnd, getWindowByString("WeChat"),&app);
    OBSCapture cap(hWnd,&app);
////    cap.start();
//    cap.startGame();
//    engine.rootContext()->setContextProperty("backend",&cap);

    std::string exename = "pptx";
//    std::string exename = "WeChat";
    {
        EnumWindows(EnumWindowsProc, LPARAM(&pool));
        for (auto v : pool) {
            std::cout << v.first << std::endl;
            if (v.first.find(exename) != std::string::npos) {
                std::cout << GetCurrentProcessId() << ":" << GetCurrentThreadId()
                    << "---found " << exename << ": " << v.second.pid
                    << ": "<< v.second.winID << std::endl;
                cap.addGameSource(v.second.winID);
//                cap.addWindowSource(v.second.winID);
                //break;
            }
        }
    }
    cap.start();

    return app.exec();
}
