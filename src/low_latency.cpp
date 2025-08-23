#include "low_latency.h"

// private
void LowLatency::update_effective_fg_state() {
    if (!currently_active_tech)
        return;

    if (forced_fg.has_value())
        currently_active_tech->set_effective_fg_state(forced_fg.value());
    else
        currently_active_tech->set_effective_fg_state(fg);
}

void LowLatency::update_enabled_override() {
    if (!currently_active_tech)
        return;

    currently_active_tech->set_low_latency_override(Config::get().get_force_reflex());
}

// public
bool LowLatency::deinit_current_tech() {
    if (currently_active_tech) {
        currently_active_tech->deinit();

        delete currently_active_tech;
        currently_active_tech = nullptr;

        return true;
    }

    return false;
}

bool LowLatency::get_low_latency_context(void** low_latency_context, Mode* low_latency_tech) {
    if (!currently_active_tech || !low_latency_context || !low_latency_tech)
        return false;

    *low_latency_context = currently_active_tech->get_tech_context();
    *low_latency_tech = currently_active_tech->get_mode();

    // We are during deinit, don't let app use the context
    if (delay_deinit > 0) {
        *low_latency_context = nullptr;
        delay_deinit = 1;
    }

    return true;
}

bool LowLatency::set_low_latency_context(void* low_latency_context, Mode low_latency_tech) {
    forced_low_latency_context = low_latency_context;
    forced_low_latency_tech = low_latency_tech;

    deinit_current_tech();

    // Only D3D
    if (forced_low_latency_context)
        return update_low_latency_tech((IUnknown*) nullptr);
    else
        return true; // no device, low latency will need to reinit itself on the next reflex call
}