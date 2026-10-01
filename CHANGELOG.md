# Changelog

## 1.3.0 - 2026-10-01

- Added an enabled-by-default adaptive performance gate driven by fixed-step
  presentation cadence. It sheds transform interpolation first, then skeletal
  interpolation, and uses adjacent measured costs, asymmetric thresholds,
  two-window recovery, and context-refresh probes to restore features safely.
- Added `AdaptivePerformanceGate=0` as a complete opt-out that leaves every
  individually configured interpolation layer continuously eligible.
- Added a default-off bounded A/B harness with F11 controls, warm-up exclusion,
  frame-time percentiles, fixed-step cadence, and interpolation work counters.
- Deferred full render-model cloning until a correction is available and
  removed unconditional zero-initialization of the temporary pose palette.
- Added a clean twelve-segment DOTO capture across light, medium, and heavy
  *Follow the Ink* views. It showed transforms costing about 0.75-0.78 ms/frame
  in the constrained views versus about 0.59-0.39 ms/frame for skeletons, so
  the production ladder is now `All -> SkeletonsOnly -> Off`.
- Completed a focused normal-play regression through those views, including
  combat. The controller shed and restored the intended adjacent profiles,
  learned local costs, and reported no safety-counter failure.

## 1.2.0 - 2026-10-01

- Replaced DOTO's hard executable-SHA activation allowlist with a fail-closed
  structural compatibility preflight. The known GOG hash remains diagnostic;
  activation now requires the expected x64 PE image layout and section
  permissions, mapped RVAs and vtables, verified camera-call relationship, and
  exact runtime signatures for every enabled hook before any patch is written.
- Same-layout executable variants can now activate. Builds with shifted RVAs,
  changed hook code, or changed object layouts remain rejected until they are
  independently mapped and tested.
- Promoted the exact development binary after a focused in-game regression on
  the validated GOG build completed without an observed issue.

## 1.1.0 - 2026-09-29

- Changed the runtime patch from a self-loading `dinput8.dll` proxy to the
  loader-neutral x64 `DOTOHighFPSFix.asi` plugin.
- Added tested compatibility with Ultimate ASI Loader 9.7.4.
- Added a canonical plugin-only release and a separately labeled optional
  package containing the x64 Ultimate ASI Loader `dinput8.dll`.
- Added loader architecture/version checks, upstream MIT licensing and
  provenance notices, payload manifests, and SHA-256 checksums.
- Replaced the DirectInput-forwarding smoke test with direct and external ASI
  loading tests, including paths containing spaces and fail-closed host
  rejection.
- Added process-local duplicate-plugin protection without changing the
  validated camera, mouse, world-transform, first-person, skeletal, or
  cinematic interpolation behavior.

## 1.0.0 - 2026-09-28

- Initial public GOG 1.145.0.0 release using a `dinput8.dll` proxy.
- Added the above-120-FPS unlock, stable mouse input branch, renderer-camera
  prediction, first-person root correction, and supported renderer-only world,
  skeletal, and cinematic interpolation.
