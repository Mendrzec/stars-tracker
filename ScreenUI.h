#pragma once

#include "CoordsUtils.h"
#include "Dashboard.h"
#include "ItemsList.h"
#include "Mount.h"
#include "CelestialObjects/Stars/Stars.h"

#include <U8g2lib.h>


namespace ui {

using scope::Mount;

class ScreenUI {
public:
	explicit ScreenUI(U8G2& u8g2, Mount& mount) : u8g2_(u8g2), mount_(mount) {}

	void draw() { currentScreen_->draw(); }
	void up() { currentScreen_->up(); }
	void down() { currentScreen_->down(); }
	void enter() { currentScreen_->enter(); }
	void exit() { currentScreen_->exit(); }

	template<typename Stars, std::size_t... I>
	ItemsList::Items unpackStarsForTwoStarAlignmentFirstStar_(const Stars& stars, std::index_sequence<I...>) {
		return ItemsList::Items{
			{stars[I].name_, [this, &star=stars[I]]() {
				selectedStar_ = &star;
				twoStarAlignmentFirstStarConfirm_.text_ = {std::string("Move to ") + star.name_ + " and", "press OK"};
				currentScreen_ = &twoStarAlignmentFirstStarConfirm_;
			}}...
		};
	}

	ItemsList::Items unpackStarsForTwoStarAlignmentFirstStar() {
		return unpackStarsForTwoStarAlignmentFirstStar_(coords::STARS, std::make_index_sequence<coords::STARS.size()>{});
	}

	template<typename Stars, std::size_t... I>
	ItemsList::Items unpackStarsForTwoStarAlignmentSecondStar_(const Stars& stars, std::index_sequence<I...>) {
		return ItemsList::Items{
			{stars[I].name_, [this, &star=stars[I]]() {
				selectedStar_ = &star;
				twoStarAlignmentSecondStarConfirm_.text_ = {std::string("Move to ") + star.name_ + " and", "press OK"};
				currentScreen_ = &twoStarAlignmentSecondStarConfirm_;
			}}...
		};
	}

	ItemsList::Items unpackStarsForTwoStarAlignmentSecondStar() {
		return unpackStarsForTwoStarAlignmentSecondStar_(coords::STARS, std::make_index_sequence<coords::STARS.size()>{});
	}

	U8G2& u8g2_;
	Mount& mount_;

	ItemsList mountType_{u8g2_, "Mount type", {}, {
			{"EQ", [this]() {
				mount_.mountType_ = Mount::MountType::EQ;
				operationMode_.title_ = "EQ Operation mode";
				currentScreen_ = &operationMode_;
			}},
			{"AZ", [this]() {
				mount_.mountType_ = Mount::MountType::AZ;
				operationMode_.title_ = "AZ Operation mode";
				currentScreen_ = &operationMode_;
			}}
		}
	};

	ItemsList operationMode_{u8g2_, "Operation mode", {}, {
			// Full operation mode features:
			// - goto rightascension and declination coords
			// - planets and moon tracking
			// - manual control
			// Requires:
			// - date, time, location
			// Alignment:
			// - 2 star
			// - 3 star
			{"Full", [this]() {}},
			// Easy track operation mode features:
			// - compensate earth rotation
			// - switch on/off
			// - manual control
			// Requires:
			// - nothing but alignment
			// Alignment:
			// - pole
			// - 2 star
			{"Easy track", [this]() { currentScreen_ = &easyTrackAlignment_; }}
		}
	};

	ItemsList easyTrackAlignment_{u8g2_, "Easy track alignment", {}, {
			{"pole", [this]() {
				if (mount_.mountType_ == Mount::MountType::EQ) {
					currentScreen_ = &onlyPoleAlignmentEQ_;
				} else {
					currentScreen_ = &poleAlignment_;
				}
			}},
			{"2 star", [this]() {
				if (mount_.mountType_ == Mount::MountType::EQ) {
					currentScreen_ = &poleAlignmentThenTwoStarEQ_;
				} else {
					currentScreen_ = &twoStarAlignmentFirstStar_;
				}
			}}
		}, [this]() { currentScreen_ = &operationMode_; }
	};

	ItemsList onlyPoleAlignmentEQ_{u8g2_, "EQ - Pole alignment", {"Move to north pole", "and press OK"}, {
			{"OK", [this]() {
				currentScreen_ = &alignmentFinished_;
				mount_.setAutoTrackPivot();
				mount_.operationMode_ = Mount::EASY_TRACK;
			}},
			{"Test", [this]() {
				mount_.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO;
				mount_.safeMoveToPositionDeg({-180, 90}, 400);
			}},
			{"Home", [this]() {
				mount_.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO;
				mount_.safeMoveTo({Mount::X_AXIS_HOME, Mount::Y_AXIS_HOME}, 600);
			}},
			{"Cancel", [this]() { currentScreen_ = &easyTrackAlignment_; }}
		}, [this]() { currentScreen_ = &easyTrackAlignment_; }
	};

	ItemsList poleAlignmentThenTwoStarEQ_{u8g2_, "EQ 2S- Pole alignment", {"Move to north pole", "and press OK"}, {
			{"OK", [this]() {
				currentScreen_ = &twoStarAlignmentFirstStar_;
				mount_.setAutoTrackPivot();
				mount_.operationMode_ = Mount::EASY_TRACK;
			}},
			{"Test", [this]() {
				mount_.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO;
				mount_.safeMoveToPositionDeg({-180, 90}, 400);
			}},
			{"Home", [this]() {
				mount_.trackingMode_ = scope::Mount::TrackingMode::MOVE_TO;
				mount_.safeMoveTo({Mount::X_AXIS_HOME, Mount::Y_AXIS_HOME}, 600);
			}},
			{"Cancel", [this]() { currentScreen_ = &easyTrackAlignment_; }}
		}, [this]() { currentScreen_ = &easyTrackAlignment_; }
	};

	ItemsList poleAlignment_{u8g2_, "AZ - Pole alignment", {"Move to north pole", "and press OK"}, {
			{"OK", [this]() {
				alignmentFinished_.title_ = poleAlignment_.title_;
				currentScreen_ = &alignmentFinished_;
				mount_.setAutoTrackPivot();
				mount_.operationMode_ = Mount::EASY_TRACK;
			}},
			{"Cancel", [this]() { currentScreen_ = &easyTrackAlignment_; }}
		}, [this]() { currentScreen_ = &easyTrackAlignment_; }
	};

	const coords::Star* selectedStar_ = &coords::STARS[static_cast<unsigned int>(coords::StarKey::Altair)];

	ItemsList twoStarAlignmentFirstStar_{u8g2_, "2S alignment 1/2", {"Choose first star:"}, {
			unpackStarsForTwoStarAlignmentFirstStar()
		}, [this]() { currentScreen_ = &easyTrackAlignment_; }
	};

	ItemsList twoStarAlignmentFirstStarConfirm_{u8g2_, "2S alignment 1/2", {}, {
			{"OK", [this]() {
					mount_.setTwoStarAlignmentFirstStar({selectedStar_->ra_.rad(), selectedStar_->dec_.rad()});
					currentScreen_ = &twoStarAlignmentSecondStar_;
			}},
			{"Cancel", [this]() { currentScreen_ = &twoStarAlignmentFirstStar_; }}
		}, [this]() { currentScreen_ = &twoStarAlignmentFirstStar_; }
	};

	ItemsList twoStarAlignmentSecondStar_{u8g2_, "2S alignment 2/2", {"Choose second star:"}, {
			unpackStarsForTwoStarAlignmentSecondStar()
		}, [this]() { currentScreen_ = &easyTrackAlignment_; }
	};

	ItemsList twoStarAlignmentSecondStarConfirm_{u8g2_, "2S alignment 2/2", {}, {
			{"OK", [this]() {
					mount_.setTwoStarAlignmentSecondStar({selectedStar_->ra_.rad(), selectedStar_->dec_.rad()});
					mount_.operationMode_ = Mount::EASY_TRACK_GOTO;
					currentScreen_ = &alignmentFinished_;
			}},
			{"Cancel", [this]() { currentScreen_ = &twoStarAlignmentSecondStar_; }}
		}, [this]() { currentScreen_ = &twoStarAlignmentSecondStar_; }
	};

	ItemsList alignmentFinished_{u8g2_, "", {"Alignment finished!"}, {
			{"OK", [this]() {
				currentScreen_ = &dashboard_;
			}}
		}
	};

	Dashboard dashboard_{u8g2_, mount_};

	ScreenItem* currentScreen_ = &mountType_;
};

}