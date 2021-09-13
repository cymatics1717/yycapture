#include "scene_wrapper.hpp"

void SceneWrapper::init(const char *name)
{
	scene = obs_scene_create(name);
	if (!scene)
		throw "Couldn't create scene";
}
