#include "base.hpp"

class ObsCoreWrapper {
	obs_video_info ovi = {0};

public:
	void initVideo(void);
	void createOBS(void);
	void setVideoInfo(uint32_t, uint32_t, uint32_t, uint32_t);
	void sceneItemSetScale(obs_scene_t *, obs_source_t *, float, float);
};
