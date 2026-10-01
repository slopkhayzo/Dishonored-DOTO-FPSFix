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

A renderer-only high-frame-rate patch for the GOG release of *Dishonored:
Death of the Outsider*. It unlocks presentation above 120 FPS and smooths the
camera, first-person models, supported world transforms, and supported skeletal
animation while leaving simulation, physics, AI, scripts, animation events,
and audio at the game's native 120 Hz, with an additional per-frame mouse delta 
override to keep mouse input latency low.

Version `1.3.0` adds an adaptive performance gate that sheds transform
interpolation before skeletal interpolation as useful 120 Hz headroom falls,
then restores each layer after measured recovery. Version `1.2.0` replaced the
hard executable-SHA activation allowlist with a fail-closed structural
compatibility preflight, and version `1.1.0` changed delivery from the original
proxy DLL to an x64 ASI plugin. Focused tests and multi-level live gameplay
passed without a major issue for the camera, mouse, first-person root and
skeleton, ordinary world roots and skeletons, small ownerless cinematic
models, adaptive transitions, and the Ultimate ASI Loader path. See
[Known limitations](#known-limitations) for paths that remain unsupported or
less extensively tested.

## Validated game build

- Store: GOG
- Version: `1.145.0.0`
- Executable: `Dishonored_DO.exe`
- Executable SHA-256:
  `DA7E8EB3FDFA28BF552079B37F626A009F35941D9EEB015BA61AC9BFCD850C08`

This is the only build that has been live-tested. Its SHA-256 is retained as a
diagnostic identity, but it is no longer a hard activation allowlist. An
unrecognized `Dishonored_DO.exe` is accepted only when it matches the mapped
x64 PE image base, image size, entry point, section permissions and RVAs; the
camera call still resolves to the mapped copy routine; and every enabled hook
has its verified in-memory prologue. Per-object vtable, size, field, palette,
and identity checks continue to fail closed while the game runs. Executables
whose code or data layout has shifted still require a separately mapped
compatibility profile.

The tracked repository contains patch source only. It does not contain game
assets, extracted data, reverse-engineering projects, or research dumps.

## Features

- Presentation above 120 FPS, controlled by an external frame limiter.
- Frame-rate-independent mouse-sensitivity branch.
- Renderer-camera prediction, with an F10 comparison toggle.
- Camera-aligned first-person root correction.
- Independently gated interpolation for supported world transforms,
  first-person skeletons, world skeletons, and small cinematic models.
- An adaptive fixed-step cadence controller that sheds transform interpolation,
  then skeletal interpolation, when useful 120 Hz headroom is unavailable and
  restores them after sustained measured recovery.
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
- `Dishonored_DO.exe` matching the mapped compatibility profile above. The
  listed GOG build remains the only live-tested executable.
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
joint-pose and adaptive-controller tests, produces
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

After building, close the game and copy `build\DOTOHighFPSFix.asi` plus
`doto-high-fps-fix.ini` into one loader-scanned directory. Keep both files in
the same directory.

To reproduce the binary release archive after building and testing, run:

```powershell
.\package-release.ps1 -Version 1.3.0
```

The canonical plugin-only ZIP and its SHA-256 sidecar are written to `dist\`.
To create a separate package containing an already downloaded official x64
Ultimate ASI Loader, provide its exact version and path:

```powershell
.\package-release.ps1 -Version 1.3.0 `
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

`AdaptivePerformanceGate=1` monitors presentation cadence against the fixed
120 Hz simulation clock in 180-tick windows. It starts with every configured
interpolation layer eligible. Below 130 FPS it sheds transform interpolation;
if the skeletal-only profile remains below 125 FPS it sheds skeletons too.
Recovery uses the most recently observed local cost for each family and needs
two consecutive windows of headroom. A substantially lighter scene can trigger
one controlled cost-refresh probe so an old heavy-scene estimate cannot leave
features disabled indefinitely. The controller never changes the camera,
first-person root correction, input path, FPS unlock, simulation tick, or INI.
Set `AdaptivePerformanceGate=0` to keep every individually configured
interpolation layer continuously eligible.

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

The first release has not exhaustively covered:

- the complete target-rate and variable-refresh cadence matrix;
- the full gameplay, lifecycle, load/transition, and long-session matrix;
- broader secondary-view and temporal-effect validation;
- every overlay/injector combination and long-duration playthrough.

## Known limitations

- Only GOG `1.145.0.0` has been live-tested. Same-layout executables may pass
  the structural preflight, but shifted RVAs/layouts remain incompatible until
  separately mapped and tested.
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

## Source layout

- `doto_high_fps_fix.cpp` selects the DOTO ASI target.
- `doto_high_fps_fix_impl.cpp` contains the shared hooks, interpolation,
  diagnostics, and exact DOTO target constants.
- `joint_pose_interpolation.h` contains the tested palette math.
- `joint-pose-test.cpp`, `adaptive-controller-test.cpp`, and
  `asi-load-test.cpp` provide native tests.
- `doto-high-fps-fix.ini` is the tested default configuration.
- `README.txt` is the plain-text end-user guide included in releases.
- `CHANGELOG.md` records public release changes.
- `third-party/` contains the license used by optional loader packaging; no
  loader binary is committed there.
- `package-release.ps1` produces the plugin-only archive and, when explicitly
  supplied a loader, a separate optional-loader archive.
