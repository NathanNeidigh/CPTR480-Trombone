#ifndef HC_SR04_HPP
#define HC_SR04_HPP

#include <cstdint>

#include "hardware/gpio.h"

class HCSR04 {
public:
    // echo_timeout_us defaults near the HC-SR04 max range (~4m round trip ~23.5 ms).
    explicit HCSR04(uint trigger_pin, uint echo_pin, uint32_t echo_timeout_us = 25000u);

    // Returns false on timeout (out of range), true on success.
    bool distance_mm(uint32_t* mm);

    // Returns false on timeout (out of range), true on success.
    // Trims the last x distance samples
    bool distance_mm_trimmed_mean(uint32_t* mm, uint32_t num_samples, float trim_percent = 0.1);
private:
    bool send_pulse_and_wait(uint32_t* pulse_time_us);

    uint trigger_pin_;
    uint echo_pin_;
    uint32_t echo_timeout_us_;
};

#endif
