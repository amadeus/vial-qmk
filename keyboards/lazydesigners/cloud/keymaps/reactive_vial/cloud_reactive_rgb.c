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

#if CLOUD_REACTIVE_HOLD_LEVEL == 0
#    error "Cloud reactive fade settings are invalid"
#endif

static bool     was_active                 = false;
static bool     dirty                      = false;
static bool     animating                  = false;
static bool     speed_feedback_pending     = false;
static bool     speed_feedback_active      = false;
static uint32_t last_frame                 = 0;
static uint32_t speed_feedback_timer       = 0;
static uint8_t  speed_feedback_led_count   = 0;
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

static void set_energy_floor(uint8_t led, uint8_t minimum) {
    if (energy[led] < minimum) {
        energy[led] = minimum;
    }
}

static void apply_hold_levels(void) {
    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!matrix_is_on(row, col)) {
                continue;
            }

            const uint8_t center = pgm_read_byte(&led_for_column[col]);

            set_energy_floor(center, CLOUD_REACTIVE_HOLD_LEVEL);
            if (center > 0) {
                set_energy_floor(center - 1, CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL);
            }
            if (center + 1 < RGBLIGHT_LED_COUNT) {
                set_energy_floor(center + 1, CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL);
            }
        }
    }
}

static uint8_t scaled_value_for_energy(uint8_t level, uint8_t max_value) {
    return ((uint16_t)level * (max_value + 1U)) >> 8;
}

static void decay(uint32_t elapsed) {
    const uint32_t frames      = elapsed / CLOUD_REACTIVE_FRAME_INTERVAL_MS;
    const uint32_t fade_amount = MIN(frames * CLOUD_REACTIVE_FADE_STEP, UINT8_MAX);

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        energy[i] = energy[i] > fade_amount ? energy[i] - fade_amount : 0;
    }

    last_frame += frames * CLOUD_REACTIVE_FRAME_INTERVAL_MS;
}

static void render(uint8_t max_value) {
    const uint8_t hue        = rgblight_get_hue();
    const uint8_t saturation = rgblight_get_sat();

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        const uint8_t value = scaled_value_for_energy(energy[i], max_value);
        const rgb_t   rgb   = hsv_to_rgb((hsv_t){hue, saturation, value});

        rgblight_driver.set_color(driver_index(i), rgb.r, rgb.g, rgb.b);
    }

    rgblight_set();
    animating = has_energy();
}

static void start_speed_feedback(void) {
    const uint8_t speed = MIN(rgblight_get_speed(), 3U);

    clear();
    speed_feedback_led_count = 1U + ((uint16_t)(RGBLIGHT_LED_COUNT - 1U) * speed + 1U) / 3U;
    speed_feedback_timer     = timer_read32();
    last_frame              = speed_feedback_timer;
    speed_feedback_active    = true;

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        energy[i] = i < speed_feedback_led_count ? UINT8_MAX : 0;
    }

    animating = true;
    rgblight_timer_disable();
    render(rgblight_get_val());
}

static void update_speed_feedback(void) {
    const uint32_t elapsed = timer_elapsed32(last_frame);

    if (elapsed >= CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        decay(elapsed);
    }

    if (timer_elapsed32(speed_feedback_timer) < CLOUD_REACTIVE_SPEED_FEEDBACK_MS) {
        for (uint8_t i = 0; i < speed_feedback_led_count; i++) {
            energy[i] = UINT8_MAX;
        }
    }

    if (elapsed >= CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        render(rgblight_get_val());
    }
}

void cloud_reactive_rgb_activate(const keyrecord_t *record) {
    if (!record->event.pressed) {
        return;
    }

    const bool needs_enable = !rgblight_is_enabled();
    const bool needs_mode   = rgblight_get_mode() != CLOUD_REACTIVE_MODE;

    if (needs_enable) {
        // Enable in RAM first, then let the mode write persist both changes.
        rgblight_enable_noeeprom();
    }
    if (needs_enable || needs_mode) {
        rgblight_mode(CLOUD_REACTIVE_MODE);
    }

    record_keypress(record);
}

void cloud_reactive_rgb_process_record(uint16_t keycode, const keyrecord_t *record) {
    if (record->event.pressed && (keycode == RGB_SPD || keycode == RGB_SPI)) {
        speed_feedback_pending = true;
    }

    if (record->event.pressed && is_active() && !speed_feedback_pending && !speed_feedback_active) {
        record_keypress(record);
    }
}

void cloud_reactive_rgb_task(void) {
    const bool reactive_active = is_active();

    if (speed_feedback_pending) {
        speed_feedback_pending = false;

        if (rgblight_is_enabled()) {
            start_speed_feedback();
        }
    }

    if (speed_feedback_active) {
        if (!rgblight_is_enabled()) {
            speed_feedback_active = false;
            clear();
        } else {
            update_speed_feedback();

            if (has_energy()) {
                return;
            }

            speed_feedback_active = false;
        }

        if (!reactive_active) {
            rgblight_mode_noeeprom(rgblight_get_mode());
            return;
        }

        dirty = true;
    }

    if (!reactive_active) {
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

    const uint8_t max_value = rgblight_get_val();

    if (elapsed >= CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        decay(elapsed);
    }

    apply_hold_levels();
    render(max_value);
    dirty = false;
}
