#pragma once

#include <string>
#include <array>
#include <vector>
#include <memory>
#include <mutex>
#include <boost\optional.hpp>

#include "simstring\simstring.h"


class UmaEventLibrary
{
public:
	struct EventOptionEffect {
		std::wstring	option;	// 選択肢
		std::wstring	effect;	// 効果
	};
	enum { kMaxOption = 3 };	// 1つのイベントに付き、最大3つの選択肢がある
	typedef std::array<EventOptionEffect, kMaxOption>	EventOptions;

	struct UmaEvent {
		std::wstring	eventName;		// イベント名
		EventOptions	eventOptions;
	};

	// 一人のキャラが所有する全イベントを所持する
	struct CharaEvent {
		std::wstring	name;	// キャラ名 / サポート名
		std::wstring	property;	// [☆1, ☆2, ☆3] / [SSR, SR, R]
		std::vector<UmaEvent>	umaEventList;	// キャラが所有する全イベントリスト
	};
	typedef std::vector<std::unique_ptr<CharaEvent>>	CharaEventList;

	// UmaMusumeLibrary.json を読み込む
	bool	LoadUmaMusumeLibrary();

	// 現在選択中の育成ウマ娘名を返す
	const std::wstring& GetCurrentIkuseiUmaMusume() const {
		return m_currentIkuseUmaMusume;
	}

	// 育成ウマ娘のイベントリストを返す
	const CharaEventList& GetIkuseiUmaMusumeEventList() const {
		return m_charaEventList;
	}

	// 検索対象とする育成ウマ娘を変更する
	void	ChangeIkuseiUmaMusume(const std::wstring& umaName);

	// あいまい検索で育成ウマ娘を変更する
	void	AnbigiousChangeIkuseImaMusume(std::vector<std::wstring> ambiguousUmaMusumeNames);

	// あいまい検索でイベント名を探す
	boost::optional<UmaEvent>	AmbiguousSearchEvent(
		const std::vector<std::wstring>& ambiguousEventNames,
		const std::vector<std::wstring>& ambiguousEventBottomOptions );

	// イベントがどのキャラから発生したのかを返す
	const std::wstring& GetLastEventSource() const {
		return m_lastEventSource;
	}

private:
	void		_DBUmaNameInit();
	void		_DBInit();
	UmaEvent	_SearchEventOptions(const std::wstring& eventName);
	UmaEvent	_SearchEventOptionsFromBottomOption(const std::wstring& bottomOption);

	// 現在選択中の育成ウマ娘の名前
	std::wstring	m_currentIkuseUmaMusume;
	std::mutex		m_mtxName;

	// 育成ウマ娘
	CharaEventList	m_charaEventList;
	// サポートカード
	CharaEventList	m_supportEventList;

	std::wstring	m_lastEventSource;

	bool	m_simstringDBInit = false;
	std::unique_ptr<simstring::reader>	m_dbUmaNameReader;
	std::unique_ptr<simstring::reader>	m_dbReader;
	std::unique_ptr<simstring::reader>	m_dbOptionReader;

	double	m_kMinThreshold = 0.4;
};

