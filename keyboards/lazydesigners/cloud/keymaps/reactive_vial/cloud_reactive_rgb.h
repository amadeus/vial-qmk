// Copyright 2026 Amadeus Demarzi
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// Defaults keep the module reusable while allowing a keymap's config.h to
// tune the compiled behavior.
#ifndef CLOUD_REACTIVE_FRAME_INTERVAL_MS
#    define CLOUD_REACTIVE_FRAME_INTERVAL_MS 16U
#endif

#ifndef CLOUD_REACTIVE_SPEED_FEEDBACK_MS
#    define CLOUD_REACTIVE_SPEED_FEEDBACK_MS 1000U
#endif

#ifndef CLOUD_REACTIVE_HIT_INCREMENT
#    define CLOUD_REACTIVE_HIT_INCREMENT 160U
#endif

#ifndef CLOUD_REACTIVE_NEIGHBOR_INCREMENT
#    define CLOUD_REACTIVE_NEIGHBOR_INCREMENT 56U
#endif

#ifndef CLOUD_REACTIVE_FADE_STEP
#    define CLOUD_REACTIVE_FADE_STEP 3U
#endif

#ifndef CLOUD_REACTIVE_HOLD_LEVEL
#    define CLOUD_REACTIVE_HOLD_LEVEL 96U
#endif

#ifndef CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL
#    define CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL 32U
#endif

#ifndef CLOUD_REACTIVE_REVERSE_LED_ORDER
#    define CLOUD_REACTIVE_REVERSE_LED_ORDER 0
#endif

void cloud_reactive_rgb_activate(const keyrecord_t *record);
void cloud_reactive_rgb_process_record(uint16_t keycode, const keyrecord_t *record);
void cloud_reactive_rgb_task(void);
