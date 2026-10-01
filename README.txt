Dishonored: Death of the Outsider high-FPS fix v1.3.0
=====================================================

IMPORTANT
---------

This patch was generated almost entirely with AI assistance. The author does
not claim reverse-engineering expertise. It has received focused testing at
120, 144, 240, and 360 Hz without a major observed issue, but it has not been
tested across every system, level, display mode, or mod combination.


WHAT IT DOES
------------

This is a renderer-only high-frame-rate patch for the GOG release of
Dishonored: Death of the Outsider. It allows presentation above 120 FPS and
smooths the camera, mouse input, first-person models, supported world
transforms, supported skeletal animation, and small cinematic models.

Simulation, physics, AI, scripts, animation events, and audio remain at the
game's native 120 Hz. The patch changes temporary renderer-owned copies and
palettes rather than writing interpolated values back to game entities.


VALIDATED GAME BUILD
--------------------

Store:       GOG
Version:     1.145.0.0
Executable:  Dishonored_DO.exe
SHA-256:     DA7E8EB3FDFA28BF552079B37F626A009F35941D9EEB015BA61AC9BFCD850C08

This is the only build that has been live-tested. Its SHA-256 is diagnostic,
not a hard activation allowlist. An unrecognized Dishonored_DO.exe is accepted
only if its x64 PE image layout, mapped RVAs and section permissions, camera
call relationship, and every enabled hook signature match the verified
profile. Shifted code or data layouts are rejected and need a separately
mapped compatibility profile. Runtime object and palette checks remain
fail-closed.


REQUIREMENTS
------------

- 64-bit Windows.
- Dishonored_DO.exe matching the mapped compatibility profile above. The GOG
  build listed above remains the only live-tested executable.
- A compatible x64 ASI loader.
- A safe external FPS limiter such as RTSS or a driver-level limiter.

Disable the game's Triple Buffering option when unlocking above 120 FPS. Set a
sensible external frame-rate cap before launching the game.


INSTALLATION: PACKAGE WITH ULTIMATE ASI LOADER
----------------------------------------------

Use the release whose filename ends with:

  -with-Ultimate-ASI-Loader.zip

1. Close the game.
2. Open the extracted release folder.
3. Copy its contents into the game directory beside Dishonored_DO.exe.

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

Copy these two files into a directory scanned by the loader, such as the game
root, scripts, or plugins:

  DOTOHighFPSFix.asi
  doto-high-fps-fix.ini

Keep the ASI and INI together. The runtime log is written beside them as
doto-high-fps-fix.log.

Ultimate ASI Loader is available from:

  https://github.com/ThirteenAG/Ultimate-ASI-Loader


CONFIGURATION AND USE
---------------------

Edit doto-high-fps-fix.ini while the game is closed. The tested renderer
interpolation features are enabled by default in v1.1.0.

If troubleshooting, disable features one at a time in this order:

1. CinematicSkeletons
2. CinematicTransforms
3. FirstPersonSkeletons
4. WorldSkeletons
5. WorldTransforms
6. StabilizeFirstPersonHands

ShadowTransforms is reserved and forcibly disabled for this game build.
Press F10 in game to toggle camera prediction for comparison.

AdaptivePerformanceGate=1 measures presentation cadence against the fixed
120 Hz simulation clock. Below 130 FPS it disables transform interpolation;
below 125 FPS with skeletons alone it disables skeletons too. It restores
layers only after two measurement windows with enough predicted headroom. Set
it to 0 to keep every individually enabled interpolation layer active.

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


KNOWN LIMITATIONS
-----------------

- Only GOG 1.145.0.0 has been live-tested. Same-layout executables may pass the
  structural preflight; shifted layouts remain incompatible.
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
