#include "base.hpp"

class CaptureSourceWrapper {
	obs_source_t *source;

public:
	void changeCaptureSource(const char *);
	inline operator obs_source_t *() { return source; }
	//void setSource(obs_source_t *source) { source = source; }
	void init();
};
