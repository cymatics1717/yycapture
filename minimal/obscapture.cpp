#include "obscapture.hpp"
#include <util/windows/win-version.h>
#include <dwmapi.h>
#include <util/dstr.h>
#include "media-io/video-frame.h"

#include <Psapi.h>
#include "plugins/win-capture/obfuscate.h"

#define DL_OPENGL "libobs-opengl.dll"
#define DL_D3D11 "libobs-d3d11.dll"
#include <QDateTime>
#include <QDebug>
#include <QMetaObject>

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

OBSCapture::OBSCapture(const HWND &target, const HWND &_preview, QObject *parent):QObject(parent),
    context({0}),display(nullptr),sink(target),from(_preview),scene(nullptr),item(nullptr)
{
    initOBS();
//    QMetaObject::invokeMethod(this,"initOBS",Qt::QueuedConnection);
}

OBSCapture::~OBSCapture()
{
    if(display){
         obs_display_destroy(display);
    }
    if(scene){
         obs_scene_release(scene);
    }
    if(source){
         obs_source_release(source);
    }
    obs_set_output_source(0, nullptr);

    obs_shutdown();

//    obs_source_release(source);
    blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
}

int OBSCapture::ResetVideo(void){
//    struct obs_video_info ovi;
    int ret;
    context.ovi.adapter = 0;
    context.ovi.base_width = 1080;
    context.ovi.base_height = 1920;
    context.ovi.fps_num = 144000;
    context.ovi.fps_den = 1001;
    context.ovi.graphics_module = DL_D3D11;
    context.ovi.output_format = VIDEO_FORMAT_RGBA;
    context.ovi.output_width = 720;
    context.ovi.output_height = 1280;

    ret = obs_reset_video(&context.ovi);
    if (ret != OBS_VIDEO_SUCCESS) {
        if (ret == OBS_VIDEO_CURRENTLY_ACTIVE) {
            blog(LOG_WARNING, "Tried to reset when "
                      "already active");
            return ret;
        }

        /* Try OpenGL if DirectX fails on windows */
        if (strcmp(context.ovi.graphics_module, DL_OPENGL) != 0) {
            blog(LOG_WARNING,
                 "Failed to initialize obs video (%d) "
                 "with graphics_module='%s', retrying "
                 "with graphics_module='%s'",
                 ret, context.ovi.graphics_module, DL_OPENGL);
            context.ovi.graphics_module = DL_OPENGL;
            ret = obs_reset_video(&context.ovi);
        }
    }

    return ret;
}

static void RenderWindow(void *data, uint32_t cx, uint32_t cy) {

//    blog(LOG_DEBUG,"%d#################%d,%d",__LINE__,cx,cy);
    obs_render_main_texture();
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

static obs_display_t* CreateDisplay(HWND sink)
{
    gs_init_data info = {};
    info.cx = 1280;
    info.cy = 720;
    info.format = GS_RGBA;
    info.zsformat = GS_ZS_NONE;
    info.window.hwnd = sink;

    return obs_display_create(&info, 0);
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

    return 0;
}

int OBSCapture::start()
{
    source = obs_source_create("window_capture", "window capture source", NULL, nullptr);
    if (!source){
        blog(LOG_ERROR,"Couldn't create random test source");
        return 1;
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
    dstr_free(&name);
    dstr_free(&clazz);
    dstr_free(&exe);


    obs_data_t *data = obs_data_create();
    obs_data_set_string(data, "window", cur_id);
    obs_source_update(source, data);
    obs_data_release(data);

    scene = obs_scene_create("test scene");
    if (!scene){
        blog(LOG_ERROR,"Couldn't create scene");
        return 2;
    }

    item = obs_scene_add(scene, source);
    obs_set_output_source(0, source);
    display = CreateDisplay(sink);
    obs_display_add_draw_callback(display, RenderWindow, nullptr);
    return 0;
}
