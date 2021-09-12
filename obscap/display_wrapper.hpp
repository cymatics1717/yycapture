#include "base.hpp"
#include <windows.h>

class DisplayWrapper {
	obs_display_t *display = NULL;

public:
    DisplayWrapper() {}
    DisplayWrapper(obs_display_t *display) : display(display) {}
	DisplayWrapper(HWND, uint32_t, uint32_t);
    ~DisplayWrapper() { }
    operator obs_display_t *() { return display; }
	void getObsDisplaySize(uint32_t *, uint32_t *);
	void setDisplayRenderCallback(void (*callback)(void *, uint32_t, uint32_t));
	void destroy();
};

void RenderWindow(void *, uint32_t, uint32_t);
