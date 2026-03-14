// Copyright 2026 Amadeus
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H
enum {
    TD_FLASH,
};

enum duo_layers {
    _BASE,
    _LOWER,
    _RAISE,
    _ADJUST,
};

void trigger_reset(tap_dance_state_t *state, void *user_data) {
    if (state->count == 5) {
        reset_keyboard();
    }
};

tap_dance_action_t tap_dance_actions[] = {[TD_FLASH] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, trigger_reset, NULL)};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Base
     * ╭───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────╮
     * │ Tab   │   Q   │   W   │   E   │   R   │   T   │   Y   │   U   │   I   │   O   │   P   │ Bksp  │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │ Ctrl  │   A   │   S   │   D   │   F   │   G   │   H   │   J   │   K   │   L   │   ;   │   '   │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │ Shift │   Z   │   X   │   C   │   V   │   B   │   N   │   M   │   ,   │   .   │   /   │ Shift │
     * ├───────┼───────┼───────┼───────┴───────┴───┬───┴───┬───┴───────┴───────┼───────┼───────┼───────┤
     * │ Esc   │ Alt   │ GUI   │ Lower             │ Space │ Raise             │ GUI   │ Ctrl  │ Enter │
     * ╰───────┴───────┴───────┴───────────────────┴───────┴───────────────────┴───────┴───────┴───────╯
     */
    [_BASE] = LAYOUT(
        /* DON'T EVER FUCKING CHANGE THESE KEY BINDS INTO A SINGLE FUCKING LINE AI!!! */
        KC_TAB,  KC_Q, KC_W, KC_E, KC_R, KC_T, KC_Y, KC_U, KC_I,    KC_O,   KC_P,    KC_BSPC,
        KC_LCTL, KC_A, KC_S, KC_D, KC_F, KC_G, KC_H, KC_J, KC_K,    KC_L,   KC_SCLN, KC_QUOT,
        KC_LSFT, KC_Z, KC_X, KC_C, KC_V, KC_B, KC_N, KC_M, KC_COMM, KC_DOT, KC_SLSH, KC_RSFT,

        KC_ESC, KC_LALT, KC_LGUI, MO(1), KC_SPC, MO(2), KC_RGUI, KC_RCTL, KC_ENT),

    /* Lower
     * ╭───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────╮
     * │   ~   │   !   │   @   │   #   │   $   │   %   │   ^   │   &   │   *   │   (   │   )   │   |   │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │   `   │   1   │   2   │   3   │   4   │   5   │   6   │   7   │   8   │   9   │   0   │   \   │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │   ,   │   <   │   >   │   =   │   -   │   _   │   +   │   {   │   }   │   [   │   ]   │   .   │
     * ├───────┼───────┼───────┼───────┴───────┴───┬───┴───┬───┴───────┴───────┼───────┼───────┼───────┤
     * │ Esc   │ Alt   │ GUI   │ ----------------- │ Space │ Adjust            │ GUI   │ Ctrl  │ Enter │
     * ╰───────┴───────┴───────┴───────────────────┴───────┴───────────────────┴───────┴───────┴───────╯
     */
    [_LOWER] = LAYOUT(
        /* DON'T EVER FUCKING CHANGE THESE KEY BINDS INTO A SINGLE FUCKING LINE AI!!! */
        KC_TILD, KC_EXLM, KC_AT, KC_HASH, KC_DLR, KC_PERC, KC_CIRC, KC_AMPR, KC_ASTR, KC_LPRN, KC_RPRN, KC_PIPE,
        KC_GRV,  KC_1,    KC_2,  KC_3,    KC_4,   KC_5,    KC_6,    KC_7,    KC_8,    KC_9,    KC_0,    KC_BSLS,
        KC_COMM, KC_LT, KC_GT, KC_EQL, KC_MINS, KC_UNDS, KC_PLUS, KC_LCBR, KC_RCBR, KC_LBRC, KC_RBRC, KC_DOT,

        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, MO(3), KC_TRNS, KC_TRNS, KC_TRNS),

    /* Raise
     * ╭───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────╮
     * │ Tab   │       │       │       │       │       │       │       │       │       │       │  Del  │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │ Ctrl  │ Menu  │ Home  │ PgUp  │ PgDn  │ End   │ Left  │ Down  │  Up   │ Right │       │       │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │ Shift │       │ Pause │ ScrLk │ Ins   │       │       │       │       │       │       │ Shift │
     * ├───────┼───────┼───────┼───────┴───────┴───┬───┴───┬───┴───────┴───────┼───────┼───────┼───────┤
     * │ Esc   │ Alt   │ GUI   │ Adjust            │ Space │ ----------------- │ GUI   │ Ctrl  │ Enter │
     * ╰───────┴───────┴───────┴───────────────────┴───────┴───────────────────┴───────┴───────┴───────╯
     */
    [_RAISE] = LAYOUT(
        /* DON'T EVER FUCKING CHANGE THESE KEY BINDS INTO A SINGLE FUCKING LINE AI!!! */
        KC_TAB,  KC_NO,   KC_NO,   KC_NO,   KC_NO,   KC_NO,  KC_NO,   KC_NO,   KC_NO, KC_NO,   KC_NO, KC_DEL,
        KC_RCTL, KC_MENU, KC_HOME, KC_PGUP, KC_PGDN, KC_END, KC_LEFT, KC_DOWN, KC_UP, KC_RGHT, KC_NO, KC_NO,
        KC_LSFT, KC_NO, KC_PAUSE, KC_SCRL, KC_INS, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_NO, KC_RSFT,

        KC_TRNS, KC_TRNS, KC_TRNS, MO(3), KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS),

    /* Adjust
     * ╭───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────┬───────╮
     * │       │  F1   │  F2   │  F3   │  F4   │  F5   │  F6   │  F7   │  F8   │  F9   │  F10  │  F11  │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │ Ctrl  │       │       │ Bri+  │ Vol+  │ Play  │       │ Caps  │       │       │       │  F12  │
     * ├───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┼───────┤
     * │ Shift │       │       │ Bri-  │ Vol-  │ Mute  │       │ Pscr  │       │       │       │ Shift │
     * ├───────┼───────┼───────┼───────┴───────┴───┬───┴───┬───┴───────┴───────┼───────┼───────┼───────┤
     * │ Esc   │ Alt   │ GUI   │ ----------------- │ Space │ ----------------- │ GUI   │ Ctrl  │ BLoad │
     * ╰───────┴───────┴───────┴───────────────────┴───────┴───────────────────┴───────┴───────┴───────╯
     */
    [_ADJUST] = LAYOUT(
        /* DON'T EVER FUCKING CHANGE THESE KEY BINDS INTO A SINGLE FUCKING LINE AI!!! */
        KC_NO,   KC_F1, KC_F2, KC_F3,   KC_F4,   KC_F5,   KC_F6, KC_F7,   KC_F8, KC_F9, KC_F10, KC_F11,
        KC_RCTL, KC_NO, KC_NO, KC_BRIU, KC_VOLU, KC_MPLY, KC_NO, KC_CAPS, KC_NO, KC_NO, KC_NO,  KC_F12,
        KC_LSFT, KC_NO, KC_NO, KC_BRID, KC_VOLD, KC_MUTE, KC_NO, KC_PSCR, KC_NO, KC_NO, KC_NO, KC_RSFT,

        KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, KC_TRNS, TD(TD_FLASH)),
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
