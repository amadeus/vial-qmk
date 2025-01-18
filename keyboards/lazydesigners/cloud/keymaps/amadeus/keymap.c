/* Copyright 2022 LAZYDESIGNERS
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include QMK_KEYBOARD_H

enum {
    TD_FLASH,
};

void alt_finished(tap_dance_state_t *state, void *user_data) {
    if (state->count == 3) {
        reset_keyboard();
    }
};

tap_dance_action_t tap_dance_actions[] = {[TD_FLASH] = ACTION_TAP_DANCE_FN_ADVANCED(NULL, alt_finished, NULL)};

enum cloud_layers {
  _BASE,
  _LOWER,
  _RAISE,
  _ADJUST
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT_ortho(
        KC_MUTE,
        KC_TAB,   KC_Q,     KC_W,     KC_E,     KC_R,   KC_T,    KC_Y,    KC_U,    KC_I,     KC_O,     KC_P,     KC_BSPC,
        KC_RCTL,  KC_A,     KC_S,     KC_D,     KC_F,   KC_G,    KC_H,    KC_J,    KC_K,     KC_L,     KC_SCLN,  KC_QUOT,
        KC_LSFT,  KC_Z,     KC_X,     KC_C,     KC_V,   KC_B,    KC_N,    KC_M,    KC_COMM,  KC_DOT,   KC_SLSH,  KC_RSFT,
        KC_ESC,   KC_LCTL,  KC_LALT,  KC_LGUI,  MO(1),  KC_SPC,  KC_SPC,  MO(2),   KC_RGUI,  KC_RALT,  KC_RCTL,  KC_ENT
    ),
    [_LOWER] = LAYOUT_ortho(
        RGB_TOG,
        KC_TILD,  KC_EXLM,  KC_AT,    KC_HASH,  KC_DLR,   KC_PERC,  KC_CIRC,  KC_AMPR,  KC_ASTR,  KC_LPRN,  KC_RPRN,  KC_PIPE,
        KC_GRV,   KC_1,     KC_2,     KC_3,     KC_4,     KC_5,     KC_6,     KC_7,     KC_8,     KC_9,     KC_0,     KC_BSLS,
        KC_COMM,  KC_LT,    KC_GT,    KC_EQL,   KC_MINS,  KC_UNDS,  KC_PLUS,  KC_LCBR,  KC_RCBR,  KC_LBRC,  KC_RBRC,  KC_DOT,
        KC_ESC,   KC_LCTL,  KC_LALT,  KC_LGUI,  KC_TRNS,  KC_SPC,   KC_SPC,   MO(3),    KC_RGUI,  KC_RALT,  KC_RCTL,  KC_ENT
    ),
    [_RAISE] = LAYOUT_ortho(
        RGB_MOD,
        KC_TAB,   KC_NO,    KC_NO,     KC_NO,    KC_NO,    KC_NO,   KC_NO,    KC_NO,    KC_NO,    KC_NO,    KC_NO,    KC_DEL,
        KC_RCTL,  KC_MENU,  KC_HOME,   KC_PGUP,  KC_PGDN,  KC_END,  KC_LEFT,  KC_DOWN,  KC_UP,    KC_RGHT,  KC_NO,    KC_NO,
        KC_LSFT,  KC_NO,    KC_PAUSE,  KC_SCRL,  KC_INS,   KC_NO,   KC_NO,    KC_NO,    KC_NO,    KC_NO,    KC_NO,    KC_RSFT,
        KC_ESC,   KC_LCTL,  KC_LALT,   KC_LGUI,  MO(3),    KC_SPC,  KC_SPC,   KC_TRNS,  KC_RGUI,  KC_RALT,  KC_RCTL,  KC_ENT
    ),
    [_ADJUST] = LAYOUT_ortho(
        KC_NO,
        KC_NO,    KC_F1,    KC_F2,    KC_F3,    KC_F4,    KC_F5,    KC_F6,    KC_F7,    KC_F8,     KC_F9,    KC_F10,   KC_F11,
        KC_RCTL,  KC_NO,    KC_NO,    KC_BRIU,  KC_VOLU,  KC_MPLY,  KC_NO,    KC_CAPS,  RGB_MOD,   RGB_HUI,  RGB_SPI,  KC_F12,
        KC_LSFT,  AU_TOGG,  KC_NO,    KC_BRID,  KC_VOLD,  KC_MUTE,  KC_NO,    KC_PSCR,  RGB_TOG,   RGB_HUD,  RGB_SPD,  KC_RSFT,
        KC_ESC,   KC_LCTL,  KC_LALT,  KC_LGUI,  KC_TRNS,  KC_SPC,   KC_SPC,   KC_TRNS,  KC_RGUI,   KC_RALT,  KC_RCTL,  TD(TD_FLASH)
    ),
};

// Custom Caps Word implementation -- basically only shift letters, not
// symbols, given my layout
bool caps_word_press_user(uint16_t keycode) {
    switch (keycode) {
        // Capitalize all letter keys
        case KC_A ... KC_Z:
            add_weak_mods(MOD_BIT(KC_LSFT));
            return true;
        // Keycodes that still continue Caps Word
        case KC_MINS:
        case KC_1 ... KC_0:
        case KC_BSPC:
        case KC_DEL:
        case KC_UNDS:
            return true;
        // Otherwise, stop Caps Word
        default:
            return false;
    }
}

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [_BASE]   = { ENCODER_CCW_CW(KC_VOLD,  KC_VOLU) },
    [_LOWER]  = { ENCODER_CCW_CW(RGB_RMOD, RGB_MOD) },
    [_RAISE]  = { ENCODER_CCW_CW(RGB_VAD,  RGB_VAI) },
    [_ADJUST] = { ENCODER_CCW_CW(RGB_HUD,  RGB_HUI) },
};
#endif
