Dishonored: Death of the Outsider high-FPS fix v1.5.1
=====================================================

IMPORTANT
---------

This patch was generated almost entirely with AI assistance. The author does
not claim reverse-engineering expertise. It has received focused testing at
120, 144, 240, and 360 Hz without a major observed issue, but it has not been
tested across every system, level, display mode, or mod combination.


WHAT IT DOES
------------

This is a renderer-only high-frame-rate patch for the GOG and Steam releases
of Dishonored: Death of the Outsider. It allows presentation above 120 FPS,
keeps mouse sensitivity independent of the frame rate, and smooths the
camera, first-person models, supported world transforms, supported skeletal
animation, and small cinematic models.

Simulation, physics, AI, scripts, animation events, and audio remain at the
game's native 120 Hz. The patch changes temporary renderer-owned copies and
palettes rather than writing interpolated values back to game entities.


SUPPORTED GAME BUILDS
---------------------

Store:       GOG
Version:     1.145.0.0
Executable:  Dishonored_DO.exe
SHA-256:     DA7E8EB3FDFA28BF552079B37F626A009F35941D9EEB015BA61AC9BFCD850C08

Store:       Steam (February 2025 update)
Version:     1.145.0.0
Executable:  Dishonored_DO.exe
SHA-256:     BE73D979FDB5A6EACBCD385944C6060B97C997D06AB9D72FB39F275A866B955D

Both builds have been live-tested. The Steam executable is a rebuild of the
same game version with every code address moved, so the patch carries a
separate layout profile for each build and chooses it automatically.

The SHA-256 values are diagnostic, not a hard activation allowlist. Before
writing any change, the patch requires the chosen profile's x64 PE image
layout, mapped addresses and section permissions, camera call relationship,
and every enabled hook signature to match. Executables that match neither
profile are rejected and leave the game unmodified. Runtime object and
palette checks remain fail-closed.


REQUIREMENTS
------------

- 64-bit Windows.
- One of the Dishonored_DO.exe builds listed above.
- A compatible x64 ASI loader.
- A safe external FPS limiter such as RTSS or a driver-level limiter.

Disable the game's Triple Buffering option when unlocking above 120 FPS. Set a
sensible external frame-rate cap before launching the game.


INSTALLATION: PACKAGE WITH ULTIMATE ASI LOADER
----------------------------------------------

Use the release whose filename ends with:

  -with-Ultimate-ASI-Loader.zip

1. Close the game.
2. If the game directory already contains dinput8.dll, read the note below
   first.
3. Extract the ZIP directly into the game directory, beside
   Dishonored_DO.exe. The files are at the top level of the ZIP, so there is
   no folder to open or move.

The three important runtime files are:

  dinput8.dll              Ultimate ASI Loader 9.7.4
  DOTOHighFPSFix.asi       The high-FPS fix
  doto-high-fps-fix.ini    Configuration

If the game directory already contains dinput8.dll, do not overwrite it until
you know which program installed it and have backed it up. If it is already a
compatible x64 ASI loader, install only DOTOHighFPSFix.asi and the INI.


INSTALLATION: PLUGIN-ONLY PACKAGE
---------------------------------

The smaller release does not contain an ASI loader. Use it only when a
compatible x64 ASI loader is already installed.

Extract the ZIP directly into the game directory, beside Dishonored_DO.exe.
To use another directory scanned by the loader, such as scripts or plugins,
copy these two files there instead:

  DOTOHighFPSFix.asi
  doto-high-fps-fix.ini

Keep the ASI and INI together. The runtime log is written beside them as
doto-high-fps-fix.log.

Ultimate ASI Loader is available from:

  https://github.com/ThirteenAG/Ultimate-ASI-Loader


CONFIGURATION AND USE
---------------------

Edit doto-high-fps-fix.ini while the game is closed. The tested renderer
interpolation features are enabled by default in v1.1.0. Extracting a new
release over an older one replaces this file, so note any setting you changed
first.

If troubleshooting, disable features one at a time in this order:

1. CinematicSkeletons
2. CinematicTransforms
3. FirstPersonSkeletons
4. WorldSkeletons
5. WorldTransforms
6. StabilizeFirstPersonHands

ShadowTransforms is reserved and forcibly disabled for this game build.
Press F10 in game to toggle camera prediction for comparison.

FirstPersonSkeletons also smooths first-person models that move relative to
the camera, such as the held weapon while it is drawn or holstered. Without
it, the weapon steps at the simulation rate, which is most visible while time
is slowed, for example in the power wheel.

RealTimeTurnPrediction=1 keeps mouse turning even at frame rates that are not
a multiple of 120, such as 90 or 144 FPS. Ctrl+F10 switches camera rotation
between this rule and the previous simulation-time rule for an A/B comparison.

StabilizeMouseSensitivity=1 makes the turn per mouse count the same at every
frame rate, above and below 120 FPS. Sensitivity matches the unmodified game
at its normal 120 FPS cap. Versions before 1.4.0 had half that sensitivity
above 120 FPS and became faster below it; if you raised the in-game
sensitivity for an older version, lower it back to about half.

AdaptivePerformanceGate=1 measures presentation cadence against the fixed
120 Hz simulation clock. Below 130 FPS it disables transform interpolation;
below 125 FPS with skeletons alone it disables skeletons too. It restores
layers only after two measurement windows with enough predicted headroom. A
layer whose removal does not make frames faster, as under a frame cap, is
turned back on at once and kept on. Each level load starts again with every
layer on and is measured after a 5-second warm-up. Set it to 0 to keep every
individually enabled interpolation layer active.

FullInterpolationInCutscenes=1 keeps every enabled interpolation layer active
during cutscenes, such as vehicle rides, even below 120 FPS, so vehicles and
objects attached to them are not stepped. The adaptive gate resumes its
gameplay decision afterwards. Set it to 0 to let the gate manage cutscenes
like gameplay.

FullInterpolationInSlowMotion=1 does the same while the game slows time; set
it to 0 to let the gate manage slow motion too. During slow motion every
layer also accepts the longer gaps between simulation samples, and ownerless
character models are interpolated as well. TimeScaleLog=1 writes the game
clock's slow-down settings and each slow-motion period to the log.

The All -> Skeletons only -> Off order comes from a same-session DOTO capture
in three Follow the Ink views; transforms cost more in the medium and heavy
scenes. For development comparisons, set InterpolationABProbe=1. Ctrl+F11 starts or
stops a segment, Alt+F11 cycles All / Transforms only / Skeletons only / Off,
F11 toggles all interpolation, and Shift+F11 toggles only the first-person root
correction. Keep InterpolationABProbe=0 for ordinary play. Manual F11 or
Alt+F11 selection suspends adaptation until the next game launch.


UNINSTALLATION
--------------

Close the game and remove:

  DOTOHighFPSFix.asi
  doto-high-fps-fix.ini
  doto-high-fps-fix.log     optional generated log

The documentation extracted with the release can also be removed: README.txt,
CHANGELOG.md, LICENSE, and SHA256SUMS.txt, plus Ultimate-ASI-Loader.txt and
Ultimate-ASI-Loader-LICENSE.txt from the loader package.

Do not remove dinput8.dll if another mod uses the ASI loader. If this package
installed it, compare its hash with SHA256SUMS.txt and check for other ASI
plugins before removing the shared loader.


VALIDATION STATUS
-----------------

Focused live testing has passed for:

- Above-120 presentation, mouse input, camera prediction, and first-person
  root correction.
- Thrown props, bottles, doors, and other eligible world roots.
- Ordinary world skeletal animation, including observed combat and ragdolls.
- First-person hands and sword animation.
- The first-level railway-cabin cinematic root and ownerless palettes.
- Forced palette uploads on presentation frames between simulation ticks.
- Loading through the official x64 Ultimate ASI Loader 9.7.4.

At the final focused ASI-loader test diagnostic, skipped uploads, failed forced
uploads, interpolation rejections, same-time mutations, layout failures, and
capacity misses were all zero.

Version 1.3.0's adaptive controller also passed a normal-play regression
through the three performance-test scenes, including combat. The log confirmed
transform-first shedding, skeletal shedding and recovery, local cost learning,
and no safety-counter failure.

Version 1.4.0 was live-tested on both the Steam and GOG builds, including
gameplay, cutscenes with conversations, level transitions, and adaptive
shedding and recovery. Every hook installed on both builds, cutscenes kept
full interpolation and handed control back to the adaptive gate afterwards,
and skipped uploads, failed forced uploads, interpolation rejections, layout
failures, and capacity misses all remained zero.

Version 1.5.0 was played on both builds at 90 and 144 FPS caps. Mouse turning
was even, and slow motion was detected, smoothed, and ended 17 times. The
adaptive gate restarted at the level load and turned layers back on when
removing them did not raise the capped frame rate. Every safety counter stayed
clean, and there was no crash.

Version 1.5.1 was played on both builds with the power wheel opened while the
weapon was drawn or holstered, and with Foresight on GOG. The weapon stayed
smooth, slow motion was detected and ended 23 times, every safety counter
stayed clean, and there was no crash.


KNOWN LIMITATIONS
-----------------

- Only the GOG and Steam 1.145.0.0 builds listed above are supported. Other
  executables are rejected unless their layout matches one of them.
- Steam has had a shorter live test than GOG.
- Shadow transform interpolation is unsupported and disabled.
- Cloth and independently simulated dynamic vertices are not interpolated.
- Reflections, portals, motion blur, and other secondary or temporal paths
  need broader validation.
- Live ASI unloading is unsupported. Close the game before replacing files.
- ASI loaders, ReShade, overlays, injectors, and other proxy DLLs may conflict.
- Above-120 operation requires an external limiter and Triple Buffering off.
- The transform-first adaptive order still needs broader scene and long-session
  validation. Set AdaptivePerformanceGate=0 for continuous full configured
  interpolation if the controller is not useful on your system.
- Below about 150 FPS, for example with a 90 or 144 FPS cap, a layer the
  adaptive gate has turned off for a measured cost, or whose measurement a
  kill cam or cutscene interrupted, stays off until the next level load or
  reload. Set AdaptivePerformanceGate=0 to keep every layer on at a fixed cap.
- The complete cadence, lifecycle, transition, long-session, and mod-
  compatibility matrices have not been exhaustively tested.


FILE VERIFICATION
-----------------

SHA256SUMS.txt contains the SHA-256 hash of every release payload file. The
release ZIP also has a separate .sha256 sidecar file.


LICENSES
--------

The high-FPS fix is released under the MIT License in LICENSE.

The optional bundled Ultimate ASI Loader is third-party software released
under the MIT License by ThirteenAG. Its license and provenance are included as
Ultimate-ASI-Loader-LICENSE.txt and Ultimate-ASI-Loader.txt.


SOURCE AND ISSUE REPORTS
------------------------

When reporting a problem, include the storefront and executable version, the
display refresh rate and FPS cap, doto-high-fps-fix.ini, and the generated
doto-high-fps-fix.log.

The source repository contains build instructions and implementation details in
README.md.


Misc
----

for more game fixes take a look at https://slop-blog.enkhayzomachines.net/fixes :)
