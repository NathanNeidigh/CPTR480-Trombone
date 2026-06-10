// Rotary encoder and button handler for the Pico SDK (polling).

#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include <pico/multicore.h>
#include <pico/stdlib.h>

#include "HC-SR04.hpp"
#include "encoder.hpp"
#include "dac.hpp"

constexpr uint ENC_A_PIN = 18;
constexpr uint ENC_B_PIN = 17;
constexpr uint ENC_BUT_PIN = 5;
constexpr uint HC_SR04_TRIGGER_PIN = 19;
constexpr uint HC_SR04_ECHO_PIN = 20;
constexpr uint DAC_DOUT_PIN = 9;
constexpr uint DAC_BCLK_PIN = 6;
constexpr uint DAC_LRCLK_PIN = 7;

void core1_entry() {
	DAC dac(DAC_DOUT_PIN, DAC_BCLK_PIN, DAC_LRCLK_PIN);
	dac.init();
	dac.play_audio();
}

int main() {
	stdio_init_all();
	multicore_launch_core1(core1_entry);
	HCSR04 sensor(HC_SR04_TRIGGER_PIN, HC_SR04_ECHO_PIN);
	Encoder encoder(ENC_A_PIN, ENC_B_PIN, ENC_BUT_PIN);
	encoder.init();

	uint32_t distance = 0;
	while (true) {
		encoder.poll_encoder();
		encoder.poll_button();
		if (!encoder.button_pressed()) { 
			multicore_fifo_push_timeout_us(0, 0);
			continue; 
		}
		if (!sensor.distance_mm(&distance)) { 
			continue; 
		}
		double wavelength = 4.2831e-3 * distance + 5.5438; // Linear interpolation of distance to wavelength
		double frequency = 343 / wavelength;
		//std::printf("Partial: %u Frequency: %f Slide Distance: %u\n", encoder.partial(), frequency * encoder.partial(), distance);
		multicore_fifo_push_blocking(static_cast<uint32_t>(frequency * encoder.partial() * (1u << FREQ_FRAC_BITS)));
	}

	return 0;
}
