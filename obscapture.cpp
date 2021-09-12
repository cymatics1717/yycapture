#include "obscapture.hpp"
#include <util/windows/win-version.h>
#include <dwmapi.h>
#include <util/dstr.h>

#include <Psapi.h>
#include "plugins/win-capture/obfuscate.h"

#define DL_OPENGL "libobs-opengl.dll"
#define DL_D3D11 "libobs-d3d11.dll"
#include <QDateTime>
#include <QDebug>

static int ResetVideo(void)
{
    struct obs_video_info ovi;
    int ret;
    ovi.adapter = 0;
    ovi.base_width = 1080;
    ovi.base_height = 1920;
    ovi.fps_num = 144000;
    ovi.fps_den = 1001;
    ovi.graphics_module = DL_D3D11;
    ovi.output_format = VIDEO_FORMAT_RGBA;
    ovi.output_width = 720;
    ovi.output_height = 1280;

    ret = obs_reset_video(&ovi);
    if (ret != OBS_VIDEO_SUCCESS) {
        if (ret == OBS_VIDEO_CURRENTLY_ACTIVE) {
            blog(LOG_WARNING, "Tried to reset when "
                      "already active");
            return ret;
        }

        /* Try OpenGL if DirectX fails on windows */
        if (strcmp(ovi.graphics_module, DL_OPENGL) != 0) {
            blog(LOG_WARNING,
                 "Failed to initialize obs video (%d) "
                 "with graphics_module='%s', retrying "
                 "with graphics_module='%s'",
                 ret, ovi.graphics_module, DL_OPENGL);
            ovi.graphics_module = DL_OPENGL;
            ret = obs_reset_video(&ovi);
        }
    }

    return ret;
}

static void SetAeroEnabled(bool enable)
{
    static HRESULT(WINAPI * func)(UINT) = nullptr;
    static bool failed = false;

    if (!func) {
        if (failed) {
            return;
        }

        HMODULE dwm = LoadLibraryW(L"dwmapi");
        if (!dwm) {
            failed = true;
            return;
        }

        func = reinterpret_cast<decltype(func)>(
            GetProcAddress(dwm, "DwmEnableComposition"));
        if (!func) {
            failed = true;
            return;
        }
    }

    func(enable ? DWM_EC_ENABLECOMPOSITION : DWM_EC_DISABLECOMPOSITION);
}

static uint32_t GetWindowsVersion()
{
    static uint32_t ver = 0;

    if (ver == 0) {
        struct win_version_info ver_info;

        get_win_ver(&ver_info);
        ver = (ver_info.major << 8) | ver_info.minor;
    }

    return ver;
}

static HMODULE kernel32(void)
{
    static HMODULE kernel32_handle = NULL;
    if (!kernel32_handle)
        kernel32_handle = GetModuleHandleA("kernel32");
    return kernel32_handle;
}

static inline HANDLE open_process(DWORD desired_access, bool inherit_handle, DWORD process_id)
{
    typedef HANDLE(WINAPI * PFN_OpenProcess)(DWORD, BOOL, DWORD);
    static PFN_OpenProcess open_process_proc = NULL;
    if (!open_process_proc)
        open_process_proc = (PFN_OpenProcess)get_obfuscated_func(
            kernel32(), "B}caZyah`~q", 0x2D5BEBAF6DDULL);

    return open_process_proc(desired_access, inherit_handle, process_id);
}

static bool check_window_valid(HWND window)
{
    DWORD styles, ex_styles;
    RECT rect;

    if (!IsWindowVisible(window) || IsIconic(window))
        return false;

    GetClientRect(window, &rect);
    styles = (DWORD)GetWindowLongPtr(window, GWL_STYLE);
    ex_styles = (DWORD)GetWindowLongPtr(window, GWL_EXSTYLE);

    if (ex_styles & WS_EX_TOOLWINDOW)
        return false;
    if (styles & WS_CHILD)
        return false;
    if (rect.bottom == 0 || rect.right == 0)
        return false;

    return true;
}

static bool get_window_exe(struct dstr *name, HWND window)
{
    wchar_t wname[MAX_PATH];
    struct dstr temp = {0};
    bool success = false;
    HANDLE process = NULL;
    char *slash;
    DWORD id;

    GetWindowThreadProcessId(window, &id);
    if (id == GetCurrentProcessId())
        return false;

    process = open_process(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
    if (!process)
        goto fail;

    if (!GetProcessImageFileNameW(process, wname, MAX_PATH))
        goto fail;

    dstr_from_wcs(&temp, wname);
    slash = strrchr(temp.array, '\\');
    if (!slash)
        goto fail;

    dstr_copy(name, slash + 1);
    success = true;

fail:
    if (!success)
        dstr_copy(name, "unknown");

    dstr_free(&temp);
    CloseHandle(process);
    return true;
}

static HWND next_window(HWND window, HWND *parent, bool use_findwindowex)
{
    if (*parent) {
        window = *parent;
        *parent = NULL;
    }

    while (true) {
        if (use_findwindowex)
            window = FindWindowEx(GetDesktopWindow(), window, NULL,
                          NULL);
        else
            window = GetNextWindow(window, GW_HWNDNEXT);

        if (!window || check_window_valid(window))
            break;
    }

    return window;
}

static HWND first_window(HWND *parent, bool *use_findwindowex)
{
    HWND window = FindWindowEx(GetDesktopWindow(), NULL, NULL, NULL);

    if (!window) {
        *use_findwindowex = false;
        window = GetWindow(GetDesktopWindow(), GW_CHILD);
    } else {
        *use_findwindowex = true;
    }

    *parent = NULL;
    if (!check_window_valid(window)) {
        window = next_window(window, parent, *use_findwindowex);

        if (!window && *use_findwindowex) {
            *use_findwindowex = false;

            window = GetWindow(GetDesktopWindow(), GW_CHILD);
            if (!check_window_valid(window))
                window = next_window(window, parent, *use_findwindowex);
        }
    }

    return window;
}

static void get_window_class(struct dstr *clazz, HWND hwnd)
{
    wchar_t temp[256];

    temp[0] = 0;
    if (GetClassNameW(hwnd, temp, sizeof(temp) / sizeof(wchar_t)))
        dstr_from_wcs(clazz, temp);
}

static void get_window_title(struct dstr *name, HWND hwnd)
{

    int len = GetWindowTextLengthW(hwnd);
    if (!len)
        return;

    wchar_t *temp = (wchar_t *)malloc(sizeof(wchar_t) * (len + 1));
    if (GetWindowTextW(hwnd, temp, len + 1))
        dstr_from_wcs(name, temp);

    free(temp);
}

static void preview_tick(void *param, float sec)
{
    static auto last = QDateTime::currentDateTime();
    auto now = QDateTime::currentDateTime();
    qDebug() << last.msecsTo(now);
    last = now;
    UNUSED_PARAMETER(sec);

    auto ctx = (struct preview_output *)param;

    if (ctx->texrender)
        gs_texrender_reset(ctx->texrender);
}

OBSCapture::OBSCapture(const HWND &_preview, QObject *parent):QObject(parent),from(_preview)
{
    qDebug() << initOBS();
}

OBSCapture::~OBSCapture()
{
    obs_shutdown();
    blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
}

int OBSCapture::initOBS()
{
    if (!obs_startup("en-US", nullptr, nullptr)){
        blog(LOG_ERROR,"Couldn't create OBS");
        return 1;
    }

    /* load modules */
    obs_load_all_modules();
    obs_log_loaded_modules();
    obs_post_load_modules();

    /* init obs graphic things */
    ResetVideo();

#ifdef _WIN32
    /* disable aero for old windows */
    uint32_t winVer = GetWindowsVersion();
    if (winVer > 0 && winVer < 0x602) {
        SetAeroEnabled(true);
    }
#endif

    source = obs_source_create("window_capture", "window capture source", NULL, nullptr);
    if (!source){
        blog(LOG_ERROR,"Couldn't create random test source");
        return 2;
    }


    RECT rc;
    GetClientRect(from, &rc);

    struct dstr name = {0};
    struct dstr clazz = {0};
    struct dstr exe = {0};
    char cur_id[255];

    if (from) {
        screenx = rc.right;
        screeny = rc.bottom;
        get_window_title(&name, from);
        get_window_class(&clazz, from);
        get_window_exe(&exe, from);
        if (name.array && clazz.array && exe.array) {
            strcpy_s(cur_id, sizeof(cur_id), name.array);
            strcat_s(cur_id, sizeof(cur_id), ":");
            strcat_s(cur_id, sizeof(cur_id), clazz.array);
            strcat_s(cur_id, sizeof(cur_id), ":");
            strcat_s(cur_id, sizeof(cur_id), exe.array);
        }
    }

    const char *setting = "window";
    obs_data_t *data = obs_data_create();
    //const char *cur_id = "obs-studio - Microsoft Visual Studio:HwndWrapper[DefaultDomain;;c2d3a612-3846-463d-9141-4a7fb5e26412]:devenv.exe";
    obs_data_set_string(data, setting, cur_id);
    obs_source_update(source, data);

    /* ------------------------------------------------------ */
    /* create scene and add source to scene (twice) */
    /*obs_scene_t *scene = obs_scene_create("test scene");
    if (!scene)
        throw "Couldn't create scene";*/

    //AddTestItems(scene, source, from);

    /* ------------------------------------------------------ */
    /* set the scene as the primary draw source and go */
    /*obs_source_t *output_source = obs_scene_get_source(scene);
    obs_set_output_source(0, output_source);*/

    /* ------------------------------------------------------ */
    /* create display for output and set the output render callback */
    /*DisplayContext display = CreateDisplay();
    obs_display_add_draw_callback(display, RenderWindow, nullptr);*/

    /* activate source view then window caputre can run background */
    obs_source_inc_active(source);
    obs_add_tick_callback(preview_tick, source);

    return 0;
}
