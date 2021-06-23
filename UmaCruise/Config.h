#pragma once
#include "boost/filesystem.hpp"

struct Config
{
	int		refreshInterval = 1;
	bool	autoStart = false;
	bool	stopUpdatePreviewOnTraining = false;
	bool	popupRaceListWindow = false;
	bool	notifyFavoriteRaceHold = true;
	enum Theme {
		kAuto, kDark, kLight,
	};
	Theme	theme = kAuto;
	bool	windowTopMost = false;
	boost::filesystem::path screenShotFolder;

	bool	LoadConfig();
	void	SaveConfig();
};

