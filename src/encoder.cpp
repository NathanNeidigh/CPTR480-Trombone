#include "encoder.hpp"

#include <cstdio>

#include "pico/time.h"

Encoder::Encoder(uint enc_a_pin, uint enc_b_pin, uint button_pin,
                 uint32_t debounce_ms, int32_t max_partial)
    : enc_a_pin_(enc_a_pin),
      enc_b_pin_(enc_b_pin),
      button_pin_(button_pin),
      debounce_ms_(debounce_ms),
    max_partial_(max_partial > 0 ? max_partial : 1) {}

void Encoder::init() {
    gpio_init(enc_a_pin_);
    gpio_init(enc_b_pin_);
    gpio_init(button_pin_);

    gpio_set_dir(enc_a_pin_, false);
    gpio_set_dir(enc_b_pin_, false);
    gpio_set_dir(button_pin_, false);

    gpio_pull_up(enc_a_pin_);
    gpio_pull_up(enc_b_pin_);
    gpio_pull_up(button_pin_);

    last_enc_state_ = static_cast<uint8_t>((gpio_get(enc_a_pin_) << 1) | gpio_get(enc_b_pin_));
    last_button_state_ = static_cast<uint8_t>(gpio_get(button_pin_));
}

void Encoder::poll_encoder() {
    const uint8_t current_state = static_cast<uint8_t>((gpio_get(enc_a_pin_) << 1) | gpio_get(enc_b_pin_));
    if (current_state == last_enc_state_) {
        return;
    }

    const uint8_t transition = static_cast<uint8_t>((last_enc_state_ << 2) | current_state);
    const int8_t direction = kTransitions[transition];

    last_enc_state_ = current_state;

    if (direction == 0) {
        return;
    }

    // Reset accumulator on direction reversal so a mid-detent direction change
    // doesn't require an extra click to register.
    if ((direction > 0 && step_accumulator_ < 0) || (direction < 0 && step_accumulator_ > 0)) {
        step_accumulator_ = 0;
    }

    step_accumulator_ += direction;

    // Two valid transitions per detent on this encoder.
    if (step_accumulator_ >= 2) {
        partial_ = (partial_ < max_partial_) ? partial_ + 1 : partial_;
        step_accumulator_ = 0;
    } else if (step_accumulator_ <= -2) {
        partial_ = (partial_ > 1) ? partial_ - 1 : partial_;
        step_accumulator_ = 0;
    }
}

void Encoder::poll_button() {
    const uint8_t current_button_state = static_cast<uint8_t>(gpio_get(button_pin_));
    if (current_button_state == last_button_state_) {
        return;
    }

    const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
    if ((now_ms - last_button_edge_ms_) > debounce_ms_) {
        last_button_state_ = current_button_state;
        last_button_edge_ms_ = now_ms;
    }
}

uint32_t Encoder::partial() const {
    return partial_;
}

bool Encoder::button_pressed() const {
    return last_button_state_ == 0;
}
