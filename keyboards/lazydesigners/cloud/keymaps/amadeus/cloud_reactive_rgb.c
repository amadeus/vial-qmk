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

#if CLOUD_IDLE_BOUNCE_LEVEL == 0 || CLOUD_IDLE_BOUNCE_LEVEL > UINT8_MAX || CLOUD_IDLE_BOUNCE_TRAIL_DECAY > UINT8_MAX
#    error "Cloud idle bounce energy settings must fit in a uint8_t"
#endif

#if CLOUD_IDLE_BOUNCE_SWEEP_MS == 0 || CLOUD_IDLE_BOUNCE_SWEEP_MS > 32767 || CLOUD_IDLE_BOUNCE_RAMP_MS == 0 || CLOUD_IDLE_BOUNCE_FADE_OUT_MS == 0 || CLOUD_REACTIVE_DASH_FULL_SPAN_MS > UINT16_MAX
#    error "Cloud idle bounce timing settings are invalid"
#endif

#if CLOUD_IDLE_BOUNCE_TRAIL_SCALE > 256
#    error "Cloud idle bounce trail scale is out of 256"
#endif

#if CLOUD_IDLE_BOUNCE_HANDOFF_SCALE >= 256
#    error "Cloud idle bounce handoff scale must be below 256"
#endif

#define DOT_SPAN_Q8 ((uint16_t)(RGBLIGHT_LED_COUNT - 1U) << 8)
#define DOT_RADIUS_Q8 512U

static bool     was_active                 = false;
static bool     idle_bouncing              = false;
static uint8_t  idle_bounce_ramp_from      = 0;
static uint8_t  handoff_level              = 0;
static uint16_t idle_bounce_phase          = 0;
static uint32_t idle_bounce_timer          = 0;
static bool     dashing                    = false;
static bool     dash_wide                  = false;
static uint16_t dash_from_q8               = 0;
static uint16_t dash_last_q8               = 0;
static uint8_t  dash_target_q4             = 0;
static uint16_t last_target_q8             = 0;
static uint16_t trail_q8[RGBLIGHT_LED_COUNT] = {0};
static uint16_t dash_duration              = 0;
static uint32_t dash_timer                 = 0;
static bool     dirty                      = false;
static bool     animating                  = false;
static bool     speed_feedback_pending     = false;
static bool     speed_feedback_active      = false;
static uint32_t last_frame                 = 0;
static uint32_t speed_feedback_timer       = 0;
static uint8_t  speed_feedback_led_count   = 0;
static uint8_t  energy[RGBLIGHT_LED_COUNT] = {0};
static uint16_t wide_key_rows[MATRIX_ROWS] = {0};

// Photo-calibrated Q4 positions: 16 units equal one LED. Fractional positions
// blend the existing center-and-neighbor profile between adjacent pixels. The
// strip starts near Q, so Tab uses the nearest available position. Matrix
// column 12 is the encoder push at the far-left end of the board.
static const uint8_t PROGMEM led_position_q4_for_column[MATRIX_COLS] = {
    0, 6, 28, 49, 70, 92, 113, 134, 156, 177, 198, 219, 0,
};

// (1 - cos) / 2 eased from 0 to DOT_SPAN_Q8 in 33 evenly spaced samples. It is
// one pendulum half swing across the bar; the return swing mirrors it. Q8
// precision keeps the slope between samples from stepping, which shows up as
// uneven speed.
static const uint16_t PROGMEM cosine_ease_q8[33] = {
    0,    9,    34,   77,   136,  212,  302,  407,  525,  655,  796,  947,  1106, 1272, 1442, 1616, 1792,
    1968, 2142, 2312, 2478, 2637, 2788, 2929, 3059, 3177, 3282, 3372, 3448, 3507, 3550, 3575, 3584,
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
    animating     = false;
    idle_bouncing = false;
    dashing       = false;
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

static void    start_dash(uint8_t position_q4, bool wide);
static uint8_t brightest_led(uint8_t *peak);

static void land_keypress(uint8_t position_q4, bool wide) {
    apply_profile(position_q4, CLOUD_REACTIVE_HIT_INCREMENT, CLOUD_REACTIVE_NEIGHBOR_INCREMENT, wide, false);

    // Wide keys light two center lanes, so the next dash leaves from between them.
    last_target_q8 = (uint16_t)(position_q4 + (wide ? 8U : 0U)) << 4;

    // The bounce takes over once the strip fades to a fixed fraction of how
    // bright it is right now, so every key hands off at the same point in its
    // fade regardless of how its light falls between pixels.
    uint8_t peak;

    brightest_led(&peak);
    handoff_level = ((uint16_t)peak * CLOUD_IDLE_BOUNCE_HANDOFF_SCALE) >> 8;

    if (!animating) {
        last_frame = timer_read32();
    }
    dirty     = true;
    animating = true;
}

static void finish_dash(void) {
    dashing = false;
    land_keypress(dash_target_q4, dash_wide);
}

static void record_keypress(const keyrecord_t *record, bool wide) {
    if (!IS_KEYEVENT(record->event) || record->event.key.row >= MATRIX_ROWS || record->event.key.col >= MATRIX_COLS) {
        return;
    }

    const uint8_t position_q4 = pgm_read_byte(&led_position_q4_for_column[record->event.key.col]);

    // Every keypress sends the dot from the current light over to the key, and
    // the hit lands when it arrives. With nothing lit there's nowhere to slide
    // from, so the hit lands right away.
    if (idle_bouncing || dashing || has_energy()) {
        start_dash(position_q4, wide);
        return;
    }

    land_keypress(position_q4, wide);
}

// Returns whether any key is held.
static bool apply_hold_levels(void) {
    bool holding = false;

    for (uint8_t row = 0; row < MATRIX_ROWS; row++) {
        for (uint8_t col = 0; col < MATRIX_COLS; col++) {
            if (!matrix_is_on(row, col)) {
                continue;
            }

            const uint8_t position_q4 = pgm_read_byte(&led_position_q4_for_column[col]);

            holding = true;

            // The key the dot is heading to stays dark until the dot arrives.
            if (dashing && position_q4 == dash_target_q4) {
                continue;
            }

            apply_profile(position_q4, CLOUD_REACTIVE_HOLD_LEVEL, CLOUD_REACTIVE_HOLD_NEIGHBOR_LEVEL, is_wide_key_held(row, col), true);
        }
    }

    return holding;
}

static uint8_t scaled_value_for_energy(uint8_t level, uint8_t max_value) {
    return ((uint16_t)level * (max_value + 1U)) >> 8;
}

static void decay(uint32_t elapsed, uint8_t fade_step) {
    const uint32_t frames      = elapsed / CLOUD_REACTIVE_FRAME_INTERVAL_MS;
    const uint32_t fade_amount = MIN(frames * fade_step, UINT8_MAX);

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

static uint8_t brightest_led(uint8_t *peak) {
    uint8_t led = 0;

    *peak = 0;
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        if (energy[i] > *peak) {
            *peak = energy[i];
            led   = i;
        }
    }

    return led;
}

// The moving dot works in Q8 (256 units per LED) so its motion stays even at
// low speeds; key hits keep their Q4 profiles.
static uint16_t half_swing_q8(uint16_t time) {
    const uint16_t step = ((uint32_t)time << 13) / CLOUD_IDLE_BOUNCE_SWEEP_MS;
    const uint16_t from = pgm_read_word(&cosine_ease_q8[step >> 8]);
    const uint16_t to   = pgm_read_word(&cosine_ease_q8[(step >> 8) + 1]);

    return from + (((to - from) * (step & 0xFFU)) >> 8);
}

// Earliest time in the half swing that reaches position_q8. The swing only
// moves forward, so a binary search finds it.
static uint16_t half_swing_time(uint16_t position_q8) {
    uint16_t low  = 0;
    uint16_t high = CLOUD_IDLE_BOUNCE_SWEEP_MS - 1U;

    while (low < high) {
        const uint16_t mid = low + ((high - low) >> 1);

        if (half_swing_q8(mid) < position_q8) {
            low = mid + 1U;
        } else {
            high = mid;
        }
    }

    return low;
}

static uint16_t bounce_position_q8(uint32_t time) {
    const uint16_t phase = time % (2UL * CLOUD_IDLE_BOUNCE_SWEEP_MS);

    if (phase < CLOUD_IDLE_BOUNCE_SWEEP_MS) {
        return half_swing_q8(phase);
    }

    return DOT_SPAN_Q8 - half_swing_q8(phase - CLOUD_IDLE_BOUNCE_SWEEP_MS);
}

static void start_idle_bounce(uint16_t position_q8, uint8_t peak) {
    // Head for the farther end first; the exact middle goes toward LED 0.
    if (position_q8 < (DOT_SPAN_Q8 >> 1)) {
        idle_bounce_phase = half_swing_time(position_q8);
    } else {
        idle_bounce_phase = CLOUD_IDLE_BOUNCE_SWEEP_MS + half_swing_time(DOT_SPAN_Q8 - position_q8);
    }

    // Whatever is still lit becomes the start of the trail.
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        trail_q8[i] = (uint16_t)energy[i] << 8;
    }

    idle_bounce_timer     = timer_read32();
    idle_bounce_ramp_from = peak;
    idle_bouncing         = true;
}

static uint8_t idle_bounce_level(uint32_t elapsed) {
    uint8_t level = CLOUD_IDLE_BOUNCE_LEVEL;

    // Relight from the final reactive brightness instead of popping in.
    if (elapsed < CLOUD_IDLE_BOUNCE_RAMP_MS) {
        const int16_t delta = (int16_t)CLOUD_IDLE_BOUNCE_LEVEL - idle_bounce_ramp_from;

        level = idle_bounce_ramp_from + (int16_t)((int32_t)delta * (int32_t)elapsed / (int32_t)CLOUD_IDLE_BOUNCE_RAMP_MS);
    }

#ifdef RGBLIGHT_IDLE_TIMEOUT_MS
    // Dim out ahead of the idle timeout so the bar doesn't cut off mid-swing.
    const uint32_t idle = last_input_activity_elapsed();

    if (idle + CLOUD_IDLE_BOUNCE_FADE_OUT_MS > RGBLIGHT_IDLE_TIMEOUT_MS) {
        const uint32_t remaining = idle < RGBLIGHT_IDLE_TIMEOUT_MS ? RGBLIGHT_IDLE_TIMEOUT_MS - idle : 0;

        level = (uint32_t)level * remaining / CLOUD_IDLE_BOUNCE_FADE_OUT_MS;
    }
#endif

    return level;
}

static uint8_t trail_scaled(uint8_t level) {
    return ((uint16_t)level * CLOUD_IDLE_BOUNCE_TRAIL_SCALE) >> 8;
}

// Triangular glow with a two-LED radius. Sampled at whole-LED spacing, its
// pixels always sum to the same total and their brightness-weighted center is
// exactly the dot's position, so the dot glides at an even speed instead of
// pulsing or wobbling as it crosses each LED.
// Glow weight of pixel `led` for a dot at position_q8, from 0 to DOT_RADIUS_Q8.
static uint16_t dot_weight(uint8_t led, uint16_t position_q8) {
    const uint16_t led_q8   = (uint16_t)led << 8;
    const uint16_t distance = led_q8 > position_q8 ? led_q8 - position_q8 : position_q8 - led_q8;

    return distance < DOT_RADIUS_Q8 ? DOT_RADIUS_Q8 - distance : 0;
}

static void apply_dot(uint16_t position_q8, uint8_t level, bool as_floor) {
    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        const uint16_t weight = dot_weight(i, position_q8);

        if (weight != 0) {
            apply_energy_level(i, ((uint32_t)level * weight) >> 9, as_floor);
        }
    }
}

// Lay the trail along the whole path the dot covered since the last frame, at
// most a quarter LED apart. A dash covers more than a pixel per frame, so
// stamping only where the dot lands each frame left a striped trail.
static void smear_trail(uint16_t from_q8, uint16_t to_q8, uint8_t level) {
    const int16_t  travel   = (int16_t)to_q8 - (int16_t)from_q8;
    const uint16_t distance = travel < 0 ? -travel : travel;
    const uint8_t  steps    = (distance >> 6) + 1U;

    for (uint8_t step = 1; step <= steps; step++) {
        apply_dot(from_q8 + (int16_t)((int32_t)travel * step / steps), level, true);
    }
}

// The energy buffer only holds the trail. The dot leaves a dimmer copy of
// itself there, then is drawn over it for this frame only, so it sits at its
// exact sub-pixel position instead of lingering on the pixel it just left.
static void draw_dot(uint16_t position_q8, uint8_t level) {
    uint8_t trail[RGBLIGHT_LED_COUNT];

    memcpy(trail, energy, sizeof(trail));
    apply_dot(position_q8, level, false);
    render(rgblight_get_val());
    memcpy(energy, trail, sizeof(energy));
}

// Advance the bounce trail by `frames` and draw. The trail is a running blend
// of where the dot has been: each frame it keeps a fixed fraction of itself
// and takes in a little of the dot's glow, like motion blur. That changes
// smoothly from frame to frame, where stamping the dot into the trail made
// each pixel jump as the dot crossed it. A stationary dot builds its trail up
// to CLOUD_IDLE_BOUNCE_TRAIL_SCALE of its own brightness; a moving one leaves
// less behind, so the trail glows brightest where the swing slows at each end.
static void step_idle_bounce(uint32_t frames) {
    const uint32_t elapsed     = timer_elapsed32(idle_bounce_timer);
    const uint16_t position_q8 = bounce_position_q8(elapsed + idle_bounce_phase);
    const uint8_t  level       = idle_bounce_level(elapsed);
    const uint16_t gain        = (uint16_t)trail_scaled(level) * (256U - CLOUD_IDLE_BOUNCE_TRAIL_DECAY);

    // Anything left after 32 frames has already faded to zero.
    const uint8_t fade_frames = frames > 32U ? 32U : frames;

    for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
        uint16_t trail = trail_q8[i];

        for (uint8_t frame = 0; frame < fade_frames; frame++) {
            trail = ((uint32_t)trail * CLOUD_IDLE_BOUNCE_TRAIL_DECAY) >> 8;
        }

        const uint16_t weight = dot_weight(i, position_q8);
        const uint16_t added  = ((uint32_t)gain * weight) >> 9;

        trail       = trail > UINT16_MAX - added ? UINT16_MAX : trail + added;
        trail_q8[i] = trail;

        // Add the dot's own glow on top, in Q8, rounding once. Adding keeps the
        // combined glow centered on the dot; taking the brighter of the two
        // shifted it whenever the trail outshone the dot's edge.
        const uint32_t total = (uint32_t)trail + (((uint32_t)level * weight) >> 1) + 128U;

        energy[i] = total >= ((uint32_t)UINT8_MAX << 8) ? UINT8_MAX : total >> 8;
    }

    render(rgblight_get_val());
}

static void update_idle_bounce(void) {
    const uint32_t elapsed = timer_elapsed32(last_frame);

    if (elapsed < CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        return;
    }

    const uint32_t frames = elapsed / CLOUD_REACTIVE_FRAME_INTERVAL_MS;

    last_frame += frames * CLOUD_REACTIVE_FRAME_INTERVAL_MS;
    step_idle_bounce(frames);
}

// Wide keys light two center lanes, so aim the dot between them.
static uint16_t dash_visual_target_q8(void) {
    return (uint16_t)(dash_target_q4 + (dash_wide ? 8U : 0U)) << 4;
}

static uint16_t dash_position_q8(uint32_t elapsed) {
    if (elapsed >= dash_duration) {
        return dash_visual_target_q8();
    }

    const int16_t travel = (int16_t)dash_visual_target_q8() - (int16_t)dash_from_q8;

    return dash_from_q8 + (int16_t)((int32_t)travel * (int32_t)elapsed / (int32_t)dash_duration);
}

static void start_dash(uint8_t position_q4, bool wide) {
    // Leave from wherever the dot is now. A press mid-dash lands the pending
    // key immediately and redirects the dot from its current spot.
    if (idle_bouncing) {
        dash_from_q8 = bounce_position_q8(timer_elapsed32(idle_bounce_timer) + idle_bounce_phase);

        // The bounce leaves its dot in the energy buffer between frames. Drop
        // it so only the trail stays behind; the dash carries the dot away.
        for (uint8_t i = 0; i < RGBLIGHT_LED_COUNT; i++) {
            energy[i] = (trail_q8[i] + 128U) >> 8;
        }
    } else if (dashing) {
        dash_from_q8 = dash_position_q8(timer_elapsed32(dash_timer));
        finish_dash();
    } else {
        dash_from_q8 = last_target_q8;
    }

    dash_target_q4 = position_q4;
    dash_wide      = wide;
    idle_bouncing  = false;

    // Constant speed: crossing the whole bar takes CLOUD_REACTIVE_DASH_FULL_SPAN_MS.
    const uint16_t to       = dash_visual_target_q8();
    const uint16_t distance = to > dash_from_q8 ? to - dash_from_q8 : dash_from_q8 - to;

    dash_duration = (uint32_t)distance * CLOUD_REACTIVE_DASH_FULL_SPAN_MS / DOT_SPAN_Q8;
    dash_last_q8  = dash_from_q8;
    dash_timer    = timer_read32();
    dashing       = true;
}

static void update_dash(void) {
    const uint32_t frame_elapsed = timer_elapsed32(last_frame);

    if (frame_elapsed < CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        return;
    }

    // Earlier hits keep fading at the reactive rate while the dot travels.
    decay(frame_elapsed, CLOUD_REACTIVE_FADE_STEP);
    apply_hold_levels();

    const uint32_t elapsed  = timer_elapsed32(dash_timer);
    const uint16_t position = dash_position_q8(elapsed);

    smear_trail(dash_last_q8, position, trail_scaled(CLOUD_IDLE_BOUNCE_LEVEL));
    dash_last_q8 = position;

    if (elapsed >= dash_duration) {
        // The reactive path draws the landed hit on the next task pass.
        finish_dash();
        return;
    }

    draw_dot(position, CLOUD_IDLE_BOUNCE_LEVEL);
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
        decay(elapsed, CLOUD_REACTIVE_FADE_STEP);
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
    if (!record->event.pressed || !rgblight_is_enabled()) {
        return;
    }

    rgblight_mode(CLOUD_REACTIVE_MODE);
    record_keypress(record, false);
}

void cloud_reactive_rgb_process_record(uint16_t keycode, const keyrecord_t *record) {
    if (record->event.pressed && (keycode == RGB_SPD || keycode == RGB_SPI)) {
        speed_feedback_pending = true;
    }

    const bool wide = is_spacebar_keycode(keycode);

    update_wide_key_state(wide, record);

    if (record->event.pressed && is_active() && !speed_feedback_pending && !speed_feedback_active) {
        record_keypress(record, wide);
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

    if (idle_bouncing) {
        update_idle_bounce();
        return;
    }

    if (dashing) {
        update_dash();
        return;
    }

    if (!dirty && !animating) {
        return;
    }

    const uint32_t elapsed = timer_elapsed32(last_frame);
    if (!dirty && elapsed < CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        return;
    }

    uint8_t       peak;
    const uint8_t brightest = brightest_led(&peak);

    if (elapsed >= CLOUD_REACTIVE_FRAME_INTERVAL_MS) {
        decay(elapsed, CLOUD_REACTIVE_FADE_STEP);
    }

    const bool holding = apply_hold_levels();
    dirty              = false;

    // Decay is uniform, so the brightest pixel stays brightest. Once it fades to
    // the handoff level (or out entirely) with nothing held, it relights and
    // becomes the idle bounce; whatever else is still lit becomes its trail.
    if (!holding && peak != 0 && energy[brightest] <= handoff_level) {
        start_idle_bounce((uint16_t)brightest << 8, energy[brightest]);
        step_idle_bounce(0);
        return;
    }

    render(rgblight_get_val());
}
