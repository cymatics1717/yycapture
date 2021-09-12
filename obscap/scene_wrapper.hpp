#include "base.hpp"

class SceneWrapper {
	obs_scene_t *scene = NULL;

public:
	inline SceneWrapper() {}
	SceneWrapper(const char *name);
	inline ~SceneWrapper() {}
	inline operator obs_scene_t *() { return scene; }
	inline obs_scene_t *getScene() { return scene; }
	void destroy();
};
