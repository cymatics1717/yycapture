#include "base.hpp"

class CaptureSourceWrapper {
	obs_source_t *source = NULL;

public:
	inline CaptureSourceWrapper() {}
	inline ~CaptureSourceWrapper() {}
	void changeCaptureSource(const char *);
	obs_source_t *getSource() { return source; }
	void setSource(obs_source_t *source) { this->source = source; }
	void init();
	void destroy();
};
