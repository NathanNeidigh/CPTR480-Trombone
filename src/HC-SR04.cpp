#include "HC-SR04.hpp"

#include <algorithm>
#include <cassert>
#include <cstdlib>
#include <cstdint>
#include <numeric>
#include <vector>
#include "pico/time.h"

HCSR04::HCSR04(uint trigger_pin, uint echo_pin, uint32_t echo_timeout_us)
    : trigger_pin_(trigger_pin), echo_pin_(echo_pin), echo_timeout_us_(echo_timeout_us) {
    gpio_init(trigger_pin_);
    gpio_set_dir(trigger_pin_, GPIO_OUT);
    gpio_put(trigger_pin_, 0);

    gpio_init(echo_pin_);
    gpio_set_dir(echo_pin_, GPIO_IN);
}

bool HCSR04::distance_mm_trimmed_mean(uint32_t* mm, uint32_t num_samples, float trim_percent) {
    if (mm == nullptr || num_samples == 0) {
        return false;
    }

    assert(0.0f <= trim_percent && trim_percent <= 0.5f);
    if (trim_percent < 0.0f || trim_percent > 0.5f) {
        return false;
    }

    std::vector<uint32_t> distance_samples;
    distance_samples.reserve(num_samples);

    // HC-SR04 needs spacing between trigger pulses to avoid stale/overlapping echoes.
    const uint32_t max_attempts = num_samples * 3u;
    uint32_t attempts = 0;
    while (distance_samples.size() < num_samples && attempts < max_attempts) {
        uint32_t sample_mm = 0;
        if (distance_mm(&sample_mm)) {
            distance_samples.push_back(sample_mm);
        }
        ++attempts;
        sleep_ms(60);
    }

    if (distance_samples.size() < num_samples) {
        return false;
    }

    std::sort(distance_samples.begin(), distance_samples.end());

    const uint32_t lower_index = static_cast<uint32_t>(trim_percent * num_samples);
    const uint32_t upper_index = num_samples - lower_index;

    if (lower_index >= upper_index) {
        return false;
    }

    const uint64_t sum = std::accumulate(
        distance_samples.begin() + lower_index,
        distance_samples.begin() + upper_index,
        static_cast<uint64_t>(0)
    );
    *mm = sum / (upper_index - lower_index);
    return true;
}

bool HCSR04::distance_mm(uint32_t* mm) {
    if (mm == nullptr) {
        return false;
    }

    uint32_t pulse_time_us = 0;
    if (!send_pulse_and_wait(&pulse_time_us)) {
        return false;
    }

    // pulse_time // 2 // 2.91 -> pulse_time * 100 // 582
    *mm = (pulse_time_us * 100u) / 582u;
    return *mm <= 670;
}

bool HCSR04::send_pulse_and_wait(uint32_t* pulse_time_us) {
    if (pulse_time_us == nullptr) {
        return false;
    }

    // Stabilize sensor and send 10us trigger pulse.
    gpio_put(trigger_pin_, 0);
    sleep_us(5);
    gpio_put(trigger_pin_, 1);
    sleep_us(10);
    gpio_put(trigger_pin_, 0);

    const uint32_t start_wait = time_us_32();
    while (gpio_get(echo_pin_) == 0) {
        if ((time_us_32() - start_wait) > echo_timeout_us_) {
            return false;
        }
    }

    const uint32_t pulse_start = time_us_32();
    while (gpio_get(echo_pin_) == 1) {
        if ((time_us_32() - pulse_start) > echo_timeout_us_) {
            return false;
        }
    }

    *pulse_time_us = time_us_32() - pulse_start;
    return true;
}

