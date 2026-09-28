# Dishonored: Death of the Outsider high-FPS fix

> [!WARNING]
> This patch has been pretty much entirely been generated using AI; 
> I won't and will never claim to have enough knowledge or expertise to do this 
> kind of reverse-engineering by myself; I've tested two levels at 360hz + 
> briefly tested at 120, 144 and 240 and so far spotted 
> no major issues (on my machine ofc, if you have issues feel free to open an Issue)
> this patch currently probably only works for the latest GOG version of the game, I do have 
> the Steam version too but still have to test that, so no guarantees for now

A renderer-only high-frame-rate patch for the GOG release of *Dishonored:
Death of the Outsider*. It unlocks presentation above 120 FPS and smooths the
camera, first-person models, supported world transforms, and supported skeletal
animation while leaving simulation, physics, AI, scripts, animation events,
and audio at the game's native 120 Hz, with an additional per-frame mouse delta 
override to keep mouse input latency low.

Version `1.0.0` is the first public release. Focused tests and multi-level live
gameplay passed without a major issue for the camera, mouse, first-person root
and skeleton, ordinary world roots and skeletons, and small ownerless cinematic
models. See [Known limitations](#known-limitations) for paths that remain
unsupported or less extensively tested.

## Supported game build

- Store: GOG
- Version: `1.145.0.0`
- Executable: `Dishonored_DO.exe`
- Executable SHA-256:
  `DA7E8EB3FDFA28BF552079B37F626A009F35941D9EEB015BA61AC9BFCD850C08`

Other versions and storefront builds are rejected. Hook sites are protected by
exact byte checks in addition to the full executable hash.

The tracked repository contains patch source only. It does not contain game
assets, extracted data, reverse-engineering projects, or research dumps.

## Features

- Presentation above 120 FPS, controlled by an external frame limiter.
- Frame-rate-independent mouse-sensitivity branch.
- Renderer-camera prediction, with an F10 comparison toggle.
- Camera-aligned first-person root correction.
- Independently gated interpolation for supported world transforms,
  first-person skeletons, world skeletons, and small cinematic models.
- Fixed-capacity histories, identity and discontinuity checks, exact-build
  guards, and fail-closed behavior.
- A `dinput8.dll` proxy that forwards `DirectInput8Create` to the system DLL.

The patch changes temporary renderer-owned copies and palettes. It does not
increase the simulation rate or write interpolated values back to entities,
physics, animators, or source palettes.

## Requirements

- 64-bit Windows.
- The exact GOG executable listed above.
- Visual Studio 2022 Build Tools (or Visual Studio) with **Desktop development
  with C++**, the x64 MSVC toolchain, and a Windows SDK.
- A safe external FPS limiter such as RTSS or a driver-level limiter.

Disable the game's swap-chain **Triple Buffering** option when using the
above-120 unlock. Always set a sensible external cap before launching the game.

## Install a release

Download and extract `DOTO-HighFPS-Fix-v1.0.0.zip`. Close the game, then copy
these two files into the game root beside `Dishonored_DO.exe`:

- `dinput8.dll`
- `doto-high-fps-fix.ini`

The DLL computes the complete SHA-256 hash of the running executable before it
installs any hooks, then validates the individual hook bytes. On an unsupported
build it forwards DirectInput normally but leaves the patch disabled and writes
the reason to `doto-high-fps-fix.log`.

If the game root already contains a `dinput8.dll`, do not overwrite it until
you know which mod installed it and have backed it up. This patch does not
currently provide proxy chaining.

## Build from source

Install Visual Studio 2022 or the current Visual Studio Build Tools with the Desktop development with C++ workload, then run from a Command Prompt:

```bat
build-msvc.cmd
```

The script locates the Visual Studio x64 toolchain, builds and runs the native
joint-pose tests, and writes the proxy and smoke test to `build\`.

Then verify proxy loading and DirectInput forwarding:

```bat
build\proxy-smoke-test.exe build\dinput8.dll
```

The smoke-test process is not the game, so the proxy will log an expected host
executable hash rejection while still testing its exported function and system
DLL forwarding.

After building, close the game and copy `build\dinput8.dll` plus
`doto-high-fps-fix.ini` into the game root beside `Dishonored_DO.exe`.

To reproduce the binary release archive after building and testing, run:

```powershell
.\package-release.ps1 -Version 1.0.0
```

The ZIP and its SHA-256 sidecar are written to `dist\`.
The release archive contains a plain-text `README.txt` for end users; this
Markdown file remains the GitHub/source documentation.

## Configure and use

Edit `doto-high-fps-fix.ini` in the game directory while the game is closed.
The tested renderer interpolation features are enabled by default in `v1.0.0`.
Set an external FPS cap appropriate for the display and disable in-game Triple
Buffering before launching. Every layer remains independently configurable; if
troubleshooting, disable them one at a time in this order:
`CinematicSkeletons`, `CinematicTransforms`, `FirstPersonSkeletons`,
`WorldSkeletons`, `WorldTransforms`, then `StabilizeFirstPersonHands`.

`ShadowTransforms` is reserved and forcibly disabled in this DOTO build. F10
toggles the camera prediction path for an in-game comparison. The runtime log
is written beside the DLL as `doto-high-fps-fix.log`; diagnostics are bounded
and useful when reporting a problem.

## Uninstall

Close the game and delete `dinput8.dll` and `doto-high-fps-fix.ini` from the
game root. The generated `doto-high-fps-fix.log` can also be deleted. If you are
not certain the DLL belongs to this patch, compare it against `SHA256SUMS.txt`
from the release before deleting it.

## Current validation status

Focused live testing has passed for:

- above-120 presentation, stable mouse input, camera prediction, and
  first-person root correction;
- thrown props, bottles, doors, and other eligible world roots;
- ordinary world skeletal animation, including observed combat and ragdolls;
- first-person hands and sword animation;
- the first-level railway-cabin cinematic root and ownerless palette paths;
- forced palette uploads on presentation frames between simulation ticks;
- native joint-pose tests, `/W4` builds, and DirectInput forwarding tests.

The first release has not exhaustively covered:

- the complete target-rate and variable-refresh cadence matrix;
- the full gameplay, lifecycle, load/transition, and long-session matrix;
- broader secondary-view and temporal-effect validation;
- every overlay/injector combination and long-duration playthrough.

## Known limitations

- GOG `1.145.0.0` only.
- Shadow transform interpolation is unsupported and forced off.
- Cloth and independently simulated dynamic vertices are not interpolated.
- Reflections, portals, motion blur, and other secondary/temporal paths still
  need broader validation.
- Live DLL unloading is unsupported; close the game before installing or
  removing files.
- ReShade, overlays, injectors, and other `dinput8.dll` users may conflict.
- The above-120 mode requires an external limiter and in-game Triple Buffering
  to remain disabled.

## Source layout

- `dinput8_proxy.cpp` selects the DOTO target.
- `dinput8_proxy_impl.cpp` contains the shared proxy, hooks, interpolation,
  diagnostics, and exact DOTO target constants.
- `joint_pose_interpolation.h` contains the tested palette math.
- `joint-pose-test.cpp` and `proxy-smoke-test.cpp` provide native tests.
- `doto-high-fps-fix.ini` is the tested default configuration.
- `README.txt` is the plain-text guide included in binary releases.
- `package-release.ps1` produces the tested binary archive and checksums.
