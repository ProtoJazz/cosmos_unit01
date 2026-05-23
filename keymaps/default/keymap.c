#include QMK_KEYBOARD_H

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [0] = LAYOUT(
        // Left half - main grid (rows 0-3)
        KC_1,    KC_2,    KC_3,    KC_4,    KC_5,
        KC_Q,    KC_W,    KC_E,    KC_R,    KC_T,
        KC_A,    KC_S,    KC_D,    KC_F,    KC_G,
        KC_Z,    KC_X,    KC_C,    KC_V,    KC_B,
        // Left thumb row 4 (T1, T2, T3)
        KC_LCTL,          KC_SPC,  KC_LSFT,
        // Left thumb row 5 (T4, T5)
        KC_LGUI,          KC_TAB,

        // Right half - main grid (rows 6-9), reversed col order to match physical layout
        KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,
        KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,
        KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,
        // Right thumb row 10 (T3, T2, T1 in physical order due to reversal)
        KC_BSPC, KC_ENT,           KC_RCTL,
        // Right thumb row 11 (T5, T4)
        KC_DEL,                    KC_RGUI
    )
};

void keyboard_post_init_user(void) {
    gpio_set_pin_input_high(GP20);  // Enable internal pullup
}

void matrix_scan_user(void) {
    static bool encoder_btn_pressed = false;
    bool current = !gpio_read_pin(GP20);  // Active low (pressed = LOW)
    
    if (current && !encoder_btn_pressed) {
        tap_code(KC_MPLY);
        encoder_btn_pressed = true;
    } else if (!current && encoder_btn_pressed) {
        encoder_btn_pressed = false;
    }
}

#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = { ENCODER_CCW_CW(KC_VOLU, KC_VOLD) },
};
#endif

#ifdef OLED_ENABLE
oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    if (is_keyboard_left()) {
        return OLED_ROTATION_270;
    } else {
        return OLED_ROTATION_90;
    }
}
bool oled_task_user(void) {
    if (is_keyboard_master()) {
        oled_write_ln_P(PSTR("   Cosmos"), false);
        oled_write_ln_P(PSTR("   Unit 01"), false);
        oled_write_ln_P(PSTR(""), false);
        oled_write_ln_P(PSTR("   Layer:"), false);
        switch (get_highest_layer(layer_state)) {
            case 0:
                oled_write_ln_P(PSTR("   Base"), false);
                break;
            default:
                oled_write_ln_P(PSTR("   Other"), false);
        }
        oled_write_ln_P(PSTR(""), false);
        led_t led_state = host_keyboard_led_state();
        oled_write_P(led_state.caps_lock ? PSTR("CAPS") : PSTR("    "), false);
    } else {
        oled_write_ln_P(PSTR("   Cosmos"), false);
        oled_write_ln_P(PSTR("   Right"), false);
    }
    return false;
}
#endif