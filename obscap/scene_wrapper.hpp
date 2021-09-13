#include "base.hpp"

class SceneWrapper {
	obs_scene_t *scene = NULL;

public:
	inline operator obs_scene_t *() { return scene; }
	void init(const char *name);
};
