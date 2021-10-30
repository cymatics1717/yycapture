#ifndef OBSCAPTURE_HPP
#define OBSCAPTURE_HPP
#include <Windows.h>

#include "obs.h"
#include <QObject>

class OBSCapture : public QObject
{
    Q_OBJECT
public:
    OBSCapture(const HWND &target, QObject *parent = nullptr);
    ~OBSCapture();

    int startGame();
    int addGameSource(HWND from);
    int addWindowSource(HWND from);
    int start();
public slots:
//    void setTextureCallback();
    int resetVideo(int fs =144);

private:
    int fps;
    int initOBS();
    obs_video_info ovi;
    obs_display_t *display;
    obs_scene_t *scene;

    HWND sink;

    std::map<HWND,obs_source_t *> sources;
};

#endif // OBSCAPTURE_HPP
