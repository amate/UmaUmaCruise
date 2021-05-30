#pragma once

#include <vector>
#include <boost\optional.hpp>

class SkillLibrary
{
public:
	bool	LoadSkillLibrary();

	boost::optional<std::wstring> SearchSkillEffect(const std::wstring& skillName);

private:
	struct Skill {
		std::wstring name;
		std::wstring effect;
		std::wstring rare;

		Skill(const std::wstring& name, const std::wstring& effect, const std::wstring& rare) :
			name(name), effect(effect), rare(rare) 
		{
		}
	};

	std::vector<Skill>	m_skillList;

};

