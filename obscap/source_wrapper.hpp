#include "base.hpp"

class SourceWrapper {
	obs_source_t *source = NULL;

public:
	inline SourceWrapper() {}
	inline SourceWrapper(obs_source_t *source) : source(source) {}
	inline ~SourceWrapper() { }
	inline operator obs_source_t *() { return source; }
	obs_source_t *getSource() { return source; }
	void setSource(obs_source_t *source) { this->source = source; }
	void destroy();
};
