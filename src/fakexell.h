#pragma once

#include <dxgi.h>
#if _MSC_VER
#include <d3d12.h>
#else
#include "../external/d3d12.h"
#endif

#include "low_latency.h"

#include "util.h"
#include "log.h"
#include <xell.h>
#include <xell_d3d12.h>

class LowLatencyCtxXell {
public:
    static void init() {
        lowlatency_ctx = new LowLatency();
    }

    static void shutdown() {
        delete lowlatency_ctx;
        lowlatency_ctx = nullptr;
    }

    static LowLatency* get() {
        return lowlatency_ctx;
    }

private:
    static LowLatency* lowlatency_ctx;
};

namespace fakexell {
    bool Init();

    static xell_context_handle_t gameNativeContext = nullptr;

    typedef decltype(&xellDestroyContext) PFN_xellDestroyContext;
    typedef decltype(&xellSetSleepMode) PFN_xellSetSleepMode;
    typedef decltype(&xellGetSleepMode) PFN_xellGetSleepMode;
    typedef decltype(&xellSleep) PFN_xellSleep;
    typedef decltype(&xellAddMarkerData) PFN_xellAddMarkerData;
    typedef decltype(&xellGetVersion) PFN_xellGetVersion;
    typedef decltype(&xellSetLoggingCallback) PFN_xellSetLoggingCallback;
    typedef decltype(&xellD3D12CreateContext) PFN_xellD3D12CreateContext;

    static PFN_xellDestroyContext o_xellDestroyContext = nullptr;
    static PFN_xellSetSleepMode o_xellSetSleepMode = nullptr;
    static PFN_xellGetSleepMode o_xellGetSleepMode = nullptr;
    static PFN_xellSleep o_xellSleep = nullptr;
    static PFN_xellAddMarkerData o_xellAddMarkerData = nullptr;
    static PFN_xellGetVersion o_xellGetVersion = nullptr;
    static PFN_xellSetLoggingCallback o_xellSetLoggingCallback = nullptr;
    static PFN_xellD3D12CreateContext o_xellD3D12CreateContext = nullptr;

    static xell_result_t hkxellDestroyContext(xell_context_handle_t context);
    static xell_result_t hkxellSetSleepMode(xell_context_handle_t context, const xell_sleep_params_t* param);
    static xell_result_t hkxellGetSleepMode(xell_context_handle_t context, xell_sleep_params_t* param);
    static xell_result_t hkxellSleep(xell_context_handle_t context, uint32_t frame_id);
    static xell_result_t hkxellAddMarkerData(xell_context_handle_t context, uint32_t frame_id, xell_latency_marker_type_t marker);
    static xell_result_t hkxellGetVersion(xell_version_t* pVersion);
    static xell_result_t hkxellSetLoggingCallback(xell_context_handle_t hContext, xell_logging_level_t loggingLevel, xell_app_log_callback_t loggingCallback);
    static xell_result_t hkxellD3D12CreateContext(ID3D12Device* device, xell_context_handle_t* out_context);
}