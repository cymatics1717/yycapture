#include "display_wrapper.hpp"

DisplayWrapper::DisplayWrapper(HWND hwnd, uint32_t width, uint32_t height) {
    gs_init_data info = {};
	info.cx = width;
	info.cy = height;
	info.format = GS_RGBA;
	info.zsformat = GS_ZS_NONE;
	info.window.hwnd = hwnd;
    obs_display_t *d = obs_display_create(&info, 0);
    this->display = d;
}


void DisplayWrapper::getObsDisplaySize(uint32_t *width, uint32_t *height)
{
    obs_display_t *display = this->display;
	return obs_display_size(display, width, height);
}

void DisplayWrapper::destroy() {
	obs_display_destroy(display);
}

void DisplayWrapper::setDisplayRenderCallback(void (*callback)(void *param, uint32_t cx, uint32_t cy)) {
    obs_display_t *display = this->display;
    return 	obs_display_add_draw_callback(display, callback, nullptr);
}

void RenderWindow(void *data, uint32_t cx, uint32_t cy) {

//    blog(LOG_DEBUG,"%d#################%d,%d",__LINE__,cx,cy);
    obs_render_main_texture();
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
	
}
