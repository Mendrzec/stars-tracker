#include "ItemsList.h"


namespace ui {

void ItemsList::draw() {
	u8g2_.setFont(u8g2_font_profont11_tf);
	u8g2_.setFontMode(1);

	u8g2_.setDrawColor(2);
	u8g2_.drawFrame(0, 0, 127, lineHeight_ + borderWidth_);
	u8g2_.drawStr(textPadding_, lineHeight_ - borderWidth_, title_);

	auto textLines = text_.size();
	for (auto i = 0; i < textLines; ++i) {
		u8g2_.drawStr(textPadding_, (i + 2) * lineHeight_ , text_[i].c_str());
	}
	auto itemsOffset = textLines + 1; // 1 for title

	for (auto i = 0; i < maxElements_ - textLines && i + viewOffset_ < items_.size(); ++i) {
		if (i + viewOffset_ == focused_) {
			u8g2_.setDrawColor(1);
			u8g2_.drawBox(0, (i + itemsOffset) * lineHeight_ + borderWidth_, 127, lineHeight_ + borderWidth_);
		}
		u8g2_.setDrawColor(2);
		u8g2_.drawStr(textPadding_, (i + itemsOffset + 1) * lineHeight_, items_[i + viewOffset_].first);
	}
}

void ItemsList::down() {
	if (focused_ < items_.size() - 1) {
		++focused_;
	}
	if (focused_ >= viewOffset_ + maxElements_ - text_.size()) {
		++viewOffset_;
	}
}

void ItemsList::up() {
	if (focused_ > 0) {
		--focused_;
	}
	if (focused_ < viewOffset_) {
		--viewOffset_;
	}
}

}
