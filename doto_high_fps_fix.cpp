// DOTO_TARGET selects the independently mapped executable compatibility
// profile, RVAs, object layouts, configuration names, and frozen camera gate
// from the shared patch implementation included in this repository. The
// known executable hash is diagnostic; the fail-closed runtime PE/layout,
// call-relationship, and configured-hook checks decide compatibility. The
// resulting native DLL is emitted with an .asi extension and loaded by an
// external ASI loader.
#define DOTO_TARGET 1
#include "doto_high_fps_fix_impl.cpp"
