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

Version `1.1.0` changes delivery from the original proxy DLL to an x64 ASI
plugin. Focused tests and multi-level live gameplay passed without a major
issue for the camera, mouse, first-person root and skeleton, ordinary world
roots and skeletons, small ownerless cinematic models, and the Ultimate ASI
Loader path. See [Known limitations](#known-limitations) for paths that remain
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
- A loader-neutral `DOTOHighFPSFix.asi` plugin. An external x64 ASI loader owns
  process loading and any Windows-DLL proxy forwarding.

The patch changes temporary renderer-owned copies and palettes. It does not
increase the simulation rate or write interpolated values back to entities,
physics, animators, or source palettes.

## Requirements

- 64-bit Windows.
- The exact GOG executable listed above.
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

Keep the ASI and INI together. The plugin computes the complete SHA-256 hash of
the running executable before it installs any hooks, then validates the
individual hook bytes. On an unsupported build it leaves the patch disabled
and writes the reason to `doto-high-fps-fix.log` beside the ASI.

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
joint-pose tests, produces `build\DOTOHighFPSFix.asi`, and runs the ASI load
test from the repository path.

To repeat the ASI load test directly:

```bat
build\asi-load-test.exe build\DOTOHighFPSFix.asi
```

The test process is not the game, so the plugin must log an expected host-hash
rejection. The test also verifies that the artifact has an `.asi` extension and
does not expose the retired `DirectInput8Create` proxy export.

After building, close the game and copy `build\DOTOHighFPSFix.asi` plus
`doto-high-fps-fix.ini` into one loader-scanned directory. Keep both files in
the same directory.

To reproduce the binary release archive after building and testing, run:

```powershell
.\package-release.ps1 -Version 1.1.0
```

The canonical plugin-only ZIP and its SHA-256 sidecar are written to `dist\`.
To create a separate package containing an already downloaded official x64
Ultimate ASI Loader, provide its exact version and path:

```powershell
.\package-release.ps1 -Version 1.1.0 `
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
- Live ASI unloading is unsupported; close the game before installing or
  removing files.
- ASI loaders, ReShade, overlays, injectors, and other proxy-DLL users may
  conflict; never overwrite an existing proxy without identifying it.
- The above-120 mode requires an external limiter and in-game Triple Buffering
  to remain disabled.

## Source layout

- `doto_high_fps_fix.cpp` selects the DOTO ASI target.
- `doto_high_fps_fix_impl.cpp` contains the shared hooks, interpolation,
  diagnostics, and exact DOTO target constants.
- `joint_pose_interpolation.h` contains the tested palette math.
- `joint-pose-test.cpp` and `asi-load-test.cpp` provide native tests.
- `doto-high-fps-fix.ini` is the tested default configuration.
- `README.txt` is the plain-text end-user guide included in releases.
- `CHANGELOG.md` records public release changes.
- `third-party/` contains the license used by optional loader packaging; no
  loader binary is committed there.
- `package-release.ps1` produces the plugin-only archive and, when explicitly
  supplied a loader, a separate optional-loader archive.
