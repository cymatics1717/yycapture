#include "obscapture.hpp"
#include <util/windows/win-version.h>
#include <dwmapi.h>
#include <util/dstr.h>
#include "media-io/video-frame.h"
#include <util/platform.h>
#include <d3d11.h>
#include <dxgi1_6.h>
#include <wrl/wrappers/corewrappers.h>
#include <wrl/client.h>

#include <Psapi.h>
#include "plugins/win-capture/obfuscate.h"

#define DL_OPENGL "libobs-opengl.dll"
#define DL_D3D11 "libobs-d3d11.dll"
#include <QDateTime>
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

    process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, false, id);
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

OBSCapture::OBSCapture(const HWND &target, QObject *parent):
        QObject(parent),fps(60),ovi({0}),display(nullptr),scene(nullptr),sink(target)
{
    initOBS();
//    QMetaObject::invokeMethod(this,"initOBS",Qt::QueuedConnection);
}

OBSCapture::~OBSCapture()
{
    qDebug() << obs_get_version_string();

    if(display){
         obs_display_destroy(display);
    }
    if(scene){
         obs_scene_release(scene);
    }
    for(auto v:sources){
         obs_source_release(v.second);
    }

    obs_set_output_source(0, nullptr);

    obs_shutdown();

    blog(LOG_INFO, "Number of memory leaks: %ld", bnum_allocs());
}

int OBSCapture::resetVideo(int fs){

    RECT rc;
    GetClientRect(sink, &rc);

//    struct obs_video_info ovi;
    int ret;
    ovi.adapter = 0;
    ovi.base_width = rc.right;
    ovi.base_height = rc.bottom;
    ovi.fps_num = fs;
    ovi.fps_den = 1;
    ovi.graphics_module = DL_D3D11;
    ovi.output_format = VIDEO_FORMAT_RGBA;
    ovi.output_width = ovi.base_width;
    ovi.output_height = ovi.base_height;

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

static void RenderWindow(void *data, uint32_t cx, uint32_t cy) {

//    blog(LOG_DEBUG,"%d#################%d,%d",__LINE__,cx,cy);
    obs_render_main_texture();
    UNUSED_PARAMETER(data);
    UNUSED_PARAMETER(cx);
    UNUSED_PARAMETER(cy);
}

static void RenderWindowCopy(void *data, uint32_t cx, uint32_t cy) {
    OBSCapture *that = static_cast<OBSCapture *>(data);
    blog(LOG_DEBUG,"%d#################%d,%d,%s",
         __LINE__,cx,cy,QDateTime::currentDateTime().toString("hh-mm-ss.zzz").toStdString().c_str());

    obs_enter_graphics();

    obs_render_main_texture();

    if(0){
        gs_texture_t * tex = obs_get_main_texture();
    //    gs_texture_2d *tex2d = static_cast<gs_texture_2d *>(tex);

        using Microsoft::WRL::ComPtr;

        ID3D11Device *const d3d_device = (ID3D11Device *)gs_get_device_obj();
        ComPtr<ID3D11DeviceContext> context;
        d3d_device->GetImmediateContext(&context);

        ComPtr<ID3D11Texture2D> tex2d = (ID3D11Texture2D *)gs_texture_get_obj(tex);
        ComPtr<ID3D11Texture2D> incoming = nullptr;

        D3D11_TEXTURE2D_DESC desc;
        tex2d->GetDesc(&desc);
        desc.MiscFlags = D3D11_RESOURCE_MISC_SHARED;
        HRESULT hr = d3d_device->CreateTexture2D(&desc, NULL, incoming.GetAddressOf());;
        context->CopyResource(incoming.Get(),tex2d.Get());
    }

    obs_leave_graphics();
}

static obs_display_t* CreateDisplay(HWND sink)
{

    RECT rc;
    GetClientRect(sink, &rc);

    gs_init_data info = {0,};
    info.cx = rc.right;
    info.cy = rc.bottom;
    info.format = GS_RGBA;
    info.zsformat = GS_ZS_NONE;
    info.window.hwnd = sink;

    return obs_display_create(&info, 0);
}

static int GetProgramDataPath(char *path, size_t size, const char *name)
{
    return os_get_program_data_path(path, size, name);
}

int OBSCapture::initOBS()
{
    if (!obs_startup("en-US", nullptr, nullptr)){
        blog(LOG_ERROR,"Couldn't create OBS");
        return 1;
    }

    resetVideo();
    obs_load_all_modules();
    obs_log_loaded_modules();
    obs_post_load_modules();

#ifdef _WIN32
    /* disable aero for old windows */
    uint32_t winVer = GetWindowsVersion();
    if (winVer > 0 && winVer < 0x602) {
        SetAeroEnabled(true);
    }
#endif

    return 0;
}

enum window_capture_method {
    METHOD_AUTO,
    METHOD_BITBLT,
    METHOD_WGC,
};

int OBSCapture::addGameSource(HWND from){

    static int cnt =0;
    cnt++;

    struct dstr name = {0};
    struct dstr clazz = {0};
    struct dstr exe = {0};

    QString cur_id;

    if (from) {
        get_window_title(&name, from);
        get_window_class(&clazz, from);
        get_window_exe(&exe, from);
        if (name.array && clazz.array && exe.array) {
            cur_id = QString("%1:%2:%3").arg(name.array,clazz.array,exe.array);
//            snprintf(cur_id,size_t(cur_id),"%s:%s:%s",name.array,clazz.array,exe.array);
        }
    }
    dstr_free(&name);
    dstr_free(&clazz);
    dstr_free(&exe);

    obs_source_t *source = obs_source_create("game_capture", qPrintable(QString("game"+cur_id)), NULL, nullptr);
    if (!source){
        blog(LOG_ERROR,"Couldn't create random test source");
        return 1;
    }

    sources[from] = source;

    obs_data_t *data = obs_data_create();
    obs_data_set_string(data, "window", qPrintable(cur_id));
    obs_data_set_string(data, "capture_mode", "window");
//    obs_data_set_int(data, "priority", 2);
//    obs_data_set_int(data, "hook_rate", 1);
//    obs_data_set_bool(data, "capture_cursor", true);
//    obs_data_set_bool(data, "anti_cheat_hook", true);
//    obs_data_set_bool(data, "allow_transparency", false);
//    obs_data_set_bool(data, "capture_overlays", false);
//    obs_data_set_bool(data, "limit_framerate", false);
//    obs_data_set_bool(data, "sli_compatibility", false);
//    obs_data_set_int(data, "method", METHOD_BITBLT);
    obs_source_update(source, data);
    obs_data_release(data);

    obs_sceneitem_t *item = obs_scene_add(scene, source);
    {
        obs_sceneitem_selected(item);
        vec2 pos,offset{{200,200}};
        obs_sceneitem_get_pos(item, &pos);
        for(int i=0;i<cnt;++i){
            vec2_add(&pos, &pos, &offset);
        }
        obs_sceneitem_set_pos(item, &pos);
        obs_sceneitem_set_rot(item,60);
    }
    obs_set_output_source(0, source);

    return 0;
}

int OBSCapture::addWindowSource(HWND from)
{
    struct dstr name = {0};
    struct dstr clazz = {0};
    struct dstr exe = {0};
    QString cur_id;
    if (from) {
        get_window_title(&name, from);
        get_window_class(&clazz, from);
        get_window_exe(&exe, from);
        if (name.array && clazz.array && exe.array) {
            cur_id = QString("%1:%2:%3").arg(name.array,clazz.array,exe.array);
        }
    }
    dstr_free(&name);
    dstr_free(&clazz);
    dstr_free(&exe);


    obs_source_t *source = obs_source_create("window_capture", qPrintable(QString("window"+cur_id)), NULL, nullptr);
    if (!source){
        blog(LOG_ERROR,"Couldn't create random test source %s",qPrintable(cur_id));
        return 1;
    }

    sources[from] = source;

    obs_data_t *data = obs_data_create();
    obs_data_set_string(data, "window", qPrintable(cur_id));
//    obs_data_set_int(data, "method", METHOD_WGC);
//    obs_data_set_int(data, "method", METHOD_BITBLT);
    obs_source_update(source, data);
    obs_data_release(data);

    static int cnt =0;
    cnt++;
    obs_sceneitem_t *item = obs_scene_add(scene, source);
//    {
//        obs_sceneitem_selected(item);
//        vec2 pos,offset{{200,200}};
//        obs_sceneitem_get_pos(item, &pos);
//        for(int i=0;i<cnt;++i){
//            vec2_add(&pos, &pos, &offset);
//        }
//        obs_sceneitem_set_pos(item, &pos);
//        obs_sceneitem_set_rot(item,60);
//    }
    obs_set_output_source(0, source);
//obs_get_output_flags("");
    return 0;
}

int OBSCapture::start()
{
    scene = obs_scene_create("test scene");
    if (!scene){
        blog(LOG_ERROR,"Couldn't create scene");
        return 2;
    }
//    obs_set_output_source(0, nullptr);

    display = CreateDisplay(sink);
    obs_display_add_draw_callback(display, RenderWindow, nullptr);
//    obs_display_add_draw_callback(display, RenderWindowCopy, this);
    return 0;
}
