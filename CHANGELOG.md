# Changelog

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
