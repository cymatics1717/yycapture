#include "display_wrapper.hpp"

void DisplayWrapper::config(HWND hwnd, uint32_t width, uint32_t height)
{
	gs_init_data info = {};
	info.cx = width;
	info.cy = height;
	info.format = GS_RGBA;
	info.zsformat = GS_ZS_NONE;
	info.window.hwnd = hwnd;
	display = obs_display_create(&info, 0);
}

void DisplayWrapper::getObsDisplaySize(uint32_t *width, uint32_t *height)
{
	return obs_display_size(display, width, height);
}

void DisplayWrapper::setDisplayRenderCallback(void (*callback)(void *, uint32_t,
							       uint32_t))
{
	return obs_display_add_draw_callback(display, callback, nullptr);
}

void RenderWindow(void *data, uint32_t cx, uint32_t cy)
{

	obs_render_main_texture();
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(cx);
	UNUSED_PARAMETER(cy);
}
