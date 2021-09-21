#pragma once

namespace ui {

class ScreenItem {
public:
	virtual void draw() = 0;
	virtual void down() = 0;
	virtual void up() = 0;
	virtual void enter() = 0;
	virtual void exit() = 0;
};

}