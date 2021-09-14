#include "capture_manager.hpp"


/**
 * init capture manager
 * Params:
 *  viewHwnd: the view window handle
 *  fromHwnd: the capture source window handle
 *  fpsNum: fps numerator.   e.g.: 120000
 *  fpsDen: fps denominator. e.g.: 1001
 * 
 */
void CaptureManager::init(HWND viewHwnd, HWND fromHwnd, uint32_t fpsNum,
			  uint32_t fpsDen)
{
	RECT rc;
	GetClientRect(viewHwnd, &rc);
	this->viewHwnd = &viewHwnd;
	this->fromHwnd = &fromHwnd;
	this->fpsNum = &fpsNum;
	this->fpsDen = &fpsDen;
	/* init obs core */
	obsCore.setVideoInfo(rc.right, rc.bottom, fpsNum, fpsDen);
	obsCore.createOBS();
	/* config display */
	display.config(viewHwnd, rc.right, rc.bottom);
	/* init output source scene */
	outputScene.init("test scene");
	/* init window capture source */
	captureSource.init();
	obs_set_output_source(0, captureSource);
}

/**
 * scale as the capture source width/height proportion by output width/height
 * Params:
 *  scene: output scene
 *  source: capture source
 *  fromHwnd: capture window handle
 * 
 */
void CaptureManager::AddSceneItems()
{
	uint32_t cx = 0;
	uint32_t cy = 0;
	display.getObsDisplaySize(&cx, &cy);
	RECT rc;
	GetClientRect(*fromHwnd, &rc);
	uint32_t width = rc.right;
	uint32_t height = rc.bottom;
	float scale_x = 1.0f;
	float scale_y = 1.0f;
	if (width > cx) {
		scale_x = float(cx) / width;
	}

	if (height > cy) {
		scale_y = float(cy) / width;
	}

	float s = scale_x > scale_y ? scale_x : scale_y;
	obs_scene_add(outputScene, captureSource);
	obsCore.sceneItemSetScale(outputScene, captureSource, s, s);
	return;
}

void CaptureManager::initCapture(const char *curId)
{
	AddSceneItems();
	captureSource.changeCaptureSource(curId);
	return;
}

/**
 * start window cature
 * TODO: can add callback as param
 */
void CaptureManager::start()
{
	display.setDisplayRenderCallback(RenderWindow);
	return;
}

void CaptureManager::destroy()
{
	obs_display_destroy(display);
	obs_scene_release(outputScene);
	obs_source_release(captureSource);
	obs_set_output_source(0, nullptr);
	obs_shutdown();
	return;
}
