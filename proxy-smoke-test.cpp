#define WIN32_LEAN_AND_MEAN
#define DIRECTINPUT_VERSION 0x0800
#include <windows.h>
#include <dinput.h>

#include <cstdio>
#include <cwchar>

using DirectInput8CreateFn = HRESULT(WINAPI*)(
    HINSTANCE, DWORD, REFIID, LPVOID*, LPUNKNOWN);

int wmain(int argumentCount, wchar_t** arguments) {
    if (argumentCount != 2) {
        std::fwprintf(stderr, L"Usage: proxy-smoke-test.exe <path-to-dinput8.dll>\n");
        return 2;
    }

    HMODULE proxy = LoadLibraryW(arguments[1]);
    if (proxy == nullptr) {
        std::fwprintf(stderr, L"LoadLibraryW failed: %lu\n", GetLastError());
        return 3;
    }

    auto create = reinterpret_cast<DirectInput8CreateFn>(
        GetProcAddress(proxy, "DirectInput8Create"));
    if (create == nullptr) {
        std::fwprintf(stderr, L"DirectInput8Create export was not found.\n");
        return 4;
    }

    IDirectInput8W* directInput = nullptr;
    const HRESULT result = create(GetModuleHandleW(nullptr), DIRECTINPUT_VERSION,
                                  IID_IDirectInput8W,
                                  reinterpret_cast<void**>(&directInput), nullptr);
    if (FAILED(result) || directInput == nullptr) {
        std::fwprintf(stderr, L"Forwarded DirectInput8Create failed: 0x%08lX\n",
                      static_cast<unsigned long>(result));
        return 5;
    }

    directInput->Release();
    std::wprintf(L"Proxy load, export lookup, and system DirectInput forwarding succeeded.\n");

    // The worker deliberately rejects this test host's executable hash. Keep
    // the module loaded until process exit so it cannot race FreeLibrary.
    Sleep(250);
    return 0;
}
