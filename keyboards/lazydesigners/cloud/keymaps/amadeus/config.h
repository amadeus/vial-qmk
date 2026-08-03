// Copyright 2026 Amadeus
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define RGBLIGHT_IDLE_TIMEOUT_MS 30000U

// The reactive effect uses QMK's RGB Test mode as its selectable/persisted slot.
#define CLOUD_REACTIVE_FRAME_INTERVAL_MS 16U
#define CLOUD_REACTIVE_SPEED_FEEDBACK_MS 1000U
#define CLOUD_REACTIVE_HIT_INCREMENT 160U
#define CLOUD_REACTIVE_NEIGHBOR_INCREMENT 56U
#define CLOUD_REACTIVE_FADE_STEP (2U << MIN(rgblight_get_speed(), 3U))
#define CLOUD_REACTIVE_HOLD_LEVEL 48U
#define CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL 16U

// The PCB does not document which end of the light bar is LED 0. Set this to 1
// if the first hardware test shows key presses mirrored across the bar.
#define CLOUD_REACTIVE_REVERSE_LED_ORDER 0
