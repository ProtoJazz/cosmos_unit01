#include QMK_KEYBOARD_H
#include "transactions.h"
#include "images.h"

#define NUM_DISPLAY_MODES 3   // 0 = info, 1 = NERV, 2 = hotdog

// Shared state, kept in sync across both halves
typedef struct _kb_runtime_state_t {
    uint8_t display_mode;
} kb_runtime_state_t;

static kb_runtime_state_t kb_state = {0};

// Slave-side flag: did the right encoder button get pressed this cycle?
static volatile bool slave_btn_event = false;

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
        // Right half - main grid (rows 6-9)
        KC_6,    KC_7,    KC_8,    KC_9,    KC_0,
        KC_Y,    KC_U,    KC_I,    KC_O,    KC_P,
        KC_H,    KC_J,    KC_K,    KC_L,    KC_SCLN,
        KC_N,    KC_M,    KC_COMM, KC_DOT,  KC_SLSH,
        // Right thumb row 10
        KC_BSPC, KC_ENT,           KC_RCTL,
        // Right thumb row 11
        KC_DEL,                    KC_RGUI
    )
};

// ---------------------------------------------------------------------------
// Split transaction: master pulls the slave's button state across the link
// ---------------------------------------------------------------------------

// Custom RPC: master sends current display_mode, slave returns its button flag
void user_sync_handler(uint8_t in_buflen, const void* in_data,
                       uint8_t out_buflen, void* out_data) {
    // Master pushes the authoritative display mode to us (slave)
    const kb_runtime_state_t* m2s = (const kb_runtime_state_t*)in_data;
    kb_state.display_mode = m2s->display_mode;

    // Slave reports whether its encoder button was pressed since last sync
    bool* btn = (bool*)out_data;
    *btn = slave_btn_event;
    slave_btn_event = false;  // consume the event
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(USER_SYNC_A, user_sync_handler);
    gpio_set_pin_input_high(GP20);  // encoder press pin, both halves
}

// ---------------------------------------------------------------------------
// Master: read own (left) encoder press + poll slave (right) encoder press
// ---------------------------------------------------------------------------
void matrix_scan_user(void) {
    // Left encoder press = play/pause (master side, direct GPIO read)
    static bool left_btn_prev = false;
    bool left_btn = !gpio_read_pin(GP20);
    if (left_btn && !left_btn_prev) {
        tap_code(KC_MPLY);
    }
    left_btn_prev = left_btn;
}

void housekeeping_task_user(void) {
    if (is_keyboard_master()) {
        // Sync once every 50ms
        static uint32_t last_sync = 0;
        if (timer_elapsed32(last_sync) > 50) {
            last_sync = timer_read32();

            bool slave_pressed = false;
            if (transaction_rpc_exec(USER_SYNC_A,
                    sizeof(kb_state), &kb_state,
                    sizeof(slave_pressed), &slave_pressed)) {
                if (slave_pressed) {
                    // Right encoder pressed -> cycle display mode
                    kb_state.display_mode =
                        (kb_state.display_mode + 1) % NUM_DISPLAY_MODES;
                }
            }
        }
    } else {
        // Slave: detect our own (right) encoder button press edge
        static bool prev = false;
        bool now = !gpio_read_pin(GP20);
        if (now && !prev) {
            slave_btn_event = true;  // latch; master reads it on next sync
        }
        prev = now;
    }
}

// ---------------------------------------------------------------------------
// Encoders
// ---------------------------------------------------------------------------
#if defined(ENCODER_MAP_ENABLE)
const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][NUM_DIRECTIONS] = {
    [0] = {
        ENCODER_CCW_CW(KC_VOLU, KC_VOLD),  // left encoder: volume
        ENCODER_CCW_CW(KC_MNXT, KC_MPRV)   // right encoder: track skip
    },
};
#endif

// ---------------------------------------------------------------------------
// OLED
// ---------------------------------------------------------------------------
#ifdef OLED_ENABLE

oled_rotation_t oled_init_user(oled_rotation_t rotation) {
    if (is_keyboard_left()) {
        return OLED_ROTATION_270;
    } else {
        return OLED_ROTATION_90;
    }
}

static void render_info(void) {
    if (is_keyboard_master()) {
        oled_write_ln_P(PSTR("   Cosmos"), false);
        oled_write_ln_P(PSTR("   Unit 01"), false);
        oled_write_ln_P(PSTR(""), false);
        oled_write_ln_P(PSTR("   Layer:"), false);
        switch (get_highest_layer(layer_state)) {
            case 0:  oled_write_ln_P(PSTR("   Base"),  false); break;
            case 1:  oled_write_ln_P(PSTR("   Sym"),   false); break;
            default: oled_write_ln_P(PSTR("   Other"), false);
        }
        oled_write_ln_P(PSTR(""), false);
        led_t led_state = host_keyboard_led_state();
        oled_write_P(led_state.caps_lock ? PSTR("CAPS") : PSTR("    "), false);
    } else {
        oled_write_ln_P(PSTR("   Cosmos"), false);
        oled_write_ln_P(PSTR("   Right"), false);
    }
}

bool oled_task_user(void) {
    // Clear the screen once whenever the mode changes, so leftover pixels
    // from a full-screen image don't linger behind the partial info screen.
    static uint8_t prev_mode = 0xFF;
    if (kb_state.display_mode != prev_mode) {
        oled_clear();
        prev_mode = kb_state.display_mode;
    }

    switch (kb_state.display_mode) {
        case 1:
            oled_write_raw_P(nerv_logo, sizeof(nerv_logo));
            break;
        case 2:
            oled_write_raw_P(hotdog_img, sizeof(hotdog_img));
            break;
        case 0:
        default:
            render_info();
            break;
    }
    return false;
}
#endif