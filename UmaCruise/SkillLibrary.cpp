#include "stdafx.h"
#include "SkillLibrary.h"

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"
#include "Utility\Logger.h"


using json = nlohmann::json;
using namespace CodeConvert;

bool SkillLibrary::LoadSkillLibrary()
{
	try {
		std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / L"SkillLibrary.json").wstring());
		ATLASSERT(ifs);
		if (!ifs) {
			throw std::runtime_error("SkillLibrary.json の読み込みに失敗");
		}
		json jsonLibrary;
		ifs >> jsonLibrary;

		for (const auto& items : jsonLibrary["Skill"].items()) {
			std::string rare = items.key();
			for (const auto& jsonSkill : items.value()) {
				std::string name = jsonSkill["Name"].get<std::string>();
				std::string effect = jsonSkill["Effect"].get<std::string>();

				m_skillList.emplace_back(UTF16fromUTF8(name), UTF16fromUTF8(effect), UTF16fromUTF8(rare));
			}
		}
		return true;
	}
	catch (std::exception& e) {
		ERROR_LOG << L"LoadSkillLibrary failed: " << UTF16fromUTF8(e.what());
		ATLASSERT(FALSE);
		return false;
	}
}

boost::optional<std::wstring> SkillLibrary::SearchSkillEffect(const std::wstring& skillName)
{
	auto itfound = std::find_if(m_skillList.begin(), m_skillList.end(), [skillName](const Skill& skill) {
		return skill.name == skillName;
		});
	if (itfound != m_skillList.end()) {
		return itfound->effect;
	}
	return boost::none;
}
