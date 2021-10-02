#pragma once

#include "CoordsUtils.h"

#include <Arduino.h>
#include <AccelStepper.h>
#include <Stream.h>
#include <WiFi.h>

#include <utility>

#define DEBUG_MOUNT

#ifdef DEBUG_MOUNT
#define LOG_DEBUG(x) x
#else
#define LOG_DEBUG(x)
#endif

namespace scope {

class Mount {
public:
	static constexpr const int X_AXIS_DRIVER_STEP_DIV = 16;
	static constexpr const int X_AXIS_GEAR_RATIO = 8;
	static constexpr const int X_AXIS_STEPS_PER_REV = 200 * X_AXIS_DRIVER_STEP_DIV * X_AXIS_GEAR_RATIO;
	static constexpr const int X_AXIS_LOWER_LIMIT = -X_AXIS_STEPS_PER_REV * 205.0/360;
	static constexpr const int X_AXIS_UPPER_LIMIT = X_AXIS_STEPS_PER_REV * 25.0/360;
	static constexpr const int X_AXIS_HOME = 0;
	static constexpr const double X_AXIS_STEPS_TO_ANGLE_DEG = 360.0 / X_AXIS_STEPS_PER_REV;
	static constexpr const double X_AXIS_STEPS_TO_ANGLE_RAD = X_AXIS_STEPS_TO_ANGLE_DEG * DEG_TO_RAD;
	static constexpr const double X_AXIS_ANGLE_RAD_TO_STEPS = RAD_TO_DEG / 360.0 * X_AXIS_STEPS_PER_REV;

	static constexpr const int Y_AXIS_DRIVER_STEP_DIV = 16;
	static constexpr const int Y_AXIS_GEAR_RATIO = 8;
	static constexpr const int Y_AXIS_STEPS_PER_REV = 200 * Y_AXIS_DRIVER_STEP_DIV * Y_AXIS_GEAR_RATIO;
	static constexpr const int Y_AXIS_LOWER_LIMIT = -Y_AXIS_STEPS_PER_REV * 90/360;
	static constexpr const int Y_AXIS_UPPER_LIMIT = Y_AXIS_STEPS_PER_REV * 270.0/360;
	static constexpr const int Y_AXIS_HOME = Y_AXIS_STEPS_PER_REV * 90/360;
	static constexpr const double Y_AXIS_STEPS_TO_ANGLE_DEG = 360.0 / Y_AXIS_STEPS_PER_REV;
	static constexpr const double Y_AXIS_STEPS_TO_ANGLE_RAD = Y_AXIS_STEPS_TO_ANGLE_DEG * DEG_TO_RAD;
	static constexpr const double Y_AXIS_ANGLE_RAD_TO_STEPS = RAD_TO_DEG / 360.0 * Y_AXIS_STEPS_PER_REV;

	static constexpr const int MAX_SPEED = 400;
	static constexpr const int MAX_ACCELERATION = 800;

	enum MountType : uint8_t {
		EQ,
		AZ
	};

	enum OperationMode : uint8_t {
		UNINITIALIZED,
		FULL_GOTO,
		EASY_TRACK,
		EASY_TRACK_GOTO
	};

	enum TrackingMode : uint8_t {
		MANUAL_CONTROL,
		AUTO_TRACKING,
		MOVE_TO
	};

	Mount(AccelStepper& stepperX, AccelStepper& stepperY, Stream& serial) : stepperX_(stepperX), stepperY_(stepperY), serial_(serial) {
		// stepperX_.disableOutputs();
		stepperX_.setMaxSpeed(MAX_SPEED);
		stepperX_.setAcceleration(MAX_ACCELERATION);
		stepperX_.setCurrentPosition(X_AXIS_HOME);

		// stepperY_.disableOutputs();
		stepperY_.setMaxSpeed(MAX_SPEED);
		stepperY_.setAcceleration(MAX_ACCELERATION);
		stepperY_.setCurrentPosition(Y_AXIS_HOME);
	}

	void disableSteppers() {
		stepperX_.disableOutputs();
		stepperY_.disableOutputs();
	}

	void enableSteppers() {
		stepperX_.enableOutputs();
		stepperY_.enableOutputs();
	}

	void setAutoTrackPivot() {
		if (mountType_ == MountType::EQ) {
			stepperX_.setCurrentPosition(X_AXIS_HOME);
			stepperY_.setCurrentPosition(Y_AXIS_HOME);
			autoTrackPivotSet_ = true;
		} else {
			autoTrackPivot_ = std::pair<double, double>{stepperX_.currentPosition(), stepperY_.currentPosition()};
			autoTrackPivotSet_ = true;
		}
	}

	void startAutoTrack() {
		if (mountType_ == MountType::AZ && !autoTrackPivotSet_) {
			return;
		}
		double timestamp = 0;
		if (!getTimeOfDaySeconds(timestamp)) {
			return;
		}
		autoTrackStartTimeStamp_ = timestamp;
		autoTrackStartCoords_ = std::pair<double, double>{stepperX_.currentPosition(), stepperY_.currentPosition()};
		trackingMode_ = TrackingMode::AUTO_TRACKING;
	}

	void stopAutoTrack() {
		trackingMode_ = TrackingMode::MANUAL_CONTROL;
		stepperX_.setSpeed(0);
		stepperY_.setSpeed(0);
	}

	void toggleAutoTrack() {
		if (trackingMode_ == TrackingMode::MANUAL_CONTROL) {
			startAutoTrack();
		} else {
			stopAutoTrack();
		}
	}

	// call with longer interval eg. 200ms
	void computeAutoTrackCoords() {
		if (trackingMode_ != TrackingMode::AUTO_TRACKING) {
			return;
		}
		auto angle = getEarthDeltaAngleSinceTimestamp(autoTrackStartTimeStamp_);

		LOG_DEBUG(serial_.printf("computeAutoTrackCoords() angle delta(rad): %f\n", angle));
		LOG_DEBUG(serial_.printf("computeAutoTrackCoords() start coords(rad): %f, %f\n", autoTrackStartCoords_.first * X_AXIS_STEPS_TO_ANGLE_RAD, autoTrackStartCoords_.second * Y_AXIS_STEPS_TO_ANGLE_RAD));
		if (mountType_ == MountType::EQ) {
			targetCoords_ = coords::translatePoint(autoTrackStartCoords_, {X_AXIS_ANGLE_RAD_TO_STEPS * angle, 0});
		} else {
			LOG_DEBUG(serial_.printf("computeAutoTrackCoords() pivot coords(rad): %f, %f\n", autoTrackPivot_.first * X_AXIS_STEPS_TO_ANGLE_RAD, autoTrackPivot_.second * Y_AXIS_STEPS_TO_ANGLE_RAD));
			targetCoords_ = coords::rotatePoint(autoTrackStartCoords_, angle, autoTrackPivot_);
		}
		LOG_DEBUG(serial_.printf("computeAutoTrackCoords() target coords(rad): %f, %f\n", targetCoords_.first * X_AXIS_STEPS_TO_ANGLE_RAD, targetCoords_.second * Y_AXIS_STEPS_TO_ANGLE_RAD));
		safeMoveTo({targetCoords_});
	}

	// PS4 range: -128 : 127  int8_t
	void manualControlSetSpeed(std::pair<int8_t, int8_t> speedXY) {
		if (speedXY == lastManualControlSpeed_) {
			return;
		}
		lastManualControlSpeed_ = speedXY;

		auto newSpeedX = speedXY.first/128.0 * MAX_SPEED;
		auto newSpeedY = speedXY.second/128.0 * MAX_SPEED;

		trackingMode_ = TrackingMode::MANUAL_CONTROL;
		stepperX_.setSpeed(newSpeedX);
		stepperY_.setSpeed(newSpeedY);
	}


	// This applies limits and reduces revolutions
	template<typename T>
	bool normalizeTargetSteps(std::pair<T, T>& targetPosition) {
		auto position = targetPosition;

		position.first = std::fmod(position.first, X_AXIS_STEPS_PER_REV);
		while (position.first > X_AXIS_UPPER_LIMIT) {
			position.first -= X_AXIS_STEPS_PER_REV / 2;
			position.second = Y_AXIS_STEPS_PER_REV / 2 - position.second;
		}
		while (position.first < X_AXIS_LOWER_LIMIT) {
			position.first += X_AXIS_STEPS_PER_REV / 2;
			position.second = Y_AXIS_STEPS_PER_REV / 2 - position.second;
		}
		position.second = std::fmod(position.second, Y_AXIS_STEPS_PER_REV);

		targetPosition = position;
		return true;
	}

	void stopMountMove() {
		LOG_DEBUG(serial_.println("stopMountMove() stopping mount"));
		safeMoveTo({stepperX_.currentPosition(), stepperY_.currentPosition()});
	}

	void safeMoveTo(std::pair<int, int> position, int speed = MAX_SPEED) {
		LOG_DEBUG(serial_.printf("safeMoveTo() moveto(steps): %d, %d\n", position.first, position.second));
		if (!normalizeTargetSteps(position)) {
			return;
		}
		LOG_DEBUG(serial_.printf("safeMoveTo() moveto(limits,steps): %d, %d\n", position.first, position.second));
		stepperX_.moveTo(stepperX_.currentPosition());
		stepperY_.moveTo(stepperY_.currentPosition());
		stepperX_.setMaxSpeed(speed);
		stepperX_.moveTo(position.first);
		stepperY_.setMaxSpeed(speed);
		stepperY_.moveTo(position.second);
	}

	void safeMoveToPositionRad(std::pair<double, double> position, int speed = MAX_SPEED) {
		LOG_DEBUG(serial_.printf("safeMoveToPositionRad(): position(rad) %f, %f\n", position.first, position.second));
		safeMoveTo({position.first * X_AXIS_ANGLE_RAD_TO_STEPS, position.second * Y_AXIS_ANGLE_RAD_TO_STEPS}, speed);
	}

	void safeMoveToPositionDeg(std::pair<double, double> position, int speed = MAX_SPEED) {
		LOG_DEBUG(serial_.printf("safeMoveToPositionDeg(): position(deg) %f, %f\n", position.first, position.second));
		safeMoveToPositionRad({position.first * DEG_TO_RAD, position.second * DEG_TO_RAD}, speed);
	}

	void safeMoveToPositionRADec(std::pair<double, double> position, int speed  = MAX_SPEED) {
		LOG_DEBUG(serial_.printf("safeMoveToPositionRADec(): position(rad) %f, %f\n", position.first, position.second));
		auto angle = getEarthDeltaAngleSinceTimestamp(alignmentTimestamp_);
		LOG_DEBUG(serial_.printf("safeMoveToPositionRADec(): angle delta(rad) %f\n", angle));

		if (mountType_ == MountType::EQ) {
			auto mountPositionRad = coords::translatePoint(position, alignmentDelta_);
			safeMoveToPositionRad(coords::translatePoint(mountPositionRad, {angle, 0}), speed);
		} else {
			if (!skyPivotSet_) {
				serial_.print("safeMoveToPositionRADec(). skyPivot not set.");
				//TODO show error somehow
				return;
			}
			auto mountPositionRad = coords::translatePoint(coords::rotatePoint(position, alignmentAngle_), alignmentDelta_);
			safeMoveToPositionRad(coords::rotatePoint(mountPositionRad, angle, skyPivotRad_), speed);
		}
	}

	bool getTimeOfDaySeconds(double& result) const {
		timeval tv;
		if (gettimeofday(&tv, NULL) != 0) {
			return false;
		}
		result = static_cast<double>(tv.tv_sec) + tv.tv_usec*pow(10,-6);
		return true;
	}

	double getEarthDeltaAngleSinceTimestamp(double time) const {
		double timestamp = 0;
		if (!getTimeOfDaySeconds(timestamp)) {
			serial_.print("safeMoveToPositionRADec(). gettimeofday() failed.");
			//TODO show error somehow
			return 0;
		}
		auto timeDiff = timestamp - time;
		return -coords::EARTH_ANG_SPEED * timeDiff;
	}

	std::pair<double, double> currentPositionDeg() const {
		return {stepperX_.currentPosition() * X_AXIS_STEPS_TO_ANGLE_DEG, stepperY_.currentPosition() * Y_AXIS_STEPS_TO_ANGLE_DEG};
	}

	std::pair<double, double> currentPositionRad() const {
		auto position = currentPositionDeg();
		return {position.first * DEG_TO_RAD, position.second * DEG_TO_RAD};
	}

	std::pair<double, double> currentPositionDegEQNormalized() const {
		auto position = currentPositionDeg();

		position.second = std::fmod(position.second, 360);
		while (position.second > 90 || position.second < -90) {
			position.first += 180;
			if (position.second > 90) {
				position.second = -position.second + 180;
			} else {
				position.second = -position.second - 180;
			}
		}

		position.first = std::fmod(position.first, 360);
		while (position.first < 0) { position.first += 360; }

		return position;
	}

	std::pair<double, double> currentPositionRadEQNormalized() const {
		auto position = currentPositionDegEQNormalized();
		return {position.first * DEG_TO_RAD, position.second * DEG_TO_RAD};
	}

	// coords::RA currentPositionRADecNormalized() const {
	// 	// TODO use alignment angle and current rotation to be correct
	// 	return coords::degToRA(currentPositionXDeg());
	// }

	// coords::Dec currentPositionDec() const {
	// 	// TODO use alignment angle and current rotation to be correct
	// 	return coords::degToDec(currentPositionYDeg());
	// }

	double targetPositionXDeg() const {
		return stepperX_.targetPosition() * X_AXIS_STEPS_TO_ANGLE_DEG;
	}

	double targetPositionYDeg() const {
		return stepperY_.targetPosition() * Y_AXIS_STEPS_TO_ANGLE_DEG;
	}

	double targetPositionXRad() const {
		return stepperX_.targetPosition() * X_AXIS_STEPS_TO_ANGLE_RAD;
	}

	double targetPositionYRad() const {
		return stepperY_.targetPosition() * Y_AXIS_STEPS_TO_ANGLE_RAD;
	}

	void setTwoStarAlignmentFirstStar(std::pair<double, double> firstStarRAandDecRad) {
		twoStarAlignmentFirstStarRad_ = firstStarRAandDecRad;
		twoStarAlignmentFirstStarMountRad_ = currentPositionRadEQNormalized();
		twoStarAlignmentFirstStarSet_ = true;
	}

	void setTwoStarAlignmentSecondStar(std::pair<double, double> secondStarRAandDecRad) {
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): first star mount(rad) %f, %f\n", twoStarAlignmentFirstStarMountRad_.first, twoStarAlignmentFirstStarMountRad_.second));
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): first star(rad) %f, %f\n", twoStarAlignmentFirstStarRad_.first, twoStarAlignmentFirstStarRad_.second));
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): second star mount(rad) %f, %f\n", currentPositionRadEQNormalized().first, currentPositionRadEQNormalized().second));
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): second star(rad) %f, %f\n", secondStarRAandDecRad.first, secondStarRAandDecRad.second));

		double timestamp = 0;
		if (!getTimeOfDaySeconds(timestamp)) {
			serial_.print("setTwoStarAlignmentSecondStar(). gettimeofday() failed.");
			//TODO show error somehow
			return;
		}
		alignmentTimestamp_ = timestamp;

		// EQ specific
		if (mountType_ == MountType::EQ) {
			auto deltaXYFirstStar = coords::deltaXdeltaYFrom2Points(twoStarAlignmentFirstStarRad_, twoStarAlignmentFirstStarMountRad_);
			auto deltaXYSecondStar = coords::deltaXdeltaYFrom2Points(secondStarRAandDecRad, currentPositionRadEQNormalized());
			alignmentDelta_ = {
					(deltaXYFirstStar.first + deltaXYSecondStar.first) / 2,
					(deltaXYFirstStar.second + deltaXYSecondStar.second) / 2
			};
			LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): alignment delta(rad) %f, %f\n", alignmentDelta_.first, alignmentDelta_.second));
			return;
		}

		// AZ specific
		// TODO this may be incorrect - check this logic again
		alignmentAngle_ = coords::angleFrom2Lines(
				coords::lineFrom2Points(twoStarAlignmentFirstStarRad_, secondStarRAandDecRad).first,
				coords::lineFrom2Points(twoStarAlignmentFirstStarMountRad_, currentPositionRadEQNormalized()).first
		);
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): alignment angle(rad) %f\n", alignmentAngle_));

		alignmentDelta_ = coords::deltaXdeltaYFrom2Points(twoStarAlignmentFirstStarRad_, twoStarAlignmentFirstStarMountRad_, alignmentAngle_);
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): alignment delta(rad) %f, %f\n", alignmentDelta_.first, alignmentDelta_.second));

		skyPivotRad_ = coords::translatePoint(coords::rotatePoint({0, 90 * DEG_TO_RAD}, alignmentAngle_), alignmentDelta_);
		skyPivotSet_ = true;
		autoTrackPivot_ = {skyPivotRad_.first * X_AXIS_ANGLE_RAD_TO_STEPS, skyPivotRad_.second * Y_AXIS_ANGLE_RAD_TO_STEPS};
		if (normalizeTargetSteps(autoTrackPivot_)) {
			autoTrackPivotSet_ = true;
		}
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): sky pivot(rad) %f, %f\n", skyPivotRad_.first, skyPivotRad_.second));
		LOG_DEBUG(serial_.printf("setTwoStarAlignmentSecondStar(): autotrack pivot(steps) %f, %f\n", autoTrackPivot_.first, autoTrackPivot_.second));
	}

	// call as frequent as possible
	void tick() {
		// TODO reset target to current and no return
		// if ((stepperX_.targetPosition() > X_AXIS_UPPER_LIMIT) || (stepperX_.targetPosition() < X_AXIS_LOWER_LIMIT) ||
		// 	(stepperY_.targetPosition() > Y_AXIS_UPPER_LIMIT) || (stepperY_.targetPosition() < Y_AXIS_LOWER_LIMIT)) {
		// 	return;
		// }

		//TODO check speed() also
		// if ((stepperX_.currentPosition() >= X_AXIS_UPPER_LIMIT) || (stepperX_.currentPosition() <= X_AXIS_LOWER_LIMIT)) {
		// 	stepperX_.setSpeed(0);
		// }
		// if ((stepperY_.currentPosition() >= Y_AXIS_UPPER_LIMIT) || (stepperY_.currentPosition() <= Y_AXIS_LOWER_LIMIT)) {
		// 	stepperY_.setSpeed(0);
		// }
		if (trackingMode_ == TrackingMode::MANUAL_CONTROL) {
			stepperX_.runSpeed();
			stepperY_.runSpeed();
		} else {
			stepperX_.run();
			stepperY_.run();
		}
	}

	AccelStepper& stepperX_;
	AccelStepper& stepperY_;
	Stream& serial_;
	MountType mountType_ = MountType::EQ;
	OperationMode operationMode_ = OperationMode::UNINITIALIZED;
	TrackingMode trackingMode_ = TrackingMode::MANUAL_CONTROL;

	bool autoTrackPivotSet_ = false;
	std::pair<double, double> autoTrackPivot_ = {0, 0};
	std::pair<double, double> autoTrackStartCoords_ = {0, 0};
	double autoTrackStartTimeStamp_ = 0;
	std::pair<int8_t, int8_t> lastManualControlSpeed_ = {0, 0};
	std::pair<double, double> targetCoords_ = {0, 0};

	bool twoStarAlignmentFirstStarSet_ = false;
	std::pair<double, double> twoStarAlignmentFirstStarRad_ = {0, 0};
	std::pair<double, double> twoStarAlignmentFirstStarMountRad_ = {0, 0};
	double alignmentAngle_ = 0;
	std::pair<double, double> alignmentDelta_ = {0, 0};
	double alignmentTimestamp_ = 0;
	bool skyPivotSet_ = false;
	std::pair<double, double> skyPivotRad_ = {0, 0};
};

}