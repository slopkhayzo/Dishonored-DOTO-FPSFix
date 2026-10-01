#define DOTO_TARGET 1
#define DllMain DotoPluginDllMainForAdaptiveTest
#include "doto_high_fps_fix_impl.cpp"
#undef DllMain

#include <cstdio>

namespace {

int g_failures = 0;

void Check(bool condition, const char* description) {
    if (!condition) {
        std::fprintf(stderr, "FAIL: %s\n", description);
        ++g_failures;
    }
}

void ResetController(bool transforms = true, bool skeletons = true) {
    g_interpolateWorldTransforms = transforms;
    g_interpolateCinematicTransforms = false;
    g_interpolateShadowTransforms = false;
    g_interpolateFirstPersonSkeletons = skeletons;
    g_interpolateWorldSkeletons = false;
    g_interpolateCinematicSkeletons = false;
    g_enableAdaptiveInterpolation = true;
    g_enableInterpolationAbProbe = false;
    g_adaptiveInterpolationManualOverride.store(false,
                                                 std::memory_order_relaxed);
    g_transformInterpolationRuntimeEnabled.store(true,
                                                  std::memory_order_relaxed);
    g_skeletalInterpolationRuntimeEnabled.store(true,
                                                 std::memory_order_relaxed);
    g_adaptiveInterpolationGate = {};

    PresentationContext initial{};
    initial.valid = true;
    initial.serial = 1;
    UpdateAdaptiveInterpolationGate(initial);
}

void EvaluateWindow(double cadenceFps) {
    AdaptiveInterpolationGateState& gate = g_adaptiveInterpolationGate;
    gate.windowFrames = cadenceFps *
        kAdaptiveWindowSimulationTicks * kNativeSimulationStep - 1.0;
    gate.windowSimulationTicks = kAdaptiveWindowSimulationTicks - 1.0;

    PresentationContext context{};
    context.valid = true;
    context.serial = 2;
    context.scaledTime = gate.previousScaledTime + kNativeSimulationStep;
    context.simulationTime =
        gate.previousSimulationTime + kNativeSimulationStep;
    UpdateAdaptiveInterpolationGate(context);
}

void TestThresholdBoundaries() {
    ResetController();
    EvaluateWindow(kAdaptiveAllMinimumFps);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All,
          "All remains enabled at the exact degradation threshold");

    ResetController();
    EvaluateWindow(kAdaptiveAllMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          !RuntimeTransformInterpolationEnabled() &&
          RuntimeSkeletalInterpolationEnabled(),
          "All sheds transforms immediately below its threshold");

    ResetController();
    g_adaptiveInterpolationGate.mode =
        AdaptiveInterpolationMode::SkeletonsOnly;
    EvaluateWindow(kAdaptiveIntermediateMinimumFps);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly,
          "Skeletons remain enabled at the exact lower-profile threshold");
    EvaluateWindow(kAdaptiveIntermediateMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::Off,
          "Skeletons shed immediately below the lower-profile threshold");
}

void TestRecoveryDwellAndRefresh() {
    ResetController();
    EvaluateWindow(kAdaptiveAllMinimumFps - 0.01);
    EvaluateWindow(kAdaptiveIntermediateMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::Off &&
          g_adaptiveInterpolationGate.hasTransformPenalty &&
          g_adaptiveInterpolationGate.learnSkeletalPenalty,
          "Sustained heavy cadence follows All -> SkeletonsOnly -> Off");

    ResetController();
    g_adaptiveInterpolationGate.mode =
        AdaptiveInterpolationMode::SkeletonsOnly;
    g_transformInterpolationRuntimeEnabled.store(false,
                                                 std::memory_order_relaxed);
    g_adaptiveInterpolationGate.hasTransformPenalty = true;
    g_adaptiveInterpolationGate.transformPenaltySeconds = 0.0;
    EvaluateWindow(kAdaptiveAllRecoveryFps);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          g_adaptiveInterpolationGate.recoveryWindows == 1,
          "Recovery requires more than one qualifying window");
    EvaluateWindow(kAdaptiveAllRecoveryFps);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All &&
          RuntimeTransformInterpolationEnabled(),
          "Second qualifying window restores transforms");

    ResetController();
    g_adaptiveInterpolationGate.mode = AdaptiveInterpolationMode::Off;
    g_transformInterpolationRuntimeEnabled.store(false,
                                                  std::memory_order_relaxed);
    g_skeletalInterpolationRuntimeEnabled.store(false,
                                                 std::memory_order_relaxed);
    g_adaptiveInterpolationGate.hasSkeletalPenalty = true;
    g_adaptiveInterpolationGate.skeletalPenaltySeconds = 0.0;
    EvaluateWindow(kAdaptiveIntermediateRecoveryFps);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::Off &&
          g_adaptiveInterpolationGate.recoveryWindows == 1,
          "Lower-profile recovery also requires two qualifying windows");
    EvaluateWindow(kAdaptiveIntermediateRecoveryFps);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          !RuntimeTransformInterpolationEnabled() &&
          RuntimeSkeletalInterpolationEnabled(),
          "Off recovers to the measured DOTO skeleton-only profile");

    ResetController();
    g_adaptiveInterpolationGate.mode =
        AdaptiveInterpolationMode::SkeletonsOnly;
    g_transformInterpolationRuntimeEnabled.store(false,
                                                 std::memory_order_relaxed);
    g_adaptiveInterpolationGate.hasTransformPenalty = true;
    g_adaptiveInterpolationGate.transformPenaltySeconds = 0.010;
    g_adaptiveInterpolationGate.transformPenaltyReferenceFrameSeconds = 0.010;
    EvaluateWindow(kAdaptiveAllCostRefreshMinimumFps);
    EvaluateWindow(kAdaptiveAllCostRefreshMinimumFps);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All,
          "A 25-percent-faster context gets one transform cost-refresh probe");
    EvaluateWindow(kAdaptiveAllMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly,
          "A failed cost-refresh probe sheds the expensive family again");
    EvaluateWindow(kAdaptiveAllCostRefreshMinimumFps);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          g_adaptiveInterpolationGate.recoveryWindows == 0,
          "Failed probe relearns locally and does not immediately repeat");
}

void TestSuspensionAndInvalidClock() {
    ResetController();
    g_enableAdaptiveInterpolation = false;
    EvaluateWindow(60.0);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All,
          "AdaptivePerformanceGate=0 is a complete controller opt-out");

    ResetController();
    g_enableInterpolationAbProbe = true;
    EvaluateWindow(60.0);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All,
          "A/B probing suspends adaptive decisions");

    ResetController();
    g_adaptiveInterpolationManualOverride.store(true,
                                                 std::memory_order_relaxed);
    EvaluateWindow(60.0);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All,
          "Manual profile selection persists for the process lifetime");

    ResetController();
    g_adaptiveInterpolationGate.mode =
        AdaptiveInterpolationMode::SkeletonsOnly;
    g_adaptiveInterpolationGate.windowFrames = 100.0;
    g_adaptiveInterpolationGate.windowSimulationTicks = 100.0;
    g_adaptiveInterpolationGate.recoveryWindows = 1;
    PresentationContext paused{};
    paused.valid = true;
    paused.serial = 2;
    paused.scaledTime = g_adaptiveInterpolationGate.previousScaledTime;
    paused.simulationTime =
        g_adaptiveInterpolationGate.previousSimulationTime;
    UpdateAdaptiveInterpolationGate(paused);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          g_adaptiveInterpolationGate.windowFrames == 0.0 &&
          g_adaptiveInterpolationGate.windowSimulationTicks == 0.0 &&
          g_adaptiveInterpolationGate.recoveryWindows == 0,
          "Paused clocks clear only the active window and recovery streak");
}

void TestSingleFamilyConfigurations() {
    ResetController(true, false);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::TransformsOnly,
          "Transform-only configuration skips the meaningless All mode");
    EvaluateWindow(kAdaptiveIntermediateMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::Off,
          "Transform-only configuration can shed directly to Off");

    ResetController(false, true);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::All,
          "Skeletal-only configuration starts with its configured family on");
    EvaluateWindow(kAdaptiveAllMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode == AdaptiveInterpolationMode::Off,
          "Skeletal-only configuration skips TransformsOnly");
}

}  // namespace

int main() {
    TestThresholdBoundaries();
    TestRecoveryDwellAndRefresh();
    TestSuspensionAndInvalidClock();
    TestSingleFamilyConfigurations();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d adaptive controller test(s) failed.\n",
                     g_failures);
        return 1;
    }
    std::puts("DOTO adaptive interpolation controller tests passed.");
    return 0;
}
