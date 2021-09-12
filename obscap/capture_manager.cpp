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
CaptureManager::CaptureManager(HWND viewHwnd, HWND fromHwnd, uint32_t fpsNum,
			       uint32_t fpsDen)
{
	this->viewHwnd = viewHwnd;
	this->fromHwnd = fromHwnd;
	this->fpsNum = fpsNum;
	this->fpsDen = fpsDen;
}

void CaptureManager::init() {
	RECT rc;
	GetClientRect(this->viewHwnd, &rc);
    obsCore = ObsCoreWrapper(rc.right, rc.bottom, fpsNum, fpsDen);
	this->obsCore.createOBS();
	this->display = createDisplay(this->viewHwnd, rc.right, rc.bottom);
	this->outputScene = createOutputScene("test scene");
	this->outputSource = createOutputSourceByScene(this->outputScene);
	this->captureSource = createCaptureSource();
}

void CaptureManager::destroy() {
	//outputSource.destroy();
	//captureSource.destroy();
	//outputScene.destroy();
	//display.destroy();
	obsCore.destroyOBS();
}



/**
 * create display for output and set the output render callback
 * Params:
 *  hwnd: the view window handle
 *  width: view window widht
 *  height: view window height
 */
DisplayWrapper CaptureManager::createDisplay(HWND hwnd, uint32_t width,
					     uint32_t height)
{
	DisplayWrapper displayWrapper = DisplayWrapper(hwnd, width, height);
	return displayWrapper;
}

/**
 * create window capture obs source
 * 
 */
CaptureSourceWrapper CaptureManager::createCaptureSource()
{
	CaptureSourceWrapper captureSource = CaptureSourceWrapper();
	captureSource.init();
	return captureSource;
}

/**
 * create view output obs scene
 * Params:
 *  name: the capture window exe name to distinguish
 * 
 */
SceneWrapper CaptureManager::createOutputScene(const char *name)
{
	SceneWrapper sceneWrapper = SceneWrapper(name);
	return sceneWrapper;
}

/**
 * create view output window obs source by output scene
 * 
 */
SourceWrapper CaptureManager::createOutputSourceByScene(SceneWrapper scene)
{
	SourceWrapper sourceWrapper = SourceWrapper();
	sourceWrapper.setSource(obsCore.getSourceByScene(scene.getScene()));
	return sourceWrapper;
}

/**
 * scale as the capture source width/height proportion by output width/height
 * Params:
 *  scene: output scene
 *  source: capture source
 *  fromHwnd: capture window handle
 * 
 */
void CaptureManager::AddSceneItems(SceneWrapper scene, CaptureSourceWrapper source,
				   HWND fromHwnd)
{
	uint32_t cx = 0;
	uint32_t cy = 0;
	display.getObsDisplaySize(&cx, &cy);
	RECT rc;
	GetClientRect(fromHwnd, &rc);
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

	obsCore.sceneItemSetScale(scene.getScene(), source.getSource(), scale_x, scale_y);
	return;
}

void CaptureManager::initCapture(const char *curId)
{
	this->AddSceneItems(this->outputScene, this->captureSource, this->fromHwnd);
	this->captureSource.changeCaptureSource(curId);
	return;
}

/**
 * start window cature
 * TODO: can add callback as param
 */
void CaptureManager::start()
{
	this->display.setDisplayRenderCallback(RenderWindow);
	return;
}
