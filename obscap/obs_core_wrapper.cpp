#include "obs_core_wrapper.hpp"
#define DL_OPENGL "libobs-opengl.dll"
#define DL_D3D11 "libobs-d3d11.dll"

void ObsCoreWrapper::initVideo(void)
{
	int ret = obs_reset_video(&ovi);
	if (ret != OBS_VIDEO_SUCCESS) {
		if (ret == OBS_VIDEO_CURRENTLY_ACTIVE) {
			blog(LOG_WARNING, "Tried to reset when "
					  "already active");
			return;
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
	return;
}

void ObsCoreWrapper::createOBS()
{
	if (!obs_startup("en-US", nullptr, nullptr))
		throw "Couldn't create OBS";

	/* init obs graphic things */
	initVideo();

	/* load modules */
	obs_load_all_modules();
	obs_log_loaded_modules();
	obs_post_load_modules();


	blog(LOG_INFO, "Successfully init OBS core");
	return;
}

void ObsCoreWrapper::setVideoInfo(uint32_t width, uint32_t height,
				  uint32_t fpsNumerator,
				  uint32_t fpsDenominator)
{
	ovi.adapter = 0;
	ovi.base_width = width;
	ovi.base_height = height;
	ovi.fps_num = fpsNumerator;
	ovi.fps_den = fpsDenominator;
	ovi.graphics_module = DL_D3D11;
	ovi.output_format = VIDEO_FORMAT_RGBA;
	ovi.output_width = width;
	ovi.output_height = height;
}

void ObsCoreWrapper::sceneItemSetScale(obs_scene_t *scene, obs_source_t *source,
				       float scaleX, float scaleY)
{
	obs_sceneitem_t *item = NULL;
	struct vec2 scale;
	vec2_set(&scale, scaleX, scaleY);
	item = obs_scene_add((obs_scene_t *)scene, (obs_source_t *)source);
	obs_sceneitem_set_scale(item, &scale);
}
