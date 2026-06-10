#include "dac.hpp"

#include "i2s.pio.h"

#include <cassert>
#include <cmath>
#include <cstdint>
#include <hardware/clocks.h>
#include <hardware/gpio.h>
#include <hardware/pio.h>
#include <pico/multicore.h>

DAC::DAC(uint32_t data_out_pin, uint32_t bclk_pin, uint32_t lrclk_pin, PIO pio)
	: data_out_pin_(data_out_pin),
	  bclk_pin_(bclk_pin),
	  lrclk_pin_(lrclk_pin),
	  pio_(pio) {}

void DAC::init() {
	assert(lrclk_pin_ == bclk_pin_ + 1);

	gpio_init_mask((1u << data_out_pin_) | (1u << bclk_pin_) | (1u << lrclk_pin_));
	gpio_set_dir_out_masked((1u << data_out_pin_) | (1u << bclk_pin_) | (1u << lrclk_pin_));

	sm_ = pio_claim_unused_sm(pio_, true);
	offset_ = pio_add_program(pio_, &i2s_out_master_program);
	i2s_out_master_program_init(pio_, sm_, offset_, bit_depth_, data_out_pin_, bclk_pin_);

	// Program uses two PIO cycles per BCLK edge and 32-bit slots per channel.
	// target_pio_hz = sample_rate * 64 bits/frame * 2 cycles/bit.
	const float target_pio_hz = sample_rate_hz_ * 64.0f * 2.0f;
	const float clk_div = static_cast<float>(clock_get_hz(clk_sys)) / target_pio_hz;
	pio_sm_set_clkdiv(pio_, sm_, clk_div);
	pio_sm_set_enabled(pio_, sm_, true);
}

uint32_t DAC::get_next_audio_sample() {
	const float phase_increment = (2.0f * static_cast<float>(M_PI) * get_frequency_hz()) / sample_rate_hz_;
	phase_ += phase_increment;
	if (phase_ >= (2.0f * static_cast<float>(M_PI))) {
		phase_ -= (2.0f * static_cast<float>(M_PI));
	}

	const int32_t sample_max = static_cast<int32_t>((1u << (bit_depth_ - 1)) - 1u);
	const int32_t sample = static_cast<int32_t>(std::sin(phase_) * static_cast<float>(sample_max) * volume_);

	// I2S frame uses 32-bit slots per channel; left-align configured sample width.
	const uint32_t sample_mask = (bit_depth_ >= 32u)
		? 0xFFFFFFFFu
		: ((1u << bit_depth_) - 1u);
	uint32_t packed = static_cast<uint32_t>(sample) & sample_mask;
	if (bit_depth_ < 32u) {
		packed <<= (32u - bit_depth_);
	}
	return packed;
}

void DAC::play_audio() {
	while (true) {
		uint32_t new_frequency = 0;
		if (multicore_fifo_pop_timeout_us(0, &new_frequency)) {
			frequency_ = new_frequency;
		}

		if (frequency_ == 0u || pio_sm_get_tx_fifo_level(pio_, sm_) >= 6) {
			continue;
		} // Loops if the 8 sample TX FIFO can't store another stereo sample
		uint32_t mono_audio_sample = get_next_audio_sample();
		pio_sm_put(pio_, sm_, mono_audio_sample); // Left
		pio_sm_put(pio_, sm_, mono_audio_sample); // Right
	}
}