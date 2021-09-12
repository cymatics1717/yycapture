#include "base.hpp"

class ObsCoreWrapper {
	obs_video_info ovi = {0};

public:
    ObsCoreWrapper() {}
	ObsCoreWrapper(uint32_t, uint32_t, uint32_t, uint32_t);
    ~ObsCoreWrapper(){}
    void initVideo(void);
    void createOBS(void);
    void destroyOBS(void);
    obs_source_t *getSourceByScene(obs_scene_t *);
    void sceneItemSetScale(obs_scene_t *, obs_source_t *, float, float);
};
