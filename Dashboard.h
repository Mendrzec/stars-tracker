#pragma once

#include "Mount.h"
#include "ScreenItemIfc.h"

#include <U8g2lib.h>

#include <functional>
#include <utility>

namespace ui {

using scope::Mount;

class Dashboard : public ScreenItem {
	using Handler = std::function<void()>;
	using Items = std::vector<std::pair<const char*, bool>>;

	enum Item : uint8_t {
		CTRL,
		GOTO,
		SETTINGS,
		first = CTRL,
		last = SETTINGS,
	};
public:
	Dashboard(U8G2& u8g2, Mount& mount, Handler gotoHandler/*, Handler settingsHandler*/)
		: u8g2_(u8g2), mount_(mount), gotoHandler_(std::move(gotoHandler))/*, settingsHandler_(std::move(settingsHandler))*/ {}

	void draw() override {
		u8g2_.setFont(u8g2_font_profont11_tf);

		u8g2_.setDrawColor(1);
		u8g2_.drawBox(0, focusedItem_ * 11, 64, 11);

		u8g2_.setFontMode(1);
		u8g2_.setDrawColor(2);
		if (mount_.trackingMode_ == Mount::MANUAL_CONTROL) {
			u8g2_.drawStr(1, 10, "CTRL: MAN");
		} else if (mount_.trackingMode_ == Mount::MOVE_TO) {
			u8g2_.drawStr(1, 10, "CTRL: MOV");
		} else {
			u8g2_.drawStr(1, 10, "CTRL: AUT");
		}
		u8g2_.drawStr(1, 20, "GOTO");
		u8g2_.drawStr(1, 30, "SETTINGS");

		u8g2_.drawVLine(64, 0, 64);

		if (mount_.operationMode_ == Mount::FULL_GOTO) {
			u8g2_.drawStr(66, 10, "MODE: FULL");
		} else if (mount_.operationMode_ == Mount::EASY_TRACK) {
			u8g2_.drawStr(66, 10, "MODE: EASY");
		} else if (mount_.operationMode_ == Mount::EASY_TRACK_GOTO) {
			u8g2_.drawStr(66, 10, "MODE: EASY GT");
		} else {
			u8g2_.drawStr(66, 10, "MODE: -");
		}

		char s[32];
		auto currentPosition = mount_.currentPositionDeg();
		snprintf(s, sizeof(s), "X:  %3.2f", currentPosition.first);
		u8g2_.drawStr(66, 20, s);
		snprintf(s, sizeof(s), "Y:  %3.2f", currentPosition.second);
		u8g2_.drawStr(66, 30, s);
		// if (mount_.operationMode_ == Mount::OperationMode::EASY_TRACK_GOTO || mount_.operationMode_ == Mount::OperationMode::FULL_GOTO) {
		// 	snprintf(s, sizeof(s), "%s", mount_.currentPositionRA().str().c_str());
		// 	u8g2_.drawStr(66, 40, s);
		// 	snprintf(s, sizeof(s), "%s", mount_.currentPositionDec().str().c_str());
		// 	u8g2_.drawStr(66, 50, s);
		// }
		snprintf(s, sizeof(s), "TX: %3.2f", mount_.targetPositionXDeg());
		u8g2_.drawStr(66, 40, s);
		snprintf(s, sizeof(s), "TY: %3.2f", mount_.targetPositionYDeg());
		u8g2_.drawStr(66, 50, s);
	}
	void down() override {
		if (focusedItem_ < Item::last) {
			focusedItem_ += 1;
		}
	}
	void up() override {
		if (focusedItem_ > Item::first) {
			focusedItem_ -= 1;
		}
	}
	void enter() override {
		switch(focusedItem_) {
			case Item::CTRL:
				mount_.toggleAutoTrack();
				break;
			case Item::GOTO:
				gotoHandler_();
				break;
			default:
				break;
		}
	}
	void exit() override {}


	U8G2& u8g2_;
	Mount& mount_;
	Handler gotoHandler_;
	Handler settingsHandler_;
	uint8_t focusedItem_ = Item::CTRL;
};

}