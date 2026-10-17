// Copyright 2026 Amadeus
// SPDX-License-Identifier: GPL-2.0-or-later

#include "amadeus.h"

static void bootloader_each_tap(tap_dance_state_t *state, void *user_data) {
    (void)user_data;

    if (state->count >= 5) {
        state->finished = true;
        reset_keyboard();
    }
}

tap_dance_action_t tap_dance_actions[AMADEUS_TAP_DANCE_COUNT] = {
    [TD_FLASH] = ACTION_TAP_DANCE_FN_ADVANCED(bootloader_each_tap, NULL, NULL),
};

bool caps_word_press_user(uint16_t keycode) {
    switch (keycode) {
        case KC_A ... KC_Z:
            add_weak_mods(MOD_BIT(KC_LSFT));
            return true;
        case KC_MINS:
        case KC_1 ... KC_0:
        case KC_BSPC:
        case KC_DEL:
        case KC_UNDS:
            return true;
        default:
            return false;
    }
}
