#pragma once

struct Config
{
	int		refreshInterval = 1;
	bool	autoStart = false;

	bool	LoadConfig();
	void	SaveConfig();
};

