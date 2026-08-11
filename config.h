// Copyright 2026 Jim MacKenzie (@protojazz)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

// I2C1 on GP14 (SDA) and GP15 (SCL) for OLED
#define I2C_DRIVER I2CD1
#define I2C1_SDA_PIN GP14
#define I2C1_SCL_PIN GP15
#define OLED_DISPLAY_128X64

#define SPLIT_TRANSACTION_IDS_USER USER_SYNC_A

// Handedness lives in each half's emulated EEPROM. Defining this is only half
// the job -- the value has to be written to both controllers, and a plain
// `qmk flash` does NOT write it. See readme.md for the flashing procedure.
#define EE_HANDS

// The RP2040 has no VBUS sense pin, so ChibiOS force-enables SPLIT_USB_DETECT:
// each half waits up to SPLIT_USB_TIMEOUT ms for USB enumeration before
// deciding it is the slave. If the host is slow, both halves give up and come
// up as slave, and the board is dead until it's unplugged. The watchdog reboots
// a slave that never hears from a master, so it retries instead of hanging.
#define SPLIT_WATCHDOG_ENABLE

// Encoder push-button. Not part of the key matrix, same pin on both halves.
#define USER_ENCODER_BTN_PIN GP20
