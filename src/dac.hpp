#ifndef DAC_HPP
#define DAC_HPP

#include <cstdint>

#include <hardware/pio.h>

#define FREQ_FRAC_BITS (16)

class DAC {
public:
	explicit DAC(uint32_t data_out_pin, uint32_t bclk_pin, uint32_t lrclk_pin, PIO pio = pio0);
	
	void init();
	void play_audio();

private:
	uint32_t data_out_pin_;
	uint32_t bclk_pin_;
	uint32_t lrclk_pin_;
	PIO pio_;
	uint sm_;
	uint offset_;
	static const uint32_t bit_depth_ = 24;
	static constexpr float sample_rate_hz_ = 48000.0f;

	uint32_t frequency_ = 0; //fixed-point 16-bit integer bits 16-bits fractional bits
	float phase_ = 0.0f;
	float volume_ = 0.25;

	float get_frequency_hz() const {
		return static_cast<float>(frequency_) / static_cast<float>(1u << FREQ_FRAC_BITS);
	}
	uint32_t get_next_audio_sample();
};

#endif