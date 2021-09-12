#include "scene_wrapper.hpp"

SceneWrapper::SceneWrapper(const char *name) {
    obs_scene_t *_scene = obs_scene_create(name);
	if (!_scene)
		throw "Couldn't create scene";

	scene = _scene;
}

void SceneWrapper::destroy() {
	obs_scene_release(scene);
}
