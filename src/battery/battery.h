
#ifndef BATTERY_H
#define BATTERY_H

#include <stdbool.h>

typedef struct
{
    int percentage;       /* Battery charge estimate: 0–100 */
    float voltage;        /* Battery voltage, or 0 if unavailable */
    bool is_charging;
    bool is_valid;
} BatteryStatus;

/* Initialize the selected battery backend. */
void battery_init(void);

/* Refresh the battery reading when needed. */
void battery_update(void);

/* Return the most recent reading. */
BatteryStatus battery_get_status(void);

#endif
