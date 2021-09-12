#include "base.hpp"

class SceneWrapper {
	obs_scene_t *scene = NULL;

public:
    SceneWrapper() {}
	SceneWrapper(const char *name);
    ~SceneWrapper() {}
    operator obs_scene_t *() { return scene; }
    obs_scene_t *getScene() { return scene; }
	void destroy();
};
