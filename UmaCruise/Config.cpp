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

    return true;
}

void Config::SaveConfig()
{
	std::ifstream ifs((GetExeDirectory() / "setting.json").wstring());
	if (!ifs) {
		ATLASSERT(FALSE);
		ERROR_LOG << L"SaveConfig failed: !fs";
		return ;
	}
	json jsonSetting;
	ifs >> jsonSetting;
	ifs.close();

	jsonSetting["Config"]["RefreshInterval"] = refreshInterval;
	jsonSetting["Config"]["AutoStart"] = autoStart;
	jsonSetting["Config"]["StopUpdatePreviewOnTraining"] = stopUpdatePreviewOnTraining;

	std::ofstream ofs((GetExeDirectory() / "setting.json").wstring());
	ofs << jsonSetting.dump(4);
}
