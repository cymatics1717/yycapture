#ifndef OBSCAPTURE_HPP
#define OBSCAPTURE_HPP
#include <Windows.h>
#include "obs.h"

#include <QObject>

struct preview_output {
    bool enabled;
    obs_source_t *current_source;
    obs_output_t *output;

    video_t *video_queue;
    gs_texrender_t *texrender;
    gs_stagesurf_t *stagesurface;
    uint8_t *video_data;
    uint32_t video_linesize;

    obs_video_info ovi;
};

class OBSCapture : public QObject
{
    Q_OBJECT
public:
    OBSCapture(const HWND &target, const HWND &_preview, QObject *parent = nullptr);
    ~OBSCapture();

    preview_output context;

public slots:
    int initOBS();
    int start();

signals:


private:
    int ResetVideo();

    obs_display_t *display;
    obs_scene_t *scene;

    HWND from;
    HWND sink;
    obs_source_t *source;
};

#endif // OBSCAPTURE_HPP
