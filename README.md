# Dishonored: Death of the Outsider high-FPS fix

> [!WARNING]
> This patch has been pretty much entirely been generated using AI; 
> I won't and will never claim to have enough knowledge or expertise to do this 
> kind of reverse-engineering by myself; I've tested two levels at 360hz + 
> briefly tested at 120, 144 and 240 and so far spotted 
> no major issues (on my machine ofc, if you have issues feel free to open an Issue)
> this patch currently probably only works for the latest GOG version of the game, I do have 
> the Steam version too but still have to test that, so no guarantees for now
> If you're interested and want more (human generated) info, I have a blog post [here](https://slop-blog.enkhayzomachines.net/posts/dishonored-doto-high-fps-fix) :)

A renderer-only high-frame-rate patch for the GOG and Steam releases of
*Dishonored: Death of the Outsider*. It unlocks presentation above 120 FPS and smooths the
camera, first-person models, supported world transforms, and supported skeletal
animation while leaving simulation, physics, AI, scripts, animation events,
and audio at the game's native 120 Hz, with an additional per-frame mouse delta 
override to keep mouse input latency low.

Version `1.5.0` keeps mouse turning even at frame rates that are not a
multiple of 120, smooths slow motion, and stops the adaptive performance gate
from leaving interpolation off after a level load or under a frame cap.
Version `1.4.0` added the Steam build, made mouse sensitivity frame-rate
independent below 120 FPS as well, and kept full interpolation during
cutscenes. Version `1.3.0` added an adaptive performance gate that sheds
transform interpolation before skeletal interpolation as useful 120 Hz
headroom falls, then restores each layer after measured recovery. Version
`1.2.0` replaced the
hard executable-SHA activation allowlist with a fail-closed structural
compatibility preflight, and version `1.1.0` changed delivery from the original
proxy DLL to an x64 ASI plugin. Focused tests and multi-level live gameplay
passed without a major issue for the camera, mouse, first-person root and
skeleton, ordinary world roots and skeletons, small ownerless cinematic
models, adaptive transitions, and the Ultimate ASI Loader path. See
[Known limitations](#known-limitations) for paths that remain unsupported or
less extensively tested.

## Supported game builds

| Store | Version | `Dishonored_DO.exe` SHA-256 | Status |
| --- | --- | --- | --- |
| GOG | `1.145.0.0` | `DA7E8EB3FDFA28BF552079B37F626A009F35941D9EEB015BA61AC9BFCD850C08` | Live-tested across multiple levels |
| Steam | `1.145.0.0` (February 2025 rebuild) | `BE73D979FDB5A6EACBCD385944C6060B97C997D06AB9D72FB39F275A866B955D` | Live-tested in `1.4.0` and `1.5.0` (shorter coverage than GOG) |

The Steam executable is a newer-compiler rebuild of the same game version: its
engine structures are unchanged, but every code and global-data address moved
and it is loaded with ASLR. The plugin therefore carries one layout profile per
build. It selects the profile from the executable's PE image size and entry
point, then requires that profile's section permissions and RVAs, camera-call
relationship, vtables, and the exact in-memory prologue of every enabled hook
before writing any patch. The SHA-256 values are logged as diagnostic
identities, not as an activation allowlist. Per-object vtable, size, field,
palette, and identity checks continue to fail closed while the game runs.
Executables whose code or data layout differs from both profiles still need a
separately mapped profile.

The tracked repository contains patch source only. It does not contain game
assets, extracted data, reverse-engineering projects, or research dumps.

## Features

- Presentation above 120 FPS, controlled by an external frame limiter.
- Frame-rate-independent mouse sensitivity above and below 120 FPS: the game's
  own look scaling divides by the simulation time step instead of the real
  frame time.
- Renderer-camera prediction, with an F10 comparison toggle.
- Even mouse turning at frame rates that are not a multiple of 120, such as
  90 or 144 FPS.
- Camera-aligned first-person root correction.
- Independently gated interpolation for supported world transforms,
  first-person skeletons, world skeletons, and small cinematic models.
- An adaptive fixed-step cadence controller that sheds transform interpolation,
  then skeletal interpolation, when useful 120 Hz headroom is unavailable and
  restores them after sustained measured recovery.
- Smooth slow motion: while the game slows time, every enabled layer stays on
  and accepts the longer gaps between simulation samples.
- A development-only bounded A/B harness for comparing All, Transforms only,
  Skeletons only, and Off profiles in one session.
- Fixed-capacity histories, identity and discontinuity checks, structural
  compatibility guards, and fail-closed behavior.
- A loader-neutral `DOTOHighFPSFix.asi` plugin. An external x64 ASI loader owns
  process loading and any Windows-DLL proxy forwarding.

The patch changes temporary renderer-owned copies and palettes. It does not
increase the simulation rate or write interpolated values back to entities,
physics, animators, or source palettes.

## Requirements

- 64-bit Windows.
- `Dishonored_DO.exe` matching one of the mapped profiles above. The GOG build
  remains the only one live-tested in gameplay.
- A compatible x64 ASI loader. The initial compatibility target is
  [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader).
- Visual Studio 2022 Build Tools (or Visual Studio) with **Desktop development
  with C++**, the x64 MSVC toolchain, and a Windows SDK.
- A safe external FPS limiter such as RTSS or a driver-level limiter.

Disable the game's swap-chain **Triple Buffering** option when using the
above-120 unlock. Always set a sensible external cap before launching the game.

## Install a release

Download and extract the release. Close the game before changing the plugin or
loader.

For a plugin-only release, first install a compatible x64 ASI loader according
to its own documentation. Then copy these files to a directory scanned by that
loader, such as the game root, `scripts`, or `plugins`:

- `DOTOHighFPSFix.asi`
- `doto-high-fps-fix.ini`

Keep the ASI and INI together. Before writing any hook, the plugin checks the
running executable's x64 PE layout, mapped correction RVAs and section
permissions, camera-call relationship, and the exact in-memory prologue of
every enabled hook. The known GOG SHA-256 is logged as diagnostic evidence but
does not decide compatibility. On an incompatible build the plugin leaves the
patch disabled and writes the failed invariant to `doto-high-fps-fix.log`
beside the ASI.

Some releases may additionally provide a clearly labeled archive containing
Ultimate ASI Loader as `dinput8.dll`. Its MIT license and provenance notice are
included in that archive. If the game already contains `dinput8.dll` or another
loader/proxy, do not copy the bundled loader over it. Install only the ASI and
INI after confirming the existing loader is compatible. Never combine or
replace proxy DLLs merely because their filenames match.

## Build from source

Install Visual Studio 2022 or the current Visual Studio Build Tools with the Desktop development with C++ workload, then run from a Command Prompt:

```bat
build-msvc.cmd
```

The script locates the Visual Studio x64 toolchain, builds and runs the native
joint-pose, adaptive-controller, and camera-prediction/slow-motion tests,
produces
`build\DOTOHighFPSFix.asi`, and runs the ASI load test from the repository
path.

To repeat the ASI load test directly:

```bat
build\asi-load-test.exe build\DOTOHighFPSFix.asi
```

The test process is not the game, so the plugin must log an expected host
identity/PE-layout rejection. The test also verifies that the artifact has an
`.asi` extension and does not expose the retired `DirectInput8Create` proxy
export.

The script also builds `build\layout-profile-test\layout-profile-test.exe`.
It maps a real `Dishonored_DO.exe` into the test process without running it,
then runs the plugin's own profile selection, strict validation, and hook
installation against that private mapping and checks every patched entry and
trampoline. The executable on disk is never modified. To run it from the build
script, set the paths of the executables you have before building:

```bat
set "DOTO_GOG_EXE=C:\path\to\GOG\Dishonored_DO.exe"
set "DOTO_STEAM_EXE=C:\path\to\Steam\Dishonored_DO.exe"
build-msvc.cmd
```

After building, close the game and copy `build\DOTOHighFPSFix.asi` plus
`doto-high-fps-fix.ini` into one loader-scanned directory. Keep both files in
the same directory.

To reproduce the binary release archive after building and testing, run:

```powershell
.\package-release.ps1 -Version 1.5.0
```

The canonical plugin-only ZIP and its SHA-256 sidecar are written to `dist\`.
To create a separate package containing an already downloaded official x64
Ultimate ASI Loader, provide its exact version and path:

```powershell
.\package-release.ps1 -Version 1.5.0 `
  -AsiLoaderPath 'C:\path\to\dinput8.dll' `
  -AsiLoaderVersion '9.7.4'
```

The script validates that the supplied loader is an x64 PE file, records its
hash and upstream version, and includes its MIT notice. It never downloads a
loader or replaces a game file.

## Configure and use

Edit the `doto-high-fps-fix.ini` beside `DOTOHighFPSFix.asi` while the game is
closed.
The tested renderer interpolation features are enabled by default in `v1.0.0`.
Set an external FPS cap appropriate for the display and disable in-game Triple
Buffering before launching. Every layer remains independently configurable; if
troubleshooting, disable them one at a time in this order:
`CinematicSkeletons`, `CinematicTransforms`, `FirstPersonSkeletons`,
`WorldSkeletons`, `WorldTransforms`, then `StabilizeFirstPersonHands`.

`ShadowTransforms` is reserved and forcibly disabled in this DOTO build. F10
toggles the camera prediction path for an in-game comparison. The runtime log
is written beside the ASI as `doto-high-fps-fix.log`; diagnostics are bounded
and useful when reporting a problem.

`RealTimeTurnPrediction=1` (with `StabilizeCamera=1`) predicts camera rotation
from the real time since the game last applied mouse look, while position
keeps the simulation-time prediction. With the mouse fix below, the game turns
the view once per game frame by the mouse input gathered since the previous
one, so extrapolating that turn by simulation time made turning judder at
frame rates that are not a multiple of 120, such as 90 or 144 FPS (a 30 Hz
judder at 90 FPS), while walking stayed smooth. Cutscenes and menus keep the
simulation-time rotation rule, which suits scripted cameras. **Ctrl+F10**
switches camera rotation between the two rules for an A/B comparison of
turning; F10 alone is unchanged. Set it to `0` to always use the
simulation-time rule.

`StabilizeMouseSensitivity=1` keeps the game's default
`g_fixMouseSensibility=1` look scaling, which divides the look input by a frame
delta, and makes it read the simulation time step instead of the real frame
time. The view later multiplies by the same simulation step, so the two cancel
and the turn per mouse count no longer depends on the frame rate or on how many
120 Hz ticks a frame contains. Earlier versions selected
`g_fixMouseSensibility=0` instead, which was stable above 120 FPS but turned
1.5x faster at 80 FPS and 2x faster at 60 FPS. Sensitivity now matches the
unmodified game at its normal 120 FPS cap, twice the gain of those earlier
versions; if you raised the in-game sensitivity for them, lower it back to
about half.

`AdaptivePerformanceGate=1` monitors presentation cadence against the fixed
120 Hz simulation clock in 180-tick windows. It starts with every configured
interpolation layer eligible. Below 130 FPS it sheds transform interpolation;
if the skeletal-only profile remains below 125 FPS it sheds skeletons too.
A shed that does not make frames measurably faster, as under an external
frame cap, only costs smoothness, so it is undone at once and that layer stays
on while the frame rate stays in the same range. Each level load restarts the
controller with every layer on and without learned costs, and it measures the
new level only after a 5-second warm-up, so the load's streaming hitches are
not mistaken for interpolation cost.
Recovery uses the most recently observed local cost for each family and needs
two consecutive windows of headroom. A substantially lighter scene can trigger
one controlled cost-refresh probe so an old heavy-scene estimate cannot leave
features disabled indefinitely. The controller never changes the camera,
first-person root correction, input path, FPS unlock, simulation tick, or INI.
Set `AdaptivePerformanceGate=0` to keep every individually configured
interpolation layer continuously eligible.

`FullInterpolationInCutscenes=1` (the default) exempts cutscenes from that
shedding. While the game's cinematic state is active, such as during vehicle
rides, every configured layer stays enabled even if the frame rate falls
below 120 FPS, so vehicles and objects attached to them are not stepped, and
cutscene frames are left out of the controller's measurements. Afterwards the
controller resumes the mode it had chosen for gameplay. The log records each
game-state change and the start and end of each cutscene. Set it to `0` to let
the controller treat cutscenes like gameplay.

`FullInterpolationInSlowMotion=1` (the default) does the same while the game
slows time. Slow motion makes the game run fewer, longer simulation ticks, so
new states arrive less often than frames at any frame rate; that is what the
interpolation layers smooth, but below about 130 FPS the controller would
otherwise shed them. The plugin reads the slow-down directly from
the engine clock, which slows two of its timers together, and logs each
detected period. Set it to `0` to let the controller treat slow motion like
normal play.

Two slow-motion adjustments apply regardless of that setting. Every layer, the
camera included, accepts the longer gap between simulation samples instead of
treating it as a stall. Ownerless skinned character models are also
interpolated while time is slowed; outside slow motion the cinematic layers
admit only small ownerless models of at most 64 joints.

`TimeScaleLog=1` logs the engine clock's time scales whenever they change and
summarizes each detected slow-motion period, a few lines per event. Set it to
`0` to omit these lines.

The production order is based on a same-session DOTO capture in three
*Follow the Ink* views. Transforms were effectively free in the light view but
cost about 0.75-0.78 ms/frame in the medium and heavy views, while skeletons
cost about 0.59 and 0.39 ms/frame there. The resulting production ladder is
**All -> Skeletons only -> Off**. To repeat the comparison, set
`Diagnostics/InterpolationABProbe=1`; this suspends adaptation:

- **Ctrl+F11** starts or stops a segment. It drops the first two seconds and
  reports FPS, frame-time percentiles, fixed-step cadence, and work counters.
- **Alt+F11** cycles All, Transforms only, Skeletons only, and Off.
- **F11** toggles all configured interpolation against Off.
- **Shift+F11** toggles only first-person root stabilization.

F11 or Alt+F11 also establishes a process-lifetime manual override when the
probe is off. Restart to return control to the adaptive gate. Keep
`InterpolationABProbe=0` for ordinary play.

## Uninstall

Close the game and delete `DOTOHighFPSFix.asi` and the adjacent
`doto-high-fps-fix.ini`. The generated `doto-high-fps-fix.log` can also be
deleted.

Do not remove an external ASI loader if another mod may use it. If this mod's
optional loader package installed `dinput8.dll`, compare it with the packaged
`SHA256SUMS.txt` and check for other ASI plugins before deciding whether to
remove that shared loader.

## Current validation status

Focused live testing has passed for:

- above-120 presentation, stable mouse input, camera prediction, and
  first-person root correction;
- thrown props, bottles, doors, and other eligible world roots;
- ordinary world skeletal animation, including observed combat and ragdolls;
- first-person hands and sword animation;
- the first-level railway-cabin cinematic root and ownerless palette paths;
- forced palette uploads on presentation frames between simulation ticks;
- native joint-pose tests and `/W4` builds for the historical proxy release.

The ASI build, direct unsupported-host load test, automated discovery test, and
focused in-game run through the official x64 Ultimate ASI Loader `9.7.4` pass.
The installed ASI, loader, INI, and supported executable hashes matched the
packaged inputs. At the final frame-39000 diagnostic, forced-upload failures,
skipped uploads, interpolation rejections, same-time mutations, layout
failures, and capacity misses remained at zero.

The exact `1.2.0` structural-compatibility binary also passed a focused
in-game regression on the validated GOG build without an observed issue.

The `1.3.0` adaptive controller, A/B harness, and deferred hot-path work pass
the native `/W4`, joint-pose, adaptive-controller, and unsupported-host load
tests. The four-profile DOTO capture completed without a safety-counter failure
and established the transform-first shedding order. A subsequent normal-play
regression through the three test scenes, including combat, exercised shedding,
cost learning, and recovery without an observed safety failure or gameplay
issue.

The exact `1.4.0` binary passed `/W4`, joint-pose, adaptive-controller
(including fake-image cutscene cases), unsupported-host load, and offline
GOG/Steam layout-profile tests, then live gameplay sessions on both the Steam
and GOG builds. Each build selected its own profile and installed the camera,
mouse-look, GPU-copy, and skeletal patches; several cutscenes, including
nested conversations and level transitions, kept full interpolation and handed
control back to the adaptive gate; skipped and failed forced uploads,
interpolation rejections, layout failures, and capacity misses stayed at zero.

The `1.5.0` turn prediction, slow-motion handling, and adaptive-gate changes
pass `/W4`, the camera-prediction/slow-motion test, the adaptive-controller
test (including slow-motion, turn-rule, frame-cap, and level-load cases), and
the offline GOG/Steam layout tests. At 90 FPS the modeled mouse-turn error
falls from 0.360 frames RMS under the previous rule to 0.000, walking is
unchanged, and the previous rule, still used for cutscenes and menus, is
bit-identical to `1.4.0`. A development build first handled 19 slow-motion
periods on GOG. The exact release binary was then played on the GOG and Steam
builds at 90 and 144 FPS caps. Slow motion was detected and ended 17 times
(21-35% of the native tick rate), and ownerless characters were interpolated
during it. The gate restarted at the level load and undid sheds that did not
raise the capped frame rate. Every safety counter stayed clean, and there was
no crash.

The releases have not exhaustively covered:

- the complete target-rate and variable-refresh cadence matrix;
- the full gameplay, lifecycle, load/transition, and long-session matrix;
- broader secondary-view and temporal-effect validation;
- every overlay/injector combination and long-duration playthrough.

## Known limitations

- Only GOG and Steam `1.145.0.0` are supported; Steam has had a shorter live
  test than GOG. Other executables remain incompatible unless their layout
  matches one of the mapped profiles.
- Shadow transform interpolation is unsupported and forced off.
- Cloth and independently simulated dynamic vertices are not interpolated.
- Reflections, portals, motion blur, and other secondary/temporal paths still
  need broader validation.
- Live ASI unloading is unsupported; close the game before installing or
  removing files.
- ASI loaders, ReShade, overlays, injectors, and other proxy-DLL users may
  conflict; never overwrite an existing proxy without identifying it.
- The above-120 mode requires an external limiter and in-game Triple Buffering
  to remain disabled.
- The transform-first adaptive ladder is based on three repeatable second-level
  views; broader scene and long-session coverage remains pending. Use
  `AdaptivePerformanceGate=0` if its decisions are not useful on your system.
- Below about 150 FPS, for example under a 90 or 144 FPS cap, the adaptive gate
  cannot win back a layer within the same level once it has shed that layer
  for a measured cost. The same applies when a kill cam or cutscene
  interrupted the measurement. The layer returns at the next level load or
  reload. Set `AdaptivePerformanceGate=0` to keep every layer on at a fixed
  cap.

## Source layout

- `doto_high_fps_fix.cpp` selects the DOTO ASI target.
- `doto_high_fps_fix_impl.cpp` contains the shared hooks, interpolation,
  diagnostics, and exact DOTO target constants.
- `joint_pose_interpolation.h` contains the tested palette math.
- `joint-pose-test.cpp`, `adaptive-controller-test.cpp`,
  `camera-prediction-test.cpp`, `asi-load-test.cpp`, and
  `layout-profile-test.cpp` provide native tests.
- `doto-high-fps-fix.ini` is the tested default configuration.
- `README.txt` is the plain-text end-user guide included in releases.
- `CHANGELOG.md` records public release changes.
- `third-party/` contains the license used by optional loader packaging; no
  loader binary is committed there.
- `package-release.ps1` produces the plugin-only archive and, when explicitly
  supplied a loader, a separate optional-loader archive.
