// Copyright 2026 Amadeus Demarzi
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#define VIAL_KEYBOARD_UID {0x2D, 0x50, 0x00, 0xBF, 0x2D, 0x41, 0xEC, 0x79}

#define VIAL_UNLOCK_COMBO_ROWS {0, 0}
#define VIAL_UNLOCK_COMBO_COLS {0, 11}

#define DYNAMIC_KEYMAP_LAYER_COUNT 4

// RGB Test is the persisted mode slot owned by the reactive renderer. These
// defaults are applied when RGBLight EEPROM is initialized or cleared; saved
// lighting choices are otherwise left alone.
#define RGBLIGHT_DEFAULT_ON true
#define RGBLIGHT_DEFAULT_MODE RGBLIGHT_MODE_RGB_TEST

// Reactive animation tuning is intentionally firmware-only. Vial continues to
// expose the standard RGB hue, saturation, brightness, and on/off controls.
#define CLOUD_REACTIVE_FRAME_INTERVAL_MS 16U
#define CLOUD_REACTIVE_SPEED_FEEDBACK_MS 1000U
#define CLOUD_REACTIVE_HIT_INCREMENT 160U
#define CLOUD_REACTIVE_NEIGHBOR_INCREMENT 56U
#define CLOUD_REACTIVE_FADE_STEP (2U << MIN(rgblight_get_speed(), 3U))
#define CLOUD_REACTIVE_HOLD_LEVEL 48U
#define CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL 16U
#define CLOUD_REACTIVE_REVERSE_LED_ORDER 0
