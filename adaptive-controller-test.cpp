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
    g_fullInterpolationInCutscenes = false;
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

void PresentNextFrame() {
    PresentationContext context{};
    context.valid = true;
    context.serial = 3;
    context.scaledTime =
        g_adaptiveInterpolationGate.previousScaledTime + kNativeSimulationStep;
    context.simulationTime =
        g_adaptiveInterpolationGate.previousSimulationTime + kNativeSimulationStep;
    UpdateAdaptiveInterpolationGate(context);
}

// A minimal stand-in for the executable image: a game pointer, an idGameLocal
// whose embedded game-logic manager points back at it, and the cinematic state
// object, all addressed relative to g_executableBase through a test profile.
class FakeGameImage {
public:
    static constexpr std::uintptr_t kGamePointerRva = 0x10;
    static constexpr std::uintptr_t kGameVtableRva = 0x20;
    static constexpr std::uintptr_t kCinematicVtableRva = 0x30;
    static constexpr std::uintptr_t kCinematicStateRva = 0x40;
    static constexpr std::uintptr_t kGameRva = 0x100;

    FakeGameImage() : bytes_(kGameRva + kGameLogicManagerOffset + 0x1000) {
        layout_.name = "test";
        layout_.gamePointerRva = kGamePointerRva;
        layout_.gameVtableRva = kGameVtableRva;
        layout_.cinematicGameLogicStateVtableRva = kCinematicVtableRva;
        g_executableBase = reinterpret_cast<std::uintptr_t>(bytes_.data());
        g_layout = &layout_;

        Store(kGamePointerRva, g_executableBase + kGameRva);
        Store(kGameRva, g_executableBase + kGameVtableRva);
        SetOwner(g_executableBase + kGameRva);
        Store(ManagerRva() + kGameLogicStatesOffset +
                  kGameLogicStateCinematic * sizeof(void*),
              g_executableBase + kCinematicStateRva);
        SetCinematicVtable(g_executableBase + kCinematicVtableRva);
        Store(ManagerRva() + kGameLogicStackDataOffset,
              reinterpret_cast<std::uintptr_t>(stack_));
    }

    ~FakeGameImage() {
        g_layout = nullptr;
        g_executableBase = 0;
    }

    void SetStack(std::initializer_list<std::int32_t> ids) {
        std::uint32_t count = 0;
        for (const std::int32_t id : ids) {
            stack_[count++] = id;
        }
        memcpy(bytes_.data() + ManagerRva() + kGameLogicStackCountOffset, &count,
               sizeof(count));
    }

    void SetOwner(std::uintptr_t owner) {
        Store(ManagerRva() + kGameLogicOwnerOffset, owner);
    }

    void SetCinematicVtable(std::uintptr_t vtable) {
        Store(kCinematicStateRva, vtable);
    }

private:
    static constexpr std::uintptr_t ManagerRva() {
        return kGameRva + kGameLogicManagerOffset;
    }

    void Store(std::uintptr_t rva, std::uintptr_t value) {
        memcpy(bytes_.data() + rva, &value, sizeof(value));
    }

    std::vector<unsigned char> bytes_;
    LayoutProfile layout_{};
    std::int32_t stack_[kGameLogicStackLimit]{};
};

void TestCutsceneOverride() {
    constexpr std::int32_t kInGame = 1;
    FakeGameImage image;

    ResetController();
    g_fullInterpolationInCutscenes = true;
    image.SetStack({kInGame});
    EvaluateWindow(kAdaptiveAllMinimumFps - 0.01);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          !RuntimeTransformInterpolationEnabled() &&
          !g_adaptiveInterpolationGate.cutsceneOverride,
          "Gameplay below the threshold sheds transforms before a cutscene");

    image.SetStack({kInGame, kGameLogicStateCinematic});
    PresentNextFrame();
    Check(g_adaptiveInterpolationGate.cutsceneOverride &&
          RuntimeTransformInterpolationEnabled() &&
          RuntimeSkeletalInterpolationEnabled() &&
          g_adaptiveInterpolationGate.windowFrames == 0.0,
          "Entering the cinematic state enables every configured layer");

    EvaluateWindow(60.0);
    EvaluateWindow(60.0);
    Check(g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          RuntimeTransformInterpolationEnabled() &&
          RuntimeSkeletalInterpolationEnabled(),
          "Heavy cutscene cadence neither sheds layers nor changes the gameplay mode");

    image.SetStack({kInGame});
    PresentNextFrame();
    Check(!g_adaptiveInterpolationGate.cutsceneOverride &&
          g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly &&
          !RuntimeTransformInterpolationEnabled() &&
          RuntimeSkeletalInterpolationEnabled() &&
          g_adaptiveInterpolationGate.windowFrames == 0.0,
          "Leaving the cutscene restores the gameplay mode with a fresh window");

    ResetController();
    g_fullInterpolationInCutscenes = true;
    image.SetStack({kGameLogicStateCinematic});
    image.SetCinematicVtable(0);
    PresentNextFrame();
    Check(g_adaptiveInterpolationGate.cutsceneDetectionFailed &&
          !g_adaptiveInterpolationGate.cutsceneOverride,
          "A foreign object in the cinematic slot disables cutscene detection");
    image.SetCinematicVtable(g_executableBase + FakeGameImage::kCinematicVtableRva);

    ResetController();
    g_fullInterpolationInCutscenes = true;
    image.SetStack({kGameLogicStateCinematic});
    image.SetOwner(0);
    PresentNextFrame();
    Check(!g_adaptiveInterpolationGate.cutsceneOverride &&
          !g_adaptiveInterpolationGate.cutsceneDetectionFailed,
          "A manager not owned by the game is ignored without an override");
    image.SetOwner(g_executableBase + FakeGameImage::kGameRva);

    ResetController();
    image.SetStack({kGameLogicStateCinematic});
    EvaluateWindow(kAdaptiveAllMinimumFps - 0.01);
    Check(!g_adaptiveInterpolationGate.cutsceneOverride &&
          g_adaptiveInterpolationGate.mode ==
              AdaptiveInterpolationMode::SkeletonsOnly,
          "FullInterpolationInCutscenes=0 lets the gate manage cutscenes");
    g_fullInterpolationInCutscenes = false;
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
    TestCutsceneOverride();
    if (g_failures != 0) {
        std::fprintf(stderr, "%d adaptive controller test(s) failed.\n",
                     g_failures);
        return 1;
    }
    std::puts("DOTO adaptive interpolation controller tests passed.");
    return 0;
}
