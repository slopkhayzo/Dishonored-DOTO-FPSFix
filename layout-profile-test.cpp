// Offline layout-profile test. Maps a real Dishonored_DO.exe image into this
// process without running any of its code, then runs the plugin's own profile
// selection, strict validation, and hook installation against that private
// mapping. The executable file on disk is never modified.
#define DOTO_TARGET 1
#define DllMain DotoPluginDllMainForLayoutTest
#include "doto_high_fps_fix_impl.cpp"
#undef DllMain

#include <cstdio>

namespace {

int Fail(const char* message) {
    std::fprintf(stderr, "FAIL: %s\n", message);
    return 1;
}

bool IsAbsoluteJumpTo(const unsigned char* code, const void* target) {
    std::uintptr_t address = 0;
    memcpy(&address, code + 6, sizeof(address));
    return code[0] == 0xFF && code[1] == 0x25 && code[2] == 0 && code[3] == 0 &&
           code[4] == 0 && code[5] == 0 &&
           address == reinterpret_cast<std::uintptr_t>(target);
}

bool IsNopPadded(const unsigned char* code, std::size_t length) {
    for (std::size_t index = kAbsoluteJumpLength; index < length; ++index) {
        if (code[index] != 0x90) {
            return false;
        }
    }
    return true;
}

// The entry must jump to the hook, and the trampoline must replay the
// profile's original bytes before resuming just after the patched entry.
bool HookIsInstalled(std::uintptr_t rva, const unsigned char* originalBytes,
                     std::size_t length, const void* hook, const void* trampoline) {
    const auto* entry = reinterpret_cast<const unsigned char*>(g_executableBase + rva);
    const auto* replay = static_cast<const unsigned char*>(trampoline);
    return trampoline != nullptr && IsAbsoluteJumpTo(entry, hook) &&
           IsNopPadded(entry, length) && memcmp(replay, originalBytes, length) == 0 &&
           IsAbsoluteJumpTo(replay + length, entry + length);
}

bool NameMatches(const char* name, const wchar_t* expected) {
    for (; *name != '\0' && *expected != L'\0'; ++name, ++expected) {
        if (static_cast<wchar_t>(static_cast<unsigned char>(*name)) != *expected) {
            return false;
        }
    }
    return *name == '\0' && *expected == L'\0';
}

}  // namespace

int wmain(int argumentCount, wchar_t** arguments) {
    if (argumentCount != 3) {
        std::fwprintf(stderr,
                      L"Usage: layout-profile-test.exe <path-to-Dishonored_DO.exe> "
                      L"<expected-profile-name>\n");
        return 2;
    }

    unsigned char fileHash[32]{};
    if (!ComputeFileSha256(arguments[1], fileHash)) {
        std::fwprintf(stderr, L"Unable to hash %ls\n", arguments[1]);
        return 3;
    }

    HMODULE image = LoadLibraryExW(arguments[1], nullptr, DONT_RESOLVE_DLL_REFERENCES);
    if (image == nullptr) {
        std::fwprintf(stderr, L"Unable to map %ls: %lu\n", arguments[1], GetLastError());
        return 3;
    }
    g_executableBase = reinterpret_cast<std::uintptr_t>(image);

    // Enable every correction path so every location in the profile is checked.
    // Shadow interpolation stays unsupported and forced off for DOTO.
    g_unlockAbove120 = true;
    g_stabilizeMouseSensitivity = true;
    g_stabilizeCamera = true;
    g_stabilizeFirstPersonHands = true;
    g_interpolateWorldTransforms = true;
    g_interpolateCinematicTransforms = true;
    g_interpolateShadowTransforms = false;
    g_interpolateFirstPersonSkeletons = true;
    g_interpolateWorldSkeletons = true;
    g_interpolateCinematicSkeletons = true;
    g_enableAdaptiveInterpolation = true;
    g_fullInterpolationInCutscenes = true;
    g_fullInterpolationInSlowMotion = true;
    g_realTimeTurnPrediction = true;

    PeImageLayout layout{};
    if (!ValidatePeImageLayout(layout) || !ValidateConfiguredCorrectionPaths(layout)) {
        return Fail("strict layout validation rejected the executable");
    }
    if (!NameMatches(g_layout->name, arguments[2])) {
        std::fprintf(stderr, "FAIL: selected profile '%s'\n", g_layout->name);
        return 1;
    }
    if (memcmp(fileHash, g_layout->referenceSha256, sizeof(fileHash)) != 0) {
        return Fail("the executable is not the profile's reference build");
    }

    int headerMatches = 0;
    for (const LayoutProfile& profile : kLayoutProfiles) {
        headerMatches +=
            layout.nt->OptionalHeader.SizeOfImage == profile.imageSize &&
                    layout.nt->OptionalHeader.AddressOfEntryPoint ==
                        profile.entryPointRva
                ? 1
                : 0;
    }
    if (headerMatches != 1) {
        return Fail("profile PE-header selectors are not mutually exclusive");
    }

    if (!InstallHook()) {
        return Fail("view-copy call-site hook was not installed");
    }
    const auto* callsite = reinterpret_cast<const unsigned char*>(
        g_executableBase + g_layout->copyCallsiteRva);
    std::int32_t displacement = 0;
    memcpy(&displacement, callsite + 1, sizeof(displacement));
    const auto* relay = static_cast<const unsigned char*>(g_relay);
    std::uintptr_t relayTarget = 0;
    memcpy(&relayTarget, relay + 2, sizeof(relayTarget));
    if (callsite[0] != 0xE8 ||
        reinterpret_cast<std::uintptr_t>(callsite) + 5 + displacement !=
            reinterpret_cast<std::uintptr_t>(g_relay) ||
        relay[0] != 0x48 || relay[1] != 0xB8 || relay[10] != 0xFF || relay[11] != 0xE0 ||
        relayTarget != reinterpret_cast<std::uintptr_t>(&HookCopyView) ||
        reinterpret_cast<std::uintptr_t>(g_originalCopyView) !=
            g_executableBase + g_layout->copyViewRva) {
        return Fail("view-copy call site does not reach HookCopyView");
    }

    // The virtual delta call must become `lea rax,[rcx+clock+0xC8]; nop` while
    // the surrounding instructions stay intact.
    if (!InstallMouseLookDeltaPatch() || !g_mouseLookDeltaPatched) {
        return Fail("mouse-look delta patch was not installed");
    }
    const auto* mouseSite = reinterpret_cast<const unsigned char*>(
        g_executableBase + g_layout->mouseLookDeltaSiteRva);
    const std::uint32_t mouseDisplacement =
        static_cast<std::uint32_t>(kClockOffset + kSimulationTimerDeltaOffset);
    unsigned char expectedMousePatch[kMouseLookDeltaPatchLength] = {0x48, 0x8D, 0x81};
    memcpy(expectedMousePatch + 3, &mouseDisplacement, sizeof(mouseDisplacement));
    expectedMousePatch[7] = 0x66;
    expectedMousePatch[8] = 0x90;
    const std::size_t mouseSuffix = kMouseLookDeltaPatchOffset + kMouseLookDeltaPatchLength;
    if (memcmp(mouseSite, kExpectedMouseLookDeltaSite, kMouseLookDeltaPatchOffset) != 0 ||
        memcmp(mouseSite + kMouseLookDeltaPatchOffset, expectedMousePatch,
               sizeof(expectedMousePatch)) != 0 ||
        memcmp(mouseSite + mouseSuffix, kExpectedMouseLookDeltaSite + mouseSuffix,
               sizeof(kExpectedMouseLookDeltaSite) - mouseSuffix) != 0) {
        return Fail("mouse-look delta patch bytes are wrong");
    }

    if (!InstallRenderModelGpuCopyHook() ||
        !HookIsInstalled(g_layout->renderModelGpuCopyRva,
                         g_layout->renderModelGpuCopyBytes.data(),
                         g_layout->renderModelGpuCopyHookLength,
                         reinterpret_cast<const void*>(&HookRenderModelGpuCopy),
                         g_renderModelGpuCopyTrampoline)) {
        return Fail("renderer GPU-copy hook or trampoline is wrong");
    }

    if (!InstallSkinnedPoseUploadHook() ||
        !HookIsInstalled(g_layout->skinnedPoseUploadRva,
                         g_layout->skinnedPoseUploadBytes.data(),
                         g_layout->skinnedPoseUploadHookLength,
                         reinterpret_cast<const void*>(&HookSkinnedPoseUpload),
                         g_skinnedPoseUploadTrampoline)) {
        return Fail("skinned-pose upload hook or trampoline is wrong");
    }

    if (InstallShadowCasterBuildHooks()) {
        return Fail("shadow hooks were installed although DOTO has no mapping");
    }

    // Do not FreeLibrary: hooks in the mapping point into this process.
    std::printf("PASS: %s profile validated; camera, mouse-look, GPU-copy, and "
                "skeletal patches installed and verified on a private mapping.\n",
                g_layout->name);
    return 0;
}
