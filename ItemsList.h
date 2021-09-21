#pragma once


#include "ScreenItemIfc.h"

#include <functional>
#include <string>
#include <vector>
#include <utility>
#include <U8g2lib.h>


namespace ui {

class ItemsList : public ScreenItem {
	static constexpr const std::size_t MAX_ELEMENTS = 5;
	static constexpr const uint8_t LINE_HEIGHT = 10;
	static constexpr const uint8_t TEXT_PADDING = 2;
	static constexpr const uint8_t BORDER_WIDTH = 1;

public:
	using Text = std::vector<std::string>;
	using Handler = std::function<void()>;
	using Items = std::vector<std::pair<const char*, Handler>>;

	ItemsList(U8G2& u8g2, const char* title, Text text, Items items, Handler exitHandler)
		: u8g2_(u8g2), title_(title), text_(std::move(text)), items_(std::move(items)), exitHandler_(std::move(exitHandler)) {}

	ItemsList(U8G2& u8g2, const char* title, Text text, Items items)
		: u8g2_(u8g2), title_(title), text_(std::move(text)), items_(std::move(items)) {}

	void draw() override;
	void down() override;
	void up() override;

	void enter() override {
		items_[focused_].second();
	}

	void exit() override {
		if (exitHandler_) {
			exitHandler_();
		}
	}

	U8G2& u8g2_;
	const char* title_;
	Text text_;
	Items items_;
	Handler exitHandler_;
	std::size_t viewOffset_ = 0;
	std::size_t focused_ = 0;

	std::size_t maxElements_ = MAX_ELEMENTS;
	uint8_t lineHeight_ = LINE_HEIGHT;
	uint8_t textPadding_ = TEXT_PADDING;
	uint8_t borderWidth_ = BORDER_WIDTH;
};

}