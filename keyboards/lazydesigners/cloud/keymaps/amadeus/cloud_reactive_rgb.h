// Copyright 2026 Amadeus
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

// These defaults make the module directly reusable by another Cloud keymap.
// Override any of them in config.h to tune the effect.
#ifndef CLOUD_REACTIVE_FRAME_INTERVAL_MS
#    define CLOUD_REACTIVE_FRAME_INTERVAL_MS 16U
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

#ifndef CLOUD_REACTIVE_REVERSE_LED_ORDER
#    define CLOUD_REACTIVE_REVERSE_LED_ORDER 0
#endif

void cloud_reactive_rgb_activate(const keyrecord_t *record);
void cloud_reactive_rgb_process_record(const keyrecord_t *record);
void cloud_reactive_rgb_task(void);
