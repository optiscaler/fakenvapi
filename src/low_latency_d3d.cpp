#include "low_latency.h"

#include "low_latency_tech/ll_antilag2.h"
#include "low_latency_tech/ll_latencyflex.h"
#include "low_latency_tech/ll_xell.h"

#include "log.h"
#include "config.h"

// private
bool LowLatency::update_low_latency_tech(IUnknown* pDevice) {
    if (!currently_active_tech) {
        if (!Config::get().get_force_latencyflex()) {
            currently_active_tech = new AntiLag2();
            if (currently_active_tech->init(pDevice)) {
                spdlog::info("LowLatency algo: AntiLag 2");
                return true;
            }
            
            delete currently_active_tech;

            currently_active_tech = new XeLL();
            if (currently_active_tech->init(pDevice)) {
                spdlog::info("LowLatency algo: XeLL");
                return true;
            }
            
            delete currently_active_tech;
        }

        currently_active_tech = new LatencyFlex();
        if (currently_active_tech->init(pDevice)) {
            spdlog::info("LowLatency algo: LatencyFlex");
            return true;
        }
    }
    
    static bool last_force_latencyflex = Config::get().get_force_latencyflex();
    bool force_latencyflex = Config::get().get_force_latencyflex();
    bool change_detected = last_force_latencyflex != force_latencyflex;
    last_force_latencyflex = force_latencyflex;
    
    if (change_detected) {
        if (deinit_current_tech()) {
            return update_low_latency_tech(pDevice); // call back to reinit
        } else {
            spdlog::error("Couldn't deinitialize low latency tech");
            return false;
        }
    }

    return true;
}

void LowLatency::get_latency_result(NV_LATENCY_RESULT_PARAMS* pGetLatencyParams) {
    for (auto i = 0; i < 64; i++) {
        memcpy(&pGetLatencyParams->frameReport[i], &frame_reports[i], sizeof(frame_reports[i]));
    }
}

void LowLatency::add_marker_to_report(NV_LATENCY_MARKER_PARAMS* pSetLatencyMarkerParams) {
    auto current_timestamp = get_timestamp() / 1000;
    static auto last_sim_start = current_timestamp;
    static auto _2nd_last_sim_start = current_timestamp;
    auto current_report = &frame_reports[pSetLatencyMarkerParams->frameID % 64];
    current_report->frameID = pSetLatencyMarkerParams->frameID;
    current_report->gpuFrameTimeUs = (uint32_t)(last_sim_start - _2nd_last_sim_start);
    current_report->gpuActiveRenderTimeUs = 100;
    current_report->driverStartTime = current_timestamp;
    current_report->driverEndTime = current_timestamp + 100;
    current_report->gpuRenderStartTime = current_timestamp;
    current_report->gpuRenderEndTime = current_timestamp + 100;
    current_report->osRenderQueueStartTime = current_timestamp;
    current_report->osRenderQueueEndTime = current_timestamp + 100;
    switch (pSetLatencyMarkerParams->markerType) {
        case SIMULATION_START:
            _2nd_last_sim_start = last_sim_start;
            last_sim_start = get_timestamp() / 1000;
            current_report->simStartTime = last_sim_start;
            break;
        case SIMULATION_END:
            current_report->simEndTime = get_timestamp() / 1000;
            break;
        case RENDERSUBMIT_START:
            current_report->renderSubmitStartTime = get_timestamp() / 1000;
            break;
        case RENDERSUBMIT_END:
            current_report->renderSubmitEndTime = get_timestamp() / 1000;
            break;
        case PRESENT_START:
            current_report->presentStartTime = get_timestamp() / 1000;
            break;
        case PRESENT_END:
            current_report->presentEndTime = get_timestamp() / 1000;
            break;
        case INPUT_SAMPLE:
            current_report->inputSampleTime = get_timestamp() / 1000;
            break;
        default:
            break;
    }
}

// public
NvAPI_Status LowLatency::Sleep(IUnknown* pDevice) {
    if (!update_low_latency_tech(pDevice))
        return ERROR();

    currently_active_tech->sleep();

    return OK();
}

NvAPI_Status LowLatency::SetSleepMode(IUnknown* pDevice, NV_SET_SLEEP_MODE_PARAMS* pSetSleepModeParams) {
    if (!update_low_latency_tech(pDevice))
        return ERROR();

    SleepMode sleep_mode{};

    sleep_mode.low_latency_enabled = pSetSleepModeParams->bLowLatencyMode;
    sleep_mode.low_latency_boost = pSetSleepModeParams->bLowLatencyBoost;
    sleep_mode.minimum_interval_us = pSetSleepModeParams->minimumIntervalUs;
    sleep_mode.use_markers_to_optimize = pSetSleepModeParams->bUseMarkersToOptimize;

    currently_active_tech->set_sleep_mode(&sleep_mode);

    return OK();
}

NvAPI_Status LowLatency::GetSleepStatus(IUnknown *pDevice, NV_GET_SLEEP_STATUS_PARAMS *pGetSleepStatusParams)
{
    if (!update_low_latency_tech(pDevice))
        return ERROR();

    SleepParams sleep_params{};

    currently_active_tech->get_sleep_status(&sleep_params);

    pGetSleepStatusParams->bLowLatencyMode = sleep_params.low_latency_enabled;
    pGetSleepStatusParams->bFsVrr = sleep_params.fullscreen_vrr;
    pGetSleepStatusParams->bCplVsyncOn = sleep_params.control_panel_vsync_override;

    return OK();
}

NvAPI_Status LowLatency::SetLatencyMarker(IUnknown* pDev, NV_LATENCY_MARKER_PARAMS* pSetLatencyMarkerParams) {
    if (!update_low_latency_tech(pDev))
        return ERROR();

    update_effective_fg_state();

    update_enabled_override();

    add_marker_to_report(pSetLatencyMarkerParams);

    MarkerParams marker_params{};

    marker_params.frame_id = pSetLatencyMarkerParams->frameID;
    marker_params.marker_type = (MarkerType) pSetLatencyMarkerParams->markerType; // requires enums to match

    currently_active_tech->set_marker(pDev, &marker_params);

    spdlog::trace("{}: {}", marker_to_name(pSetLatencyMarkerParams->markerType), pSetLatencyMarkerParams->frameID);

    return NVAPI_OK;
}

NvAPI_Status LowLatency::SetAsyncFrameMarker(ID3D12CommandQueue* pCommandQueue, NV_ASYNC_FRAME_MARKER_PARAMS* pSetAsyncFrameMarkerParams) {
    if (!currently_active_tech) // can't init using ID3D12CommandQueue, can only check if available
        return ERROR();

    MarkerParams marker_params{};

    marker_params.frame_id = pSetAsyncFrameMarkerParams->frameID;
    marker_params.marker_type = (MarkerType) pSetAsyncFrameMarkerParams->markerType; // requires enums to match

    if (marker_params.marker_type == MarkerType::OUT_OF_BAND_PRESENT_START) {
        constexpr size_t history_size = 12;
        static size_t counter = 0;
        static NvU64 previous_frame_ids[history_size] = {};

        previous_frame_ids[counter % history_size] = pSetAsyncFrameMarkerParams->frameID;
        counter++;

        int repeat_count = 0;

        for (size_t i = 1; i < history_size; i++) {
            // won't catch repeat frame ids across array wrap around
            if (previous_frame_ids[i] == previous_frame_ids[i - 1]) {
                repeat_count++;
            }
        }

        if (fg && repeat_count == 0) fg = false;
        else if (!fg && repeat_count >= history_size / 2 - 1) fg = true;

        update_effective_fg_state();
    }

    currently_active_tech->set_async_marker(&marker_params);

    spdlog::trace("Async {}: {}", marker_to_name(pSetAsyncFrameMarkerParams->markerType), pSetAsyncFrameMarkerParams->frameID);

    return NVAPI_OK;
}

NvAPI_Status LowLatency::GetLatency(IUnknown* pDev, NV_LATENCY_RESULT_PARAMS* pGetLatencyParams) {
    if (!update_low_latency_tech(pDev))
        return ERROR();

    get_latency_result(pGetLatencyParams);

    return OK();
}
