// Copyright 2026 Amadeus Demarzi
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#include "cloud_reactive_rgb.h"

#define CLOUD_REACTIVE_MODE RGBLIGHT_MODE_RGB_TEST

#if RGBLIGHT_LED_COUNT != 15
#    error "Cloud reactive RGB map expects 15 LEDs"
#endif

#if MATRIX_COLS != 13
#    error "Cloud reactive RGB map expects 13 matrix columns"
#endif

static bool     was_active                 = false;
static bool     dirty                      = false;
static bool     animating                  = false;
static uint32_t last_frame                 = 0;
static uint8_t  energy[RGBLIGHT_LED_COUNT] = {0};

// Cloud's twelve key columns are spread across its 15-pixel light bar. Matrix
// column 12 is the encoder push, which sits at the far-left end of the board.
static const uint8_t PROGMEM led_for_column[MATRIX_COLS] = {
    0, 1, 3, 4, 5, 6, 8, 9, 10, 11, 13, 14, 0,
};

static bool is_active(void) {
    return rgblight_is_enabled() && rgblight_get_mode() == CLOUD_REACTIVE_MODE;
}

static uint8_t driver_index(uint8_t logical_index) {
#if CLOUD_REACTIVE_REVERSE_LED_ORDER
    return RGBLIGHT_LED_COUNT - 1 - logical_index;
#else
    return logical_index;
#endif
}

static void clear(void) {
    memset(energy, 0, sizeof(energy));
    animating = false;
}

static bool has_energy(void) {
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        if (energy[i] != 0) {
            return true;
        }
    }

    return false;
}

static void add_energy(uint8_t led, uint8_t amount) {
    const uint16_t increased = energy[led] + amount;

    energy[led] = increased > UINT8_MAX ? UINT8_MAX : increased;
}

static void record_keypress(const keyrecord_t *record) {
    if (!IS_KEYEVENT(record->event) || record->event.key.row >= MATRIX_ROWS || record->event.key.col >= MATRIX_COLS) {
        return;
    }

    const uint8_t center = pgm_read_byte(&led_for_column[record->event.key.col]);

    add_energy(center, CLOUD_REACTIVE_HIT_INCREMENT);
    if (center > 0) {
        add_energy(center - 1, CLOUD_REACTIVE_NEIGHBOR_INCREMENT);
    }
    if (center + 1 < RGBLIGHT_LED_COUNT) {
        add_energy(center + 1, CLOUD_REACTIVE_NEIGHBOR_INCREMENT);
    }

    if (!animating) {
        last_frame = timer_read32();
    }
    dirty     = true;
    animating = true;
}

static void decay(uint32_t elapsed) {
    const uint32_t frames      = elapsed / CLOUD_REACTIVE_FRAME_INTERVAL_MS;
    const uint32_t fade_amount = MIN(frames * CLOUD_REACTIVE_FADE_STEP, UINT8_MAX);

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        energy[i] = energy[i] > fade_amount ? energy[i] - fade_amount : 0;
    }

    last_frame += frames * CLOUD_REACTIVE_FRAME_INTERVAL_MS;
}

static void render(void) {
    const uint8_t hue        = rgblight_get_hue();
    const uint8_t saturation = rgblight_get_sat();
    const uint8_t max_value  = rgblight_get_val();

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        const uint8_t value = ((uint16_t)energy[i] * (max_value + 1U)) >> 8;
        const rgb_t   rgb   = hsv_to_rgb((hsv_t){hue, saturation, value});

        rgblight_driver.set_color(driver_index(i), rgb.r, rgb.g, rgb.b);
    }

    rgblight_set();
    animating = has_energy();
}

void cloud_reactive_rgb_activate(const keyrecord_t *record) {
    if (!record->event.pressed || !rgblight_is_enabled()) {
        return;
    }

    rgblight_mode(CLOUD_REACTIVE_MODE);
    record_keypress(record);
}

void cloud_reactive_rgb_process_record(const keyrecord_t *record) {
    if (record->event.pressed && is_active()) {
        record_keypress(record);
    }
}

void cloud_reactive_rgb_task(void) {
    if (!is_active()) {
        if (was_active || dirty) {
            clear();
        }
        was_active = false;
        dirty      = false;
        return;
    }

    // RGB Test is only a persisted mode slot here; prevent its stock animation
    // from competing with the module-owned reactive frames after entry or wake.
    rgblight_timer_disable();

    if (!was_active) {
        last_frame = timer_read32();
        dirty      = true;
        was_active = true;
    }

    if (!dirty && !animating) {
        return;
    }

    const uint32_t elapsed = timer_elapsed32(last_frame);
    if (!dirty && elapsed < CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        return;
    }

    if (elapsed >= CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        decay(elapsed);
    }

    render();
    dirty = false;
}
