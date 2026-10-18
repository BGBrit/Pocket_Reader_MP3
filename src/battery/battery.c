
#include "battery.h"

/*
 * Battery status provider.
 *
 * Desktop / SDL build:
 *   Uses a simulated battery reading for UI development.
 *
 * ESP32 build:
 *   Replace the simulated values in battery_update() with
 *   readings from the selected battery-monitoring hardware.
 */

static BatteryStatus battery_status = {
    .percentage = 78,
    .voltage = 0.0f,
    .is_charging = false,
    .is_valid = true
};

void battery_init(void)
{
    /* Reset to a predictable simulated reading for now. */
    battery_status.percentage = 78;
    battery_status.voltage = 0.0f;
    battery_status.is_charging = false;
    battery_status.is_valid = true;
}

void battery_update(void)
{
    /*
     * Desktop simulation: keep the reading stable.
     *
     * Later, read the actual battery voltage / fuel gauge here
     * and update battery_status accordingly.
     */
}

BatteryStatus battery_get_status(void)
{
    return battery_status;
}
