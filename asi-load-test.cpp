#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <cstdio>
#include <cwchar>

namespace {

bool QueryFileSize(const wchar_t* path, ULONGLONG& size) {
    WIN32_FILE_ATTRIBUTE_DATA attributes{};
    if (!GetFileAttributesExW(path, GetFileExInfoStandard, &attributes)) {
        size = 0;
        return false;
    }
    size = (static_cast<ULONGLONG>(attributes.nFileSizeHigh) << 32) |
        attributes.nFileSizeLow;
    return true;
}

bool BuildSiblingPath(const wchar_t* modulePath, const wchar_t* filename,
                      wchar_t (&output)[MAX_PATH]) {
    if (wcscpy_s(output, modulePath) != 0) {
        return false;
    }
    wchar_t* slash = wcsrchr(output, L'\\');
    if (slash == nullptr) {
        return false;
    }
    slash[1] = L'\0';
    return wcscat_s(output, filename) == 0;
}

}  // namespace

int wmain(int argumentCount, wchar_t** arguments) {
    if (argumentCount != 2 && argumentCount != 3) {
        std::fwprintf(stderr,
            L"Usage: asi-load-test.exe <path-to-plugin.asi> [path-to-ASI-loader.dll]\n");
        return 2;
    }

    wchar_t pluginPath[MAX_PATH]{};
    if (GetFullPathNameW(arguments[1], MAX_PATH, pluginPath, nullptr) == 0) {
        std::fwprintf(stderr, L"GetFullPathNameW failed: %lu\n", GetLastError());
        return 3;
    }

    const wchar_t* extension = wcsrchr(pluginPath, L'.');
    if (extension == nullptr || _wcsicmp(extension, L".asi") != 0) {
        std::fwprintf(stderr, L"The plugin under test must have an .asi extension.\n");
        return 4;
    }

    wchar_t logPath[MAX_PATH]{};
    if (!BuildSiblingPath(pluginPath, L"doto-high-fps-fix.log", logPath)) {
        std::fwprintf(stderr, L"Could not construct the plugin log path.\n");
        return 5;
    }

    ULONGLONG originalLogSize = 0;
    QueryFileSize(logPath, originalLogSize);

    const bool testThroughLoader = argumentCount == 3;
    HMODULE plugin = nullptr;
    if (testThroughLoader) {
        wchar_t loaderPath[MAX_PATH]{};
        if (GetFullPathNameW(arguments[2], MAX_PATH, loaderPath, nullptr) == 0) {
            std::fwprintf(stderr, L"Could not resolve the ASI loader path: %lu\n",
                          GetLastError());
            return 6;
        }
        HMODULE loader = LoadLibraryW(loaderPath);
        if (loader == nullptr) {
            std::fwprintf(stderr, L"Loading the ASI loader failed: %lu\n", GetLastError());
            return 7;
        }

        const wchar_t* pluginName = wcsrchr(pluginPath, L'\\');
        pluginName = pluginName == nullptr ? pluginPath : pluginName + 1;
        for (unsigned int attempt = 0; attempt < 100 && plugin == nullptr; ++attempt) {
            plugin = GetModuleHandleW(pluginName);
            if (plugin == nullptr) {
                Sleep(50);
            }
        }
        if (plugin == nullptr) {
            std::fwprintf(stderr, L"The external loader did not load the ASI plugin.\n");
            return 8;
        }
    }
    else {
        plugin = LoadLibraryW(pluginPath);
        if (plugin == nullptr) {
            std::fwprintf(stderr, L"LoadLibraryW failed: %lu\n", GetLastError());
            return 9;
        }
    }

    if (GetProcAddress(plugin, "DirectInput8Create") != nullptr) {
        std::fwprintf(stderr, L"The ASI unexpectedly exports DirectInput8Create.\n");
        return 10;
    }

    HMODULE secondReference = LoadLibraryW(pluginPath);
    if (secondReference == nullptr || secondReference != plugin) {
        std::fwprintf(stderr, L"A repeated load did not reuse the same module.\n");
        return 11;
    }

    bool rejectionLogged = false;
    for (unsigned int attempt = 0; attempt < 100; ++attempt) {
        ULONGLONG currentLogSize = 0;
        if (QueryFileSize(logPath, currentLogSize) && currentLogSize > originalLogSize) {
            rejectionLogged = true;
            break;
        }
        Sleep(50);
    }

    if (!rejectionLogged) {
        std::fwprintf(stderr, L"The plugin did not append its expected host-rejection log.\n");
        return 12;
    }

    // The installation worker rejects this test host's executable identity/PE
    // layout before touching hook sites. Keep the module loaded until process
    // exit so the worker cannot race an unsupported FreeLibrary operation.
    std::wprintf(testThroughLoader
        ? L"External ASI loader integration and unsupported-host rejection succeeded.\n"
        : L"ASI load and unsupported-host rejection succeeded.\n");
    return 0;
}
