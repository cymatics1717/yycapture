#include "capture_source_wrapper.hpp"

void CaptureSourceWrapper::init() {
	/* create window source */
	obs_source_t *s = obs_source_create(
		"window_capture", "window capture source", NULL, nullptr);
	if (!s)
		throw "Couldn't create source";

	this->setSource(s);
}

void CaptureSourceWrapper::destroy() {
	obs_source_release(source);
}

void CaptureSourceWrapper::changeCaptureSource(const char *Id) {
    const char *setting = "window";
	obs_data_t *data = obs_data_create();
	obs_data_set_string(data, setting, Id);
	obs_source_update(this->getSource(), data);
    return ;
}
