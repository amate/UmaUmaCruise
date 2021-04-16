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
	std::ifstream fs((GetExeDirectory() / "setting.json").string());
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

    return true;
}

void Config::SaveConfig()
{
	std::ifstream ifs((GetExeDirectory() / "setting.json").string());
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

	std::ofstream ofs((GetExeDirectory() / "setting.json").string());
	ofs << jsonSetting;
}
