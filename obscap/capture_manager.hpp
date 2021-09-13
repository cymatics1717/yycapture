#include "obs_core_wrapper.hpp"
#include "scene_wrapper.hpp"
#include "display_wrapper.hpp"
#include "capture_source_wrapper.hpp"
#include <windows.h>
#include "utils.hpp"

class CaptureManager {
	HWND *viewHwnd;
	HWND *fromHwnd;
	uint32_t *fpsNum;
	uint32_t *fpsDen;
	ObsCoreWrapper obsCore;
	CaptureSourceWrapper captureSource;
	SceneWrapper outputScene;
	DisplayWrapper display;

public:
	void initCapture(const char *);
	void start();
	void AddSceneItems();
	void init(HWND, HWND, uint32_t, uint32_t);
	void destroy();
};
