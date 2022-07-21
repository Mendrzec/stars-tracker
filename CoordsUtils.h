#pragma once

#include <Arduino.h>

#include <array>
#include <exception>
#include <stdexcept>
#include <string>
#include <tuple>

#include <functional>
#include <utility>
#include <vector>

namespace coords {

struct RA {
	constexpr RA(double h, double m, double s) : h(h), m(m), s(s) {}

	// accepts hh:mm:ss.ss format
	RA(std::string ra) {
		auto hoursEnd = ra.find(':');
		if (hoursEnd == std::string::npos) {
			throw std::invalid_argument("Invalid RA format. Must be hh:mm:ss.ss");
		}
		h = atof(ra.substr(0, hoursEnd).c_str());
		hoursEnd++;
		auto minutesEnd = ra.find(':', hoursEnd);
		if (minutesEnd == std::string::npos) {
			throw std::invalid_argument("Invalid RA format. Must be hh:mm:ss.ss");
		}
		m = atof(ra.substr(hoursEnd, minutesEnd - hoursEnd).c_str());
		minutesEnd++;
		s = atof(ra.substr(minutesEnd, ra.length() - minutesEnd).c_str());
	}

	double h, m, s;

	constexpr double deg() const {
		return h * 15 + (m * 15) / 60 + (s * 15) / 3600;
	}

	constexpr double rad() const {
		return deg() * DEG_TO_RAD;
	}

	std::string str() const {
		char buf[32];
		snprintf(buf, sizeof(buf), "%dh%dm%.2fs", static_cast<int>(h), static_cast<int>(m), s);
		return std::string(buf);
	}
};

struct Dec {
	constexpr Dec(double d, double m, double s) : d(d), m(m), s(s) {}

	// accepts ddd,mm,ss.ss format
	Dec(std::string dec) {
		auto degEnd = dec.find(',');
		if (degEnd == std::string::npos) {
			throw std::invalid_argument("Invalid Dec format. Must be ddd,mm,ss.ss");
		}
		d = atof(dec.substr(0, degEnd).c_str());
		degEnd++;
		auto minutesEnd = dec.find(',', degEnd);
		if (minutesEnd == std::string::npos) {
			throw std::invalid_argument("Invalid Dec format. Must be ddd,mm,ss.ss");
		}
		m = atof(dec.substr(degEnd, minutesEnd - degEnd).c_str());
		minutesEnd++;
		s = atof(dec.substr(minutesEnd, dec.length() - minutesEnd).c_str());
	}

	double d, m ,s;

	constexpr double deg() const {
		return d + m / 60 + s / 3600;
	}

	constexpr double rad() const {
		return deg() * DEG_TO_RAD;
	}

	std::string str() const {
		char buf[32];
		snprintf(buf, sizeof(buf), "%d^%d\'%.2f\"", static_cast<int>(d), static_cast<int>(m), s);
		return std::string(buf);
	}
};

RA degToRA(double deg) {
	static constexpr const auto hoursFactor = 15.0;
	static constexpr const auto minutesFactor = 15.0/60;
	static constexpr const auto secondsFactor = 15.0/3600;

	auto hours = std::floor(deg / hoursFactor);
	deg = std::fmod(deg, hoursFactor);
	auto minutes = std::floor(deg / minutesFactor);
	deg = std::fmod(deg, minutesFactor);
	auto seconds = deg / secondsFactor;

	return RA{hours, minutes, seconds};
}

Dec degToDec(double deg) {
	static constexpr const auto degreesFactor = 1.0;
	static constexpr const auto minutesFactor = 1.0/60;
	static constexpr const auto secondsFactor = 1.0/3600;

	auto degrees = std::floor(deg / degreesFactor);
	deg = std::fmod(deg, degreesFactor);
	auto minutes = std::floor(deg / minutesFactor);
	deg = std::fmod(deg, minutesFactor);
	auto seconds = deg / secondsFactor;

	return Dec{degrees, minutes, seconds};
}

struct CelestialObjectBase {
	constexpr CelestialObjectBase(const char* name, RA ra, Dec dec) : name_(name), ra_(std::move(ra)), dec_(std::move(dec)) {}

	const char* name_;
	RA ra_;
	Dec dec_;
};

template<typename CelestialObjectType>
struct CelestialObject : public CelestialObjectBase {
	constexpr CelestialObject(CelestialObjectType key, const char* name, RA ra, Dec dec)
		: key_(key), CelestialObjectBase(name, std::move(ra), std::move(dec)) {}
	CelestialObjectType key_;
};

// radians per second
constexpr const double EARTH_ANG_SPEED = 7.292115 * pow(10,-5);

std::pair<double, double> rotatePoint(std::pair<double, double> point, double angle, std::pair<double, double> pivot = {0, 0}) {
	// TODO can be optimized: do not compute twice sin, cos, and others
	return {
		(point.first - pivot.first)*cos(angle) - (point.second - pivot.second)*sin(angle) + pivot.first,
		(point.first - pivot.first)*sin(angle) + (point.second - pivot.second)*cos(angle) + pivot.second
	};
}

std::pair<double, double> translatePoint(std::pair<double, double> point, std::pair<double, double> delta) {
	return {
		point.first + delta.first,
		point.second + delta.second
	};
}

std::pair<double,double> lineFrom2Points(std::pair<double, double> point1, std::pair<double, double> point2) {
	// TODO can be optimized: do not compute twice some expressions
	return {
		(point1.second - point2.second)/(point1.first - point2.first),
		point1.second - (point1.second - point2.second)/(point1.first - point2.first)*point1.first
	};
}

double angleFrom2Lines(double a1, double a2) {
	return atan((a2 - a1)/(1 + a1*a2));
}

std::pair<double, double> deltaXdeltaYFrom2Points(std::pair<double, double> point1, std::pair<double, double> point2, double angle = 0) {
	return {
		point2.first - point1.first*cos(angle) + point1.second*sin(angle),
		point2.second - point1.first*sin(angle) - point1.second*cos(angle)
	};
}




}