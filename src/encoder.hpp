#ifndef ENCODER_HPP
#define ENCODER_HPP

#include <array>
#include <cstdint>

#include "hardware/gpio.h"

class Encoder {
public:
    explicit Encoder(uint enc_a_pin, uint enc_b_pin, uint button_pin,
                     uint32_t debounce_ms = 150, int32_t max_partial = 12);

    void init();
    void poll_encoder();
    void poll_button();

    uint32_t partial() const;
    bool button_pressed() const;

private:
    uint enc_a_pin_;
    uint enc_b_pin_;
    uint button_pin_;

    uint32_t debounce_ms_;
    int32_t max_partial_;

    uint32_t partial_ = 1;
    uint8_t last_enc_state_ = 0;
    uint8_t last_button_state_ = 1;
    int8_t step_accumulator_ = 0;
    uint32_t last_button_edge_ms_ = 0;

    static constexpr std::array<int8_t, 16> kTransitions = {
        0, -1, 1, 0,
        1, 0, 0, -1,
        -1, 0, 0, 1,
        0, 1, -1, 0,
    };
};

#endif
