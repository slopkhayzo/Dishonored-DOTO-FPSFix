# Changelog

## 1.5.1 - 2026-10-26

Ports the first-person attachment fix of the Dishonored 2 high-FPS fix
v1.5.1. Live-tested on the GOG and Steam builds. There are no new settings,
and an existing `doto-high-fps-fix.ini` from 1.5.0 can be kept.

- The held weapon is now smooth while it moves relative to the camera, for
  example while it is drawn or holstered behind the power wheel. First-person
  models were only moved with the smoothed camera, so their own motion
  relative to it, such as a weapon carried by the animated hand, still stepped
  at the simulation rate. This was barely visible at normal speed but obvious
  while time is slowed, where the fingers also seemed to snap against the
  stepped grip. That motion is now interpolated at the same presentation time
  as the hand animation. Models fixed to the camera keep exactly the previous
  correction. This is part of `FirstPersonSkeletons` and runs whenever the
  first-person hand animation is interpolated.
- Unlike Dishonored 2, DOTO draws several new short-lived first-person models
  every frame during some effects, such as Foresight. They are not
  interpolated, but they must not use up the room for the models that are, so
  a first-person track is reused 4 frames after its model was last drawn, and
  there is room for 128. Without that, a weapon drawn during or shortly after
  Foresight would have stepped as before for several seconds.
- World objects no longer pause briefly within each step of deep slow motion.
  Their extrapolation limit was a fixed 8.3 ms instead of one step of the
  slowed clock. Normal speed is unchanged.
- The log adds a periodic "First-person transform" diagnostics line.
- `camera-prediction-test.exe` adds a first-person attachment that moves
  relative to a moving camera at 60-360 FPS and simulation speeds down to 5%,
  and a burst of short-lived first-person models. Built with the previous
  track limits, that burst case fails exactly as the live game did.
- Live-tested the exact release binary through Ultimate ASI Loader 9.7.4 on
  GOG (including Foresight) and Steam, with the power wheel opened while the
  weapon was drawn or holstered. The weapon stayed smooth. Slow motion was
  detected and ended 23 times. First-person tracks, skeletal uploads, and
  world transforms had no capacity misses, skipped or failed uploads, or
  interpolation rejections, and there was no crash.

## 1.5.0 - 2026-10-23

Ports the turning and slow-motion fixes of the Dishonored 2 high-FPS fix
v1.5.0 and fixes two ways the adaptive gate kept interpolation off. Live-tested
on the GOG and Steam builds at 90 and 144 FPS caps.

- Mouse turning is now even at frame rates that are not a multiple of 120,
  such as 90 or 144 FPS. Since 1.4.0 the game turns the view once per game
  frame by the mouse input gathered since the previous one, but camera
  prediction still extrapolated that turn by simulation time, which made
  turning judder below 120 FPS while walking stayed smooth (30 Hz at 90 FPS).
  Added `Fix/RealTimeTurnPrediction`, default `1`: camera rotation follows the
  real time between game frames while position keeps the simulation-time
  prediction. Cutscenes and menus keep the previous rotation rule, which suits
  scripted cameras. **Ctrl+F10** switches between the two rules in game; F10
  is unchanged.
- Slow motion is smooth. When the game slows time it runs fewer, longer
  simulation ticks, so new states arrive less often than frames at any frame
  rate. Added `Interpolation/FullInterpolationInSlowMotion`, default `1`: while
  the engine clock is slowed, every enabled interpolation layer stays on even
  if the adaptive gate would shed it, and those frames are excluded from the
  gate's measurements.
- During slow motion every layer, the camera included, accepts the longer gap
  between simulation samples instead of treating it as a stall, and ownerless
  character models are interpolated as well. Outside slow motion the cinematic
  layers still admit only small ownerless models.
- Added `Diagnostics/TimeScaleLog`, default `1`, which logs the engine clock's
  time scales when they change and summarizes each slow-motion period.
- The DOTO clock update (GOG `0x7E730`, Steam `0x5A790`) was verified to use
  the same time-scale fields as Dishonored 2's, and the game-logic `in-game`
  state id (`1`) was read from the engine's own enum table on both builds.
- Added `camera-prediction-test.exe` and slow-motion and turn-rule cases to
  the adaptive-controller test.
- The adaptive gate no longer switches interpolation off for good after a
  level load or under a frame cap. Both live sessions of this port loaded
  straight into gameplay, and the first windows after the load (about 80 FPS
  while the level streamed in) made the gate shed every layer and learn costs
  of 2.0-4.4 ms per family that then blocked recovery. Under a 90 FPS cap,
  shedding cannot raise the frame rate, so the learned cost was zero and the
  layers stayed off. Now each level load restarts the gate with every layer on
  and no learned costs and measures only after a 5-second warm-up. A shed that
  changes the frame time by less than 2.5% is undone at once, and that layer
  is not shed again while the frame time stays within 15% of where it proved
  free.
- Live-tested the exact release binary on both builds through Ultimate ASI
  Loader 9.7.4, with caps of 90 and 144 FPS: a GOG session and a Steam
  session, each loading straight into gameplay. Each build selected its own
  profile and installed every hook. Slow motion was detected and ended 10
  times on Steam and 7 times on GOG (down to 21-35% of the native tick rate),
  with ownerless characters interpolated during it. Level-load restarts and
  undone sheds appeared as intended. Skipped and failed forced uploads,
  interpolation rejections, layout failures, same-time mutations, and
  capacity misses stayed at zero. There was no crash.

## 1.4.0 - 2026-10-20

- Added a Steam `1.145.0.0` layout profile. The February 2025 Steam
  executable is a newer-compiler rebuild with unchanged engine structures but
  moved code and global data, so v1.3.0 failed closed on it. The plugin now
  selects a GOG or Steam profile from the executable's PE image size and entry
  point and then runs the unchanged strict preflight on that profile; the GOG
  profile carries exactly the previous constants.
- Fixed the preflight rejecting ASLR-relocated executables. The Steam build
  sets `DYNAMIC_BASE`, and Windows records the actual load address in the
  mapped header, so the header may now hold either the preferred or the actual
  image base.
- Added `layout-profile-test.exe`, which maps a real executable without running
  it and validates, installs, and byte-checks every hook and trampoline on that
  private mapping. Both the GOG and Steam executables pass.
- Made mouse sensitivity frame-rate independent below 120 FPS as well. The
  previous `g_fixMouseSensibility=0` path skipped the game's frame-time
  division, so the turn per count followed each frame's tick count: stable
  above 120 FPS but 1.5x at 80 and 2x at 60 FPS. The game's default `=1` path
  is now kept and its clock-delta read is rewritten in place to use the
  simulation timer's step, which cancels the view's own simulation-step
  multiplication on every frame (ported from the Dishonored 2 high-FPS fixes).
  Sensitivity now matches the unmodified game at 120 FPS, twice the earlier
  high-FPS gain; users who raised it should lower it back to about half.
- Added `FullInterpolationInCutscenes=1`. While the game-logic stack contains
  the cinematic state, every configured interpolation layer stays enabled and
  adaptive measurement pauses, so vehicles and parented objects are not
  stepped in cutscenes that run below 120 FPS. The gate resumes its gameplay
  mode afterwards. Detection reads the `idGameLocal`-embedded
  `arkGameLogicManager` and requires the engine's own cinematic state object.
- Live-tested the exact release binary on both builds: a Steam gameplay
  session and a GOG gameplay session, covering cutscenes with nested
  conversations, level transitions, and adaptive shedding and recovery. Each
  build selected its own profile and installed every hook; cutscenes kept full
  interpolation and returned control to the adaptive gate; skipped and failed
  forced uploads, interpolation rejections, layout failures, and capacity
  misses stayed at zero.

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
