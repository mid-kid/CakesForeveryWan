#pragma once

#include <stdint.h>

enum keys {
    key_a = 0b000000000001,
    key_b = 0b000000000010,
    key_select = 0b000000000100,
    key_start = 0b000000001000,
    key_right = 0b000000010000,
    key_left = 0b000000100000,
    key_up = 0b000001000000,
    key_down = 0b000010000000,
    key_r = 0b000100000000,
    key_l = 0b001000000000,
    key_x = 0b010000000000,
    key_y = 0b100000000000,
    key_released = 0b0100000000000000,
    key_pressed = 0b1000000000000000
};

extern volatile uint16_t *const hid_regs;

uint16_t wait_key();
