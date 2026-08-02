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

#if CLOUD_REACTIVE_HIT_INCREMENT > UINT8_MAX || CLOUD_REACTIVE_NEIGHBOR_INCREMENT > UINT8_MAX || CLOUD_REACTIVE_HOLD_LEVEL > UINT8_MAX || CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL > UINT8_MAX
#    error "Cloud reactive energy settings must fit in a uint8_t"
#endif

static bool     was_active                 = false;
static bool     dirty                      = false;
static bool     animating                  = false;
static uint32_t last_frame                 = 0;
static uint8_t  energy[RGBLIGHT_LED_COUNT] = {0};
static uint16_t wide_key_rows[MATRIX_ROWS] = {0};

// Photo-calibrated Q4 positions: 16 units equal one LED. Fractional positions
// blend the existing center-and-neighbor profile between adjacent pixels. The
// strip starts near Q, so Tab uses the nearest available position. Matrix
// column 12 is the encoder push at the far-left end of the board.
static const uint8_t PROGMEM led_position_q4_for_column[MATRIX_COLS] = {
    0, 6, 28, 49, 70, 92, 113, 134, 156, 177, 198, 219, 0,
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

static void set_energy_floor(uint8_t led, uint8_t minimum) {
    if (energy[led] < minimum) {
        energy[led] = minimum;
    }
}

static uint8_t interpolated_high(uint8_t amount, uint8_t fraction) {
    return ((uint16_t)amount * fraction + 8U) >> 4;
}

static void apply_energy_level(int8_t led, uint16_t amount, bool as_floor) {
    if (led < 0 || led >= RGBLIGHT_LED_COUNT || amount == 0) {
        return;
    }

    const uint8_t level = amount > UINT8_MAX ? UINT8_MAX : amount;

    if (as_floor) {
        set_energy_floor((uint8_t)led, level);
    } else {
        add_energy((uint8_t)led, level);
    }
}

static void apply_profile(uint8_t position_q4, uint8_t center_amount, uint8_t neighbor_amount, bool wide, bool as_floor) {
    if (position_q4 > ((RGBLIGHT_LED_COUNT - 1U) << 4)) {
        return;
    }

    const int8_t  base          = position_q4 >> 4;
    const uint8_t fraction      = position_q4 & 0x0FU;
    const uint8_t center_high   = interpolated_high(center_amount, fraction);
    const uint8_t center_low    = center_amount - center_high;
    const uint8_t neighbor_high = interpolated_high(neighbor_amount, fraction);
    const uint8_t neighbor_low  = neighbor_amount - neighbor_high;

    const uint16_t base_level = (uint16_t)center_low + neighbor_high;
    const uint16_t next_level = (uint16_t)center_high + neighbor_low;

    // This linearly shifts either the normal three-pixel profile or a wide
    // four-pixel profile. The wide form keeps two center lanes at equal energy
    // without double-brightening their overlap.
    apply_energy_level(base - 1, neighbor_low, as_floor);
    apply_energy_level(base, base_level, as_floor);
    apply_energy_level(base + 1, wide ? center_amount : next_level, as_floor);
    apply_energy_level(base + 2, wide ? next_level : neighbor_high, as_floor);
    if (wide) {
        apply_energy_level(base + 3, neighbor_high, as_floor);
    }
}

static bool is_spacebar_keycode(uint16_t keycode) {
    return get_tap_keycode(keycode) == KC_SPC;
}

static void update_wide_key_state(bool wide, const keyrecord_t *record) {
    if (!IS_KEYEVENT(record->event) || record->event.key.row >= MATRIX_ROWS || record->event.key.col >= MATRIX_COLS) {
        return;
    }

    const uint16_t mask = (uint16_t)1U << record->event.key.col;

    if (record->event.pressed && wide) {
        wide_key_rows[record->event.key.row] |= mask;
    } else {
        wide_key_rows[record->event.key.row] &= ~mask;
    }
}

static bool is_wide_key_held(uint8_t row, uint8_t col) {
    return wide_key_rows[row] & ((uint16_t)1U << col);
}

static void record_keypress(const keyrecord_t *record, bool wide) {
    if (!IS_KEYEVENT(record->event) || record->event.key.row >= MATRIX_ROWS || record->event.key.col >= MATRIX_COLS) {
        return;
    }

    const uint8_t position_q4 = pgm_read_byte(&led_position_q4_for_column[record->event.key.col]);

    apply_profile(position_q4, CLOUD_REACTIVE_HIT_INCREMENT, CLOUD_REACTIVE_NEIGHBOR_INCREMENT, wide, false);

    if (!animating) {
        last_frame = timer_read32();
    }
    dirty     = true;
    animating = true;
}

static void apply_hold_levels(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!matrix_is_on(row, col)) {
                continue;
            }

            const uint8_t position_q4 = pgm_read_byte(&led_position_q4_for_column[col]);

            apply_profile(position_q4, CLOUD_REACTIVE_HOLD_LEVEL, CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL, is_wide_key_held(row, col), true);
        }
    }
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
    record_keypress(record, false);
}

void cloud_reactive_rgb_process_record(uint16_t keycode, const keyrecord_t *record) {
    const bool wide = is_spacebar_keycode(keycode);

    update_wide_key_state(wide, record);

    if (record->event.pressed && is_active()) {
        record_keypress(record, wide);
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

    apply_hold_levels();
    render();
    dirty = false;
}
