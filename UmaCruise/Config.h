#pragma once

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

	bool	LoadConfig();
	void	SaveConfig();
};

