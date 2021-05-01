#pragma once

struct Config
{
	int		refreshInterval = 1;
	bool	autoStart = false;
	bool	stopUpdatePreviewOnTraining = false;
	bool	popupRaceListWindow = false;
	bool	notifyFavoriteRaceHold = true;

	bool	LoadConfig();
	void	SaveConfig();
};

