#include "stdafx.h"
#include "Config.h"

#include <fstream>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"

using json = nlohmann::json;
using namespace CodeConvert;


bool Config::LoadConfig()
{
	std::ifstream fs((GetExeDirectory() / "setting.json").wstring());
	if (!fs) {
		return true;
	}
	json jsonSetting;
	fs >> jsonSetting;

	if (jsonSetting["Config"].is_null()) {
		return true;	// default
	}

	refreshInterval = jsonSetting["Config"].value("RefreshInterval", refreshInterval);
	autoStart = jsonSetting["Config"].value("AutoStart", autoStart);
	stopUpdatePreviewOnTraining = jsonSetting["Config"].value("StopUpdatePreviewOnTraining", stopUpdatePreviewOnTraining);
	popupRaceListWindow = jsonSetting["Config"].value("PopupRaceListWindow", popupRaceListWindow);
	notifyFavoriteRaceHold = jsonSetting["Config"].value("NotifyFavoriteRaceHold", notifyFavoriteRaceHold);
	theme = jsonSetting["Config"].value("Theme", theme);
	windowTopMost = jsonSetting["Config"].value("WindowTopMost", windowTopMost);

    return true;
}

void Config::SaveConfig()
{
	json jsonSetting;
	std::ifstream fs((GetExeDirectory() / "setting.json").wstring());
	if (fs) {
		fs >> jsonSetting;
	}
	fs.close();

	jsonSetting["Config"]["RefreshInterval"] = refreshInterval;
	jsonSetting["Config"]["AutoStart"] = autoStart;
	jsonSetting["Config"]["StopUpdatePreviewOnTraining"] = stopUpdatePreviewOnTraining;
	jsonSetting["Config"]["PopupRaceListWindow"] = popupRaceListWindow;
	jsonSetting["Config"]["NotifyFavoriteRaceHold"] = notifyFavoriteRaceHold;
	jsonSetting["Config"]["Theme"] = theme;
	jsonSetting["Config"]["WindowTopMost"] = windowTopMost;

	std::ofstream ofs((GetExeDirectory() / "setting.json").wstring());
	ofs << jsonSetting.dump(4);
}
