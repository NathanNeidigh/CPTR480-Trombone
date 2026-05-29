// Rotary encoder and button handler for the Pico SDK (polling).

#include <array>
#include <cstdint>
#include <cstdio>
#include <cstdlib>

#include "pico/stdlib.h"

constexpr uint ENC_A_PIN = 18;
constexpr uint ENC_B_PIN = 17;
constexpr uint ENC_BUT_PIN = 5;

constexpr uint32_t DEBOUNCE_MS = 150;
constexpr uint32_t MAX_PARTIAL = 20;

// Transition table for quadrature decoding.
// previous_state (2 bits) + current_state (2 bits) -> direction
constexpr std::array<int8_t, 16> kTransitions = {
	0, -1, 1, 0,
	1, 0, 0, -1,
	-1, 0, 0, 1,
	0, 1, -1, 0,
};

int32_t g_counter = 0;
uint8_t g_last_enc_state = 0;
uint8_t g_last_but_state = 1;
int8_t g_step_accumulator = 0;

uint32_t g_last_center_ms = 0;

void poll_encoder() {
	const uint8_t current_state = static_cast<uint8_t>((gpio_get(ENC_A_PIN) << 1) | gpio_get(ENC_B_PIN));
	if (current_state == g_last_enc_state) {
		return;
	}

	const uint8_t transition = static_cast<uint8_t>((g_last_enc_state << 2) | current_state);
	const int8_t direction = kTransitions[transition];

	g_last_enc_state = current_state;

	if (direction == 0) {
		return;
	}
	
	// Reset accumulator on direction reversal so a mid-detent direction change
    // doesn't require an extra click to register.
    if ((direction > 0 && g_step_accumulator < 0) || (direction < 0 && g_step_accumulator > 0)) {
        g_step_accumulator = 0;
    }

    g_step_accumulator += direction;

	// Two valid transitions per detent on this encoder.
	if (g_step_accumulator >= 2) {
		g_counter = (g_counter < MAX_PARTIAL) ? g_counter + 1 : g_counter;
		g_step_accumulator = 0;
		std::printf("Partial: %ld\n", static_cast<long>(g_counter));
	} else if (g_step_accumulator <= -2) {
		g_counter = (g_counter > 0) ? g_counter - 1 : g_counter;
		g_step_accumulator = 0;
		std::printf("Partial: %ld\n", static_cast<long>(g_counter));
	}
}

void poll_button() {
	const uint8_t current_but_state = static_cast<uint8_t>(gpio_get(ENC_BUT_PIN));
	if (current_but_state == g_last_but_state) {
		return;
	}

	const uint32_t now_ms = to_ms_since_boot(get_absolute_time());
	if ((now_ms - g_last_center_ms) > DEBOUNCE_MS) {
		g_last_but_state = current_but_state;
		(g_last_but_state == 0) ? std::printf("Started Playing\n"): std::printf("Stopped Playing\n");
		g_last_center_ms = now_ms;
	}
}

int main() {
	stdio_init_all();

	gpio_init(ENC_A_PIN);
	gpio_init(ENC_B_PIN);
	gpio_init(ENC_BUT_PIN);

	gpio_set_dir(ENC_A_PIN, false);
	gpio_set_dir(ENC_B_PIN, false);
	gpio_set_dir(ENC_BUT_PIN, false);

	gpio_pull_up(ENC_A_PIN);
	gpio_pull_up(ENC_B_PIN);
	gpio_pull_up(ENC_BUT_PIN);

	g_last_enc_state = static_cast<uint8_t>((gpio_get(ENC_A_PIN) << 1) | gpio_get(ENC_B_PIN));

	std::printf("Encoder ready on GPIO 18(A), 17(B)\n");
	std::printf("Push button on GPIO 5\n");
	std::printf("Rotate encoder or press buttons...\n");

	while (true) {
		poll_encoder();
		poll_button();
	}

	return 0;
}
