#pragma once

#include <functional>

struct ButtonProcessor {
	static constexpr const int PS4_READ_INTERVAL_MS = 50;
	static constexpr const int BUTTON_PRESSED_UNTIL_SCROLL_TIME_MS = 1000;
	static constexpr const int BUTTON_PRESSED_UNTIL_SCROLL_TICKS = BUTTON_PRESSED_UNTIL_SCROLL_TIME_MS / PS4_READ_INTERVAL_MS;
	using ButtonPressedFn = std::function<bool()>;

	ButtonProcessor(ButtonPressedFn buttonPressedFn) : buttonPressedFn_(std::move(buttonPressedFn)) {}

	bool operator()() {
		bool result = false;
		auto pressed = buttonPressedFn_();
		if (pressed) {
			++buttonPressedTicksCount_;
		} else {
			buttonPressedTicksCount_ = 0;
		}

		if ((!prevButtonPressed_ && pressed) || buttonPressedTicksCount_ >= BUTTON_PRESSED_UNTIL_SCROLL_TICKS) {
			prevButtonPressed_ = pressed;
			return true;
		} else {
			prevButtonPressed_ = pressed;
			return false;
		}
	}

	ButtonPressedFn buttonPressedFn_;
	bool prevButtonPressed_ = false;
	int buttonPressedTicksCount_ = 0;


};