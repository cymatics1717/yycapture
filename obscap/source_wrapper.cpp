#include "source_wrapper.hpp"

void SourceWrapper::destroy() {
	obs_source_release(source);
}
