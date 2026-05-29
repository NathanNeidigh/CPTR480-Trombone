from machine import Pin
import time


# Rotary encoder pins
ENC_A_PIN = 18
ENC_B_PIN = 17

# Buttons (active-low with internal pull-ups)
PUSH_BUTTON_PIN = 21
CENTER_CLICK_PIN = 5


enc_a = Pin(ENC_A_PIN, Pin.IN, Pin.PULL_UP)
enc_b = Pin(ENC_B_PIN, Pin.IN, Pin.PULL_UP)
push_button = Pin(PUSH_BUTTON_PIN, Pin.IN, Pin.PULL_UP)
center_click = Pin(CENTER_CLICK_PIN, Pin.IN, Pin.PULL_UP)


# Transition table for quadrature decoding:
# previous_state (2 bits) + current_state (2 bits) -> direction
# +1 for CW, -1 for CCW, 0 for invalid/bounce transitions
_TRANSITIONS = {
	0b0001: -1,
	0b0111: -1,
	0b1110: -1,
	0b1000: -1,
	0b0010: 1,
	0b1011: 1,
	0b1101: 1,
	0b0100: 1,
}


counter = 0
_last_enc_state = (enc_a.value() << 1) | enc_b.value()
_step_accumulator = 0

_last_push_ms = 0
_last_center_ms = 0
_DEBOUNCE_MS = 150


def encoder_irq(_pin):
	global counter, _last_enc_state, _step_accumulator

	current_state = (enc_a.value() << 1) | enc_b.value()
	transition = (_last_enc_state << 2) | current_state
	direction = _TRANSITIONS.get(transition)
	assert direction is not None, "Invalid encoder transition: {:04b} -> {:04b}".format(_last_enc_state, current_state)	

	_step_accumulator += direction

	# 4 valid transitions per detent.
	if _step_accumulator >= 2:
		counter += 1
		_step_accumulator = 0
		print("CW  count:", counter)
	elif _step_accumulator <= -2:
		counter -= 1
		_step_accumulator = 0
		print("CCW count:", counter)

	_last_enc_state = current_state


def push_button_irq(_pin):
	global _last_push_ms

	now = time.ticks_ms()
	if time.ticks_diff(now, _last_push_ms) > _DEBOUNCE_MS:
		if push_button.value() == 0:
			print("Push button pressed")
			_last_push_ms = now


def center_click_irq(_pin):
	global _last_center_ms

	now = time.ticks_ms()
	if time.ticks_diff(now, _last_center_ms) > _DEBOUNCE_MS:
		if center_click.value() == 0:
			print("Center click pressed")
			_last_center_ms = now


enc_a.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=encoder_irq)
enc_b.irq(trigger=Pin.IRQ_RISING | Pin.IRQ_FALLING, handler=encoder_irq)
push_button.irq(trigger=Pin.IRQ_FALLING, handler=push_button_irq)
center_click.irq(trigger=Pin.IRQ_FALLING, handler=center_click_irq)


print("Encoder ready on GPIO 18(A), 17(B)")
print("Push button on GPIO 21, center click on GPIO 5")
print("Rotate encoder or press buttons...")

while True:
	time.sleep_ms(100)
