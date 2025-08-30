#pragma once

#include "low_latency_tech.h"

#include <xell_d3d12.h>

class XeLL : public virtual LowLatencyTech {
    HMODULE xell_dll = nullptr;
    decltype(&xellD3D12CreateContext) o_xellD3D12CreateContext = nullptr;
    decltype(&xellDestroyContext) o_xellDestroyContext = nullptr;
    decltype(&xellSetSleepMode) o_xellSetSleepMode = nullptr;
    decltype(&xellGetSleepMode) o_xellGetSleepMode = nullptr;
    decltype(&xellSleep) o_xellSleep = nullptr;
    decltype(&xellAddMarkerData) o_xellAddMarkerData = nullptr;
    decltype(&xellGetVersion) o_xellGetVersion = nullptr;
    decltype(&xellSetLoggingCallback) o_xellSetLoggingCallback = nullptr;
    decltype(&xellGetFramesReports) o_xellGetFramesReports = nullptr;

    xell_context_handle_t ctx = nullptr;
    bool sent_sleep_frame_ids[64]{};
    uint64_t simulation_start_last_id {};
    uint64_t sleep_last_id {};

    bool load_dll();
    bool unload_dll();

    void xell_sleep(uint32_t frame_id);
    void add_marker(uint32_t frame_id, xell_latency_marker_type_t marker);

public:
    XeLL(): LowLatencyTech() {}

    // From LowLatencyTech
    bool init(IUnknown *pDevice) override;
    bool init_using_ctx(void* context) override;
    void deinit() override;

    Mode get_mode() override { return Mode::XeLL; };
    void* get_tech_context() override;
    void set_fg_type(bool interpolated, uint64_t frame_id) override {}; // Not used by XeLL
    void set_low_latency_override(ForceReflex low_latency_override) override { this->low_latency_override = low_latency_override; };
    void set_effective_fg_state(bool effective_fg_state) override { this->effective_fg_state = effective_fg_state; };

    bool is_enabled() override { 
        if (inited_using_context)
            return true;
        else
            return low_latency_override != ForceReflex::InGame ? low_latency_override == ForceReflex::ForceEnable : low_latency_enabled; 
    };

    void get_sleep_status(SleepParams* sleep_params) override;
    void set_sleep_mode(SleepMode* sleep_mode) override;
    void sleep() override;
    void set_marker(IUnknown* pDevice, MarkerParams* marker_params) override;
    void set_async_marker(MarkerParams* marker_params) override {}; // Not used by XeLL
};