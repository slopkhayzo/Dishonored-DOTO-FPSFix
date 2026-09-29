// DOTO_TARGET selects the independently mapped executable hash, RVAs, object
// layouts, configuration names, and frozen camera gate from the shared patch
// implementation included in this repository. The resulting native DLL is
// emitted with an .asi extension and loaded by an external ASI loader.
#define DOTO_TARGET 1
#include "doto_high_fps_fix_impl.cpp"
