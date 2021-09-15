#include "scene_wrapper.hpp"

obs_source_t *SceneWrapper::getSourceByScene()
{
	return obs_scene_get_source(scene);
}

void SceneWrapper::init(const char *name)
{
	scene = obs_scene_create(name);
	if (!scene)
		throw "Couldn't create scene";
}
