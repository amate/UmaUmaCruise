#pragma once

#include <string>
#include <memory>
#include <vector>
#include <chrono>

#include "simstring\simstring.h"

class RaceDateLibrary
{
public:
	struct Race {
		enum Grade {
			kG1 = 1 << 0,
			kG2 = 1 << 1,
			kG3 = 1 << 2,
		};
		Grade			grade;				// G1・G2・G3
		std::wstring	name;				// レース名
		enum GroundCondition {
			kGrass = 1 << 3,
			kDart  = 1 << 4,
		};
		GroundCondition	groundCondition;	// 芝・ダート
		enum DistanceClass {
			kSprint = 1 << 5,
			kMile	= 1 << 6,
			kMiddle	= 1 << 7,
			kLong	= 1 << 8,
		};
		DistanceClass	distanceClass;		// 短距離・マイル・中距離・長距離
		std::wstring	distance;			// 上の実際の距離数
		enum Rotation {
			kRight	= 1 << 9, 
			kLeft	= 1 << 10,
			kLine	= 1 << 11,
		};
		Rotation		rotation;			// 右・左回り・直線
		std::wstring	location;			// 場所
		enum Location {
			kSapporo	= 1 << 12,
			kHakodate	= 1 << 13,
			kHukusima	= 1 << 14,
			kNiigata	= 1 << 15,
			kTokyo		= 1 << 16,
			kNakayama	= 1 << 17,
			kTyuukyou	= 1 << 18,
			kKyoto		= 1 << 19,
			kHanshin	= 1 << 20,
			kOgura		= 1 << 21,
			kOoi		= 1 << 22,
			kKawasaki	= 1 << 23,
			kFunabasi	= 1 << 24,
			kMorioka	= 1 << 25,

			kMaxLocationCount = 14,
		};
		Location		locationFlag;
		std::vector<std::wstring>	date;	// 開催日

		// =============================
		std::wstring	RaceName() const;
		std::wstring	GroundConditionText() const;
		std::wstring	DistanceText() const;
		std::wstring	RotationText() const;

		// 同種類はOR検索、別種類はAND検索
		bool	IsMatchState(int32_t state);
	};

	// RaceDataLibrary.json を読み込む
	bool	LoadRaceDataLibrary();

	// 全ターンリスト
	const std::vector<std::wstring>& GetAllTurnList() const {
		return m_allTurnList;
	}
	// ターン順の全レースリスト
	const std::vector<std::vector<std::shared_ptr<Race>>>& GetTurnOrderedRaceList() const {
		return m_turnOrderedRaceList;
	}

	// 
	int		GetTurnNumberFromTurnName(const std::wstring& searchTurn);


	// あいまい検索で現在の日付を変更する
	std::wstring	AnbigiousChangeCurrentTurn(std::vector<std::wstring> ambiguousCurrentTurn);

private:
	void	_InitDB();

	std::wstring	m_currentTurn;
	int		m_searchCount = 0;	// 逆行・遡行防止のため

	std::vector<std::wstring> m_allTurnList;

	std::unique_ptr<simstring::reader>	m_dbReader;

	std::vector<std::vector<std::shared_ptr<Race>>>	m_turnOrderedRaceList;
};

