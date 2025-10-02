#include "fakexell.h"
#include <detours.h>
#include <magic_enum.hpp>

#define HOOK(name) if (o_##name) DetourAttach(&(PVOID&) o_##name, hk##name)

LowLatency* LowLatencyCtxXell::lowlatency_ctx = nullptr;
// static auto init_mutex = std::mutex{};

namespace fakexell
{
    bool Init() {
        if (o_xellDestroyContext) {
            // spdlog::info("Already hooked");
            return true;
        }

        auto xell_module = GetModuleHandleA("libxell.dll");

        if (!xell_module) {
            spdlog::error("Couldn't hook libxell");
            return false;
        }

        o_xellDestroyContext = (PFN_xellDestroyContext) GetProcAddress(xell_module, "xellDestroyContext");
        o_xellSetSleepMode = (PFN_xellSetSleepMode) GetProcAddress(xell_module, "xellSetSleepMode");
        o_xellGetSleepMode = (PFN_xellGetSleepMode) GetProcAddress(xell_module, "xellGetSleepMode");
        o_xellSleep = (PFN_xellSleep) GetProcAddress(xell_module, "xellSleep");
        o_xellAddMarkerData = (PFN_xellAddMarkerData) GetProcAddress(xell_module, "xellAddMarkerData");
        o_xellGetVersion = (PFN_xellGetVersion) GetProcAddress(xell_module, "xellGetVersion");
        o_xellSetLoggingCallback = (PFN_xellSetLoggingCallback) GetProcAddress(xell_module, "xellSetLoggingCallback");
        o_xellD3D12CreateContext = (PFN_xellD3D12CreateContext) GetProcAddress(xell_module, "xellD3D12CreateContext");

        DetourTransactionBegin();
        DetourUpdateThread(GetCurrentThread());

        HOOK(xellDestroyContext);
        HOOK(xellSetSleepMode);
        HOOK(xellGetSleepMode);
        HOOK(xellSleep);
        HOOK(xellAddMarkerData);
        HOOK(xellGetVersion);
        HOOK(xellSetLoggingCallback);
        HOOK(xellD3D12CreateContext);

        DetourTransactionCommit();
    }

    HMODULE GetCallerModule(void* returnAddress)
    {
        HMODULE hModule = NULL;

        GetModuleHandleExA(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,
                        (LPCSTR) returnAddress, &hModule);

        return hModule;
    }

    bool is_game_context(xell_context_handle_t context, void* returnAddress) {
        if (gameNativeContext == context)
            return true;
        else if (gameNativeContext)
            return false;
        
        if (GetModuleHandle(nullptr) == GetCallerModule(returnAddress)) {
            gameNativeContext = context;
            return true;
        }

        return false;
    }

    xell_result_t hkxellDestroyContext(xell_context_handle_t context) {
        if (is_game_context(context, _ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellDestroyContext");
        return o_xellDestroyContext(context);
    }
    xell_result_t hkxellSetSleepMode(xell_context_handle_t context, const xell_sleep_params_t* param) {
        if (is_game_context(context, _ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellSetSleepMode");
        return o_xellSetSleepMode(context, param);
    }
    xell_result_t hkxellGetSleepMode(xell_context_handle_t context, xell_sleep_params_t* param) {
        if (is_game_context(context, _ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellGetSleepMode");
        return o_xellGetSleepMode(context, param);
    }
    xell_result_t hkxellSleep(xell_context_handle_t context, uint32_t frame_id) {
        if (is_game_context(context, _ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellSleep: {}", frame_id);
        return o_xellSleep(context, frame_id);
    }
    xell_result_t hkxellAddMarkerData(xell_context_handle_t context, uint32_t frame_id, xell_latency_marker_type_t marker) {
        if (is_game_context(context, _ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellAddMarkerData {}: {}", magic_enum::enum_name(marker), frame_id);
        return o_xellAddMarkerData(context, frame_id, marker);
    }
    xell_result_t hkxellGetVersion(xell_version_t* pVersion) {
        if (GetModuleHandle(nullptr) == GetCallerModule(_ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellGetVersion");
        return o_xellGetVersion(pVersion);
    }
    xell_result_t hkxellSetLoggingCallback(xell_context_handle_t hContext, xell_logging_level_t loggingLevel, xell_app_log_callback_t loggingCallback) {
        if (GetModuleHandle(nullptr) == GetCallerModule(_ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellSetLoggingCallback");
        return o_xellSetLoggingCallback(hContext, loggingLevel, loggingCallback);
    }
    xell_result_t hkxellD3D12CreateContext(ID3D12Device* device, xell_context_handle_t* out_context) {
        if (GetModuleHandle(nullptr) == GetCallerModule(_ReturnAddress())) return XELL_RESULT_SUCCESS;
        // spdlog::info("xellD3D12CreateContext");
        return o_xellD3D12CreateContext(device, out_context);
    }
}