#include <Arduino.h>
#include <U8g2lib.h>
#include <SPI.h>
#include <Wire.h>

#include <AccelStepper.h>
#include <arduino-timer.h>


#include <PS4Controller.h>



#include <SerialCommands.h>

#include <exception>
#include <stdexcept>

#include "Mount.h"
#include "ScreenUI.h"



auto timer = timer_create_default();
auto stepper1 = AccelStepper(AccelStepper::DRIVER, 22, 15);
auto stepper2 = AccelStepper(AccelStepper::DRIVER, 2, 4);
// stepper1.setEnablePin(?);
// stepper2.setEnablePin(?);

//SCK - 18, MOSI - 23, SS - 5
U8G2_SH1106_128X64_NONAME_F_4W_HW_SPI u8g2(U8G2_R0, 5, 17, 16);
scope::Mount mount(stepper1, stepper2, Serial);
ui::ScreenUI screen(u8g2, mount);

char serialCommandBuffer[64];
SerialCommands serialCommands(&Serial, serialCommandBuffer, sizeof(serialCommandBuffer), "\r\n", " ");
void unrecognizedCmdCb(SerialCommands* sender, const char* cmd) {
	sender->GetSerial()->print("Unrecognized command [");
	sender->GetSerial()->print(cmd);
	sender->GetSerial()->println("]");
}
void moveToCmdCb(SerialCommands* sender) {
	auto positionXStr = sender->Next();
	if (positionXStr == nullptr) {
		sender->GetSerial()->println("Missing X position");
		return;
	}

	auto positionYStr = sender->Next();
	if (positionYStr == nullptr) {
		sender->GetSerial()->println("Missing Y position");
		return;
	}

	auto speedStr = sender->Next();
	if (speedStr == nullptr) {
		sender->GetSerial()->println("Missing speed");
		return;
	}

	auto speed = atoi(speedStr);
	if (speed <= 0) {
		sender->GetSerial()->println("Invalid speed");
		return;
	}
	auto positionX = atoi(positionXStr);
	auto positionY = atoi(positionYStr);
	mount.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO; //TODO make a wrapper to include this in safeMoveTo public method
 	mount.safeMoveTo({positionX, positionY}, speed);
}
SerialCommand moveToCmd("moveto", &moveToCmdCb);
void moveToDegCmdCb(SerialCommands* sender) {
	auto positionXStr = sender->Next();
	if (positionXStr == nullptr) {
		sender->GetSerial()->println("Missing X position");
		return;
	}

	auto positionYStr = sender->Next();
	if (positionYStr == nullptr) {
		sender->GetSerial()->println("Missing Y position");
		return;
	}

	auto speedStr = sender->Next();
	if (speedStr == nullptr) {
		sender->GetSerial()->println("Missing speed");
		return;
	}

	auto speed = atoi(speedStr);
	if (speed <= 0) {
		sender->GetSerial()->println("Invalid speed");
		return;
	}
	auto positionX = atof(positionXStr);
	auto positionY = atof(positionYStr);
	mount.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO; //TODO make a wrapper to include this in safeMoveTo public method
	mount.safeMoveToPositionDeg({positionX, positionY}, speed);
}
SerialCommand moveToDegCmd("movetodeg", &moveToDegCmdCb);
void moveToRADecCmdCb(SerialCommands* sender) {
	auto positionXStr = sender->Next();
	if (positionXStr == nullptr) {
		sender->GetSerial()->println("Missing X position");
		return;
	}

	auto positionYStr = sender->Next();
	if (positionYStr == nullptr) {
		sender->GetSerial()->println("Missing Y position");
		return;
	}

	auto speedStr = sender->Next();
	if (speedStr == nullptr) {
		sender->GetSerial()->println("Missing speed");
		return;
	}

	auto speed = atoi(speedStr);
	if (speed <= 0) {
		sender->GetSerial()->println("Invalid speed");
		return;
	}
	try {
		auto ra = coords::RA(positionXStr);
		auto dec = coords::Dec(positionYStr);
		mount.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO; //TODO make a wrapper to include this in safeMoveTo public method
		sender->GetSerial()->printf("movetoradec: RA{%f, %f, %f}, Dec{%f, %f, %f}\n", ra.h, ra.m, ra.s, dec.d, dec.m, dec.s);
		sender->GetSerial()->printf("movetoradec: RA: %s, Dec: %s}\n", ra.str().c_str(), dec.str().c_str());
		mount.safeMoveToPositionRADec({ra.rad(), dec.rad()}, speed);
	} catch (const std::invalid_argument& e) {
		sender->GetSerial()->println(e.what());
	}
}
SerialCommand moveToRADecCmd("movetoradec", &moveToRADecCmdCb);
void menuCmdCb(SerialCommands* sender) {
	auto param = sender->Next();
	if (param == nullptr) {
		sender->GetSerial()->println("Missing param (up,down,enter,exit)");
		return;
	}
	if (strcmp(param, "up") == 0) {
		screen.up();
	} else if (strcmp(param, "down") == 0) {
		screen.down();
	} else if (strcmp(param, "enter") == 0) {
		screen.enter();
	} else if (strcmp(param, "exit") == 0) {
		screen.exit();
	} else {
		sender->GetSerial()->println("Use on of (up,down,enter,exit)");
		return;
	}
}
SerialCommand menuCmd("menu", &menuCmdCb);

void setup() {
	Serial.begin(9600);
	u8g2.begin();
	PS4.begin("d8:fb:5e:69:d4:6a");
	stepper1.setPinsInverted(true, false, false);
	stepper2.setPinsInverted(true, false, false);

	// Read serial
	timer.every(20, [](void*) -> bool {
		serialCommands.ReadSerial();
		return true;
	});

	timer.every(50, [&u8g2, &screen](void*) -> bool {
		u8g2.clearBuffer();
		screen.draw();
		u8g2.sendBuffer();
		return true;
	});

	timer.every(100, [&mount](void*) -> bool {
		if (PS4.Down()) { screen.down(); }
		if (PS4.Up()) { screen.up(); }
		if (PS4.Cross()) { screen.enter(); }
		if (PS4.Circle()) { screen.exit(); }

		// set speed with deadzone
		auto speedX = abs(PS4.LStickX()) >= 10 ? -PS4.LStickX() : 0;
		auto speedY = abs(PS4.RStickY()) >= 10 ? PS4.RStickY() : 0;
		mount.manualControlSetSpeed({speedX, speedY});

		return true;
	});

	timer.every(200, [&mount](void*) -> bool {
		mount.computeAutoTrackCoords();
		return true;
	});

	serialCommands.SetDefaultHandler(&unrecognizedCmdCb);
	serialCommands.AddCommand(&moveToCmd);
	serialCommands.AddCommand(&moveToDegCmd);
	serialCommands.AddCommand(&moveToRADecCmd);
	serialCommands.AddCommand(&menuCmd);
}

void loop() {
	mount.tick();
	timer.tick();
}
