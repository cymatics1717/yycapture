#include "obs_core_wrapper.hpp"
#include "scene_wrapper.hpp"
#include "display_wrapper.hpp"
#include "source_wrapper.hpp"
#include "capture_source_wrapper.hpp"
#include <windows.h>
#include "utils.hpp"

class CaptureManager {
	HWND viewHwnd;
	HWND fromHwnd;
	uint32_t fpsNum;
	uint32_t fpsDen;
	ObsCoreWrapper obsCore;
	DisplayWrapper display;
	CaptureSourceWrapper captureSource;
	SourceWrapper outputSource;
	SceneWrapper outputScene;

public:
	CaptureManager(HWND, HWND, uint32_t, uint32_t);
    ~CaptureManager();
	void initCapture(const char *);
	void start();
	DisplayWrapper createDisplay(HWND, uint32_t, uint32_t);
	CaptureSourceWrapper createCaptureSource();
	SceneWrapper createOutputScene(const char *);
	SourceWrapper createOutputSourceByScene(SceneWrapper);
	void AddSceneItems(SceneWrapper, CaptureSourceWrapper, HWND);
    void init();
	void destroy();
};
