#include "stdafx.h"
#include "UmaEventLibrary.h"

#include <regex>
#include <boost\algorithm\string\trim.hpp>
#include <boost\algorithm\string\replace.hpp>
#include <boost\filesystem.hpp>
#include <boost\optional.hpp>

#include "Utility\CodeConvert.h"
#include "Utility\CommonUtility.h"
#include "Utility\json.hpp"
#include "Utility\Logger.h"


using json = nlohmann::json;
using namespace CodeConvert;


boost::optional<std::pair<std::wstring, double>> retrieve(
	simstring::reader& dbr,
	const std::vector<std::wstring>& ambiguousEventNames,
	int measure,
	double threshold, double minThreshold
)
{
	// Retrieve similar strings into a string vector.
	std::vector<std::wstring> xstrs;
	std::wstring query_org;
	for (; threshold >= minThreshold/*kMinThreshold*/; threshold -= 0.05) {	// 少なくとも一つが見つかるような閾値を探す
		for (const std::wstring& query : ambiguousEventNames) {
			dbr.retrieve(query, measure, threshold, std::back_inserter(xstrs));
			if (xstrs.size()) {
				query_org = query;
				break;
			}
		}
		if (xstrs.size()) {
			break;
		}
	}
	if (xstrs.size() >= 2) {
		INFO_LOG << L"retrieve - 候補が複数あります [" << query_org << L"]";
		// 類似度が高い方を調べる
		simstring::ngram_generator ngen(1, false);
		std::vector<std::wstring> query_ngrams;
		ngen(query_org, std::back_inserter(query_ngrams));
		ATLASSERT(query_ngrams.size());
		if (query_ngrams.empty()) {
			return boost::none;
		}
		std::wstring topSimilrartyText;
		double topSimilrartyRatio = 0.0;
		for (const std::wstring& str : xstrs) {
			std::vector<std::wstring> str_ngrams;
			ngen(str, std::back_inserter(str_ngrams));
			ATLASSERT(str_ngrams.size());
			int matchCount = 0;
			for (const std::wstring& gram : str_ngrams) {
				bool found = std::find(query_ngrams.begin(), query_ngrams.end(), gram) != query_ngrams.end();
				if (found) {
					++matchCount;
				}
			}
			const int maxSize = static_cast<int>(std::max(query_ngrams.size(), str_ngrams.size()));
			const double simRatio = static_cast<double>(matchCount) / maxSize;
			INFO_LOG << L"[" << str << L"] - " << simRatio;
			if (topSimilrartyRatio < simRatio) {
				topSimilrartyRatio = simRatio;
				topSimilrartyText = str;	// 前のより類似度が高い
			}
		}
		INFO_LOG << L"topSimilrartyText -> [" << topSimilrartyText << L"]";
		return std::make_pair(topSimilrartyText, topSimilrartyRatio);
	}

	if (xstrs.size()) {
		INFO_LOG << L"result: " << xstrs.front() << L" threshold: " << threshold;
		return std::make_pair(xstrs.front(), threshold);
	} else {
		return boost::none;
	}
}

boost::optional<std::wstring> retrieve(
	simstring::reader& dbr,
	const std::vector<std::wstring>& ambiguousEventNames,
	int measure,
	double threshold
)
{
	const double kMinThreshold = 0.4;
	auto optResult = retrieve(dbr, ambiguousEventNames, measure, threshold, kMinThreshold);
	if (optResult) {
		return optResult->first;
	} else {
		return boost::none;
	}
}

// ==============================================================

bool UmaEventLibrary::LoadUmaMusumeLibrary()
{
	m_charaEventList.clear();
	m_supportEventList.clear();
	try {
		auto funcAddjsonEventToUmaEvent = [](const json& jsonEventList, CharaEvent& charaEvent) {
			for (const json& jsonEvent : jsonEventList) {
				auto eventElm = *jsonEvent.items().begin();
				std::wstring eventName = UTF16fromUTF8(eventElm.key());

				charaEvent.umaEventList.emplace_back();
				UmaEvent& umaEvent = charaEvent.umaEventList.back();
				umaEvent.parentCharaEvent = &charaEvent;
				umaEvent.eventName = eventName;

				int i = 0;
				for (const json& jsonOption : eventElm.value()) {
					std::wstring option = UTF16fromUTF8(jsonOption["Option"]);
					std::wstring effect = UTF16fromUTF8(jsonOption["Effect"]);
					boost::algorithm::replace_all(effect, L"\n", L"\r\n");

					if (kMaxOption <= i) {
						ATLASSERT(FALSE);
						ERROR_LOG << L"選択肢の数が kMaxOption を超えます: " << eventName;
						break;
						//throw std::runtime_error("選択肢の数が kMaxOption を超えます");
					}

					umaEvent.eventOptions[i].option = option;
					umaEvent.eventOptions[i].effect = effect;
					++i;
				}
			}
		};
		auto funcLoad = [=](const json& jsonLibrary, const std::string& keyName, CharaEventList& charaEventList) {
			for (const auto& propElm : jsonLibrary[keyName].items()) {
				std::wstring prop = UTF16fromUTF8(propElm.key());	// hosi or rare

				for (const auto& umaElm : propElm.value().items()) {
					std::wstring umaName = UTF16fromUTF8(umaElm.key());

					charaEventList.emplace_back(std::make_unique<CharaEvent>());
					CharaEvent& charaEvent = *charaEventList.back();
					charaEvent.name = umaName;
					charaEvent.property = prop;

					funcAddjsonEventToUmaEvent(umaElm.value()["Event"], charaEvent);
				}
			}
		};

		{	// UmaMusumeLibrary.json
			std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / "UmaMusumeLibrary.json").wstring());
			ATLASSERT(ifs);
			if (!ifs) {
				throw std::runtime_error("UmaMusumeLibrary.json の読み込みに失敗");
			}
			json jsonLibrary;
			ifs >> jsonLibrary;

			funcLoad(jsonLibrary, "Charactor", m_charaEventList);
			funcLoad(jsonLibrary, "Support", m_supportEventList);
			// R->SR->SSRと読み込まれるので SSR->SR->R順に逆転しておく
			std::reverse(m_supportEventList.begin(), m_supportEventList.end());			
		}
		{	// UmaMusumeLibraryMainStory.json
			std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / "UmaMusumeLibraryMainStory.json").wstring());
			ATLASSERT(ifs);
			if (!ifs) {
				throw std::runtime_error("UmaMusumeLibraryMainStory.json の読み込みに失敗");
			}
			json jsonLibrary;
			ifs >> jsonLibrary;

			funcLoad(jsonLibrary, "MainStory", m_supportEventList);
		}
		{	// UmaMusumeLibraryRevision.json
			std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / "UmaMusumeLibraryRevision.json").wstring());
			if (ifs) {
				json jsonRevisionLibrary;
				ifs >> jsonRevisionLibrary;
				for (const auto& keyVal : jsonRevisionLibrary.items()) {
					std::wstring sourceName = UTF16fromUTF8(keyVal.key());
					CharaEvent charaEvent;
					charaEvent.name = sourceName;
					funcAddjsonEventToUmaEvent(keyVal.value()["Event"], charaEvent);

					auto funcUpdateEventOptions = [](const CharaEvent& charaEvent, CharaEventList& charaEventList) -> bool {
						for (auto& anotherCharaEventList : charaEventList) {
							if (anotherCharaEventList->name == charaEvent.name) {	// ソース一致
								bool update = false;
								for (auto& anotherUmaEventList : anotherCharaEventList->umaEventList) {
									for (const auto& umaEventList : charaEvent.umaEventList) {
										if (anotherUmaEventList.eventName == umaEventList.eventName) {	// イベント名一致
											anotherUmaEventList.eventOptions = umaEventList.eventOptions;	// 選択肢を上書き
											update = true;
										}
									}
								}
								ATLASSERT(update);
								return true;
							}
						}
						return false;
					};
					// chara/supportEventList へ上書きする
					if (!funcUpdateEventOptions(charaEvent, m_charaEventList)) {
						if (!funcUpdateEventOptions(charaEvent, m_supportEventList)) {
							// イベントリストにイベント名がなかったので、m_supportEventListへ追加しておく
							m_supportEventList.emplace_back(std::make_unique<CharaEvent>(charaEvent));
						}
					}
				}
			}
		}
		{
			std::ifstream ifs((GetExeDirectory() / L"UmaLibrary" / L"Common.json").wstring());
			ATLASSERT(ifs);
			if (!ifs) {
				throw std::runtime_error("Common.json の読み込みに失敗");
			}
			json jsonCommon;
			ifs >> jsonCommon;
			m_kMinThreshold = jsonCommon["Common"]["Simstring"].value<double>("MinThreshold", m_kMinThreshold);
			m_kUmaMusumeNameMinThreshold = 
				jsonCommon["Common"]["Simstring"].value<double>("UmaMusumeNameMinThreshold", m_kUmaMusumeNameMinThreshold);			
		}

		_DBUmaNameInit();

		// もう一度呼ばれたときのために m_currentIkuseiUmaEvent を初期化しておく
		m_currentIkuseiUmaEvent = nullptr;
		std::wstring ikuseiUmaName = m_currentIkuseUmaMusume;
		m_currentIkuseUmaMusume.clear();
		ChangeIkuseiUmaMusume(ikuseiUmaName);
		return true;
	}
	catch (std::exception& e) {
		ERROR_LOG << L"LoadUmaMusumeLibrary failed: " << (LPCWSTR)CA2W(e.what());
		ATLASSERT(FALSE);
	}
	return false;
}

void UmaEventLibrary::ChangeIkuseiUmaMusume(const std::wstring& umaName)
{
	std::unique_lock<std::mutex> lock(m_mtxName);	// 一応念のため…
	if (m_currentIkuseUmaMusume != umaName) {
		INFO_LOG << L"ChangeIkuseiUmaMusume: " << umaName;
		m_currentIkuseUmaMusume = umaName;

		m_currentIkuseiUmaEvent = nullptr;
		if (m_currentIkuseUmaMusume.length()) {
			for (const auto& charaEvent : m_charaEventList) {
				if (charaEvent->name == m_currentIkuseUmaMusume) {
					m_currentIkuseiUmaEvent = charaEvent.get();
					break;
				}
			}
			ATLASSERT(m_currentIkuseiUmaEvent);
			if (!m_currentIkuseiUmaEvent) {
				ERROR_LOG << L"m_charaEventList から育成ウマ娘が見つかりません: " << umaName;
			}
		}
		if (m_funcNotifyChangeIkuseiUmaMusume) {
			m_funcNotifyChangeIkuseiUmaMusume(umaName);
		}
		m_simstringDBInit = false;
	}
}

void UmaEventLibrary::AnbigiousChangeIkuseImaMusume(std::vector<std::wstring> ambiguousUmaMusumeNames)
{
	// whilte space を取り除く
	for (auto& name : ambiguousUmaMusumeNames) {
		boost::algorithm::trim(name);
		boost::algorithm::replace_all(name, L"[", L"【");
		boost::algorithm::replace_all(name, L"]",  L"】");
	}

	// Output similar strings from Unicode queries.
	auto optResult = retrieve(*m_dbUmaNameReader, ambiguousUmaMusumeNames, simstring::cosine, 0.6, m_kUmaMusumeNameMinThreshold);
	if (optResult) {
		ChangeIkuseiUmaMusume(optResult->first);
	}
}


boost::optional<UmaEventLibrary::UmaEvent> UmaEventLibrary::AmbiguousSearchEvent(
	const std::vector<std::wstring>& ambiguousEventNames,
	const std::vector<std::wstring>& ambiguousEventBottomOptions )
{
	_DBInit();

	auto optOptionResult = retrieve(*m_dbOptionReader, ambiguousEventBottomOptions, simstring::cosine, 1.0, m_kMinThreshold);
	auto optResult = retrieve(*m_dbReader, ambiguousEventNames, simstring::cosine, 1.0, m_kMinThreshold);

	UmaEvent event1 = optResult ? _SearchEventOptions(optResult->first) : UmaEvent();
	UmaEvent event2 = optOptionResult ? _SearchEventOptionsFromBottomOption(optOptionResult->first) : UmaEvent();
	if (event1.eventName.length() && event2.eventName.length() &&
		event1.eventName != event2.eventName) 
	{
		WARN_LOG << L"AmbiguousSearchEvent イベント名不一致\n"
			<< L"・イベント名から 1: [" << event1.eventName << L"] (" 
				<< ambiguousEventNames.front() << L") - " << optResult->second << L"\n"
			<< L"・下部選択肢から 2: [" << event2.eventName << L"] (" 
				<< ambiguousEventBottomOptions.front() << L") - " << optOptionResult->second;
	}

	if (optOptionResult) {	// 選択肢からの検索を優先する
		if (!optResult || optResult->second < optOptionResult->second) {	// 類似度を比較する
			//INFO_LOG << L"AmbiguousSearchEvent result: " << event2.eventName;
			return event2;
		}
	}
	if (optResult) {
		//INFO_LOG << L"AmbiguousSearchEvent result: " << event1.eventName;
		return event1;

	} else {
		//INFO_LOG << L"AmbiguousSearchEvent: not found";
		return boost::none;
	}

#if 0
	auto optOptionResult = retrieve(*m_dbOptionReader, ambiguousEventBottomOptions, simstring::cosine, 1.0, m_kMinThreshold);
	if (optOptionResult) {	// 選択肢からの検索を優先する
		return _SearchEventOptionsFromBottomOption(optOptionResult.get());
	}
	auto optResult = retrieve(*m_dbReader, ambiguousEventNames, simstring::cosine, 1.0, m_kMinThreshold);
	if (optResult) {
		return _SearchEventOptions(optResult.get());

	} else {
		return boost::none;
	}
#endif
}

void UmaEventLibrary::_DBUmaNameInit()
{
	m_dbUmaNameReader = std::make_unique<simstring::reader>();
	auto dbFolder = GetExeDirectory() / L"simstringDB" / L"UmaName";
	auto dbPath = dbFolder / L"umaName_unicode.db";

	// DBフォルダ内を消して初期化
	if (boost::filesystem::is_directory(dbFolder)) {
		boost::system::error_code ec = {};
		boost::filesystem::remove_all(dbFolder, ec);
		if (ec) {
			ERROR_LOG << L"boost::filesystem::remove_all(dbFolder failed: " << (LPCWSTR)CA2W(ec.message().c_str());
		}
	}
	boost::filesystem::create_directories(dbFolder);

	// Open a SimString database for writing (with std::wstring).
	simstring::ngram_generator gen;
	simstring::writer_base<std::wstring> dbw(gen, dbPath.string());

	// 育成ウマ娘の名前を追加
	for (const auto& charaEvent : m_charaEventList) {
		const std::wstring& name = charaEvent->name;
		dbw.insert(name);
	}
	dbw.close();

	// Open the database for reading.
	m_dbUmaNameReader->open(dbPath.string());
}

void UmaEventLibrary::_DBInit()
{
	if (!m_simstringDBInit) {
		auto dbFolder = GetExeDirectory() / L"simstringDB" / L"Event";

		m_dbReader = std::make_unique<simstring::reader>();
		auto dbPath = dbFolder / L"event_unicode.db";

		m_dbOptionReader = std::make_unique<simstring::reader>();
		auto dbOptionPath = dbFolder / L"eventOption_unicode.db";

		// DBフォルダ内を消して初期化
		if (boost::filesystem::is_directory(dbFolder)) {
			boost::system::error_code ec = {};
			boost::filesystem::remove_all(dbFolder, ec);
			if (ec) {
				ERROR_LOG << L"boost::filesystem::remove_all(dbFolder failed: " << (LPCWSTR)CA2W(ec.message().c_str());
			}
		}
		boost::filesystem::create_directories(dbFolder);

		// Open a SimString database for writing (with std::wstring).
		simstring::ngram_generator gen(2, false);	// bi-gram
		// イベント名DB
		simstring::writer_base<std::wstring> dbw(gen, dbPath.string());
		// イベント選択肢DB
		simstring::writer_base<std::wstring> dbwOption(gen, dbOptionPath.string());


		// 育成ウマ娘のイベントを追加
		for (const auto& charaEvent : m_charaEventList) {
			if (charaEvent->name.find(m_currentIkuseUmaMusume) == std::wstring::npos) {
				continue;
			}

			for (const auto& umaEvent : charaEvent->umaEventList) {
				dbw.insert(umaEvent.eventName);

				for (auto it = umaEvent.eventOptions.crbegin(); it != umaEvent.eventOptions.crend(); ++it) {
					if (it->option.empty()) {
						continue;
					}
					dbwOption.insert(it->option);	// 最後の選択肢を追加
					break;
				}
			}
			break;
		}

		// サポートカードのイベントを追加
		for (const auto& charaEvent : m_supportEventList) {
			for (const auto& umaEvent : charaEvent->umaEventList) {
				dbw.insert(umaEvent.eventName);

				for (auto it = umaEvent.eventOptions.crbegin(); it != umaEvent.eventOptions.crend(); ++it) {
					if (it->option.empty()) {
						continue;
					}
					dbwOption.insert(it->option);	// 最後の選択肢を追加
					break;
				}
			}
		}
		
		dbw.close();
		dbwOption.close();

		// Open the database for reading.
		m_dbReader->open(dbPath.string());
		m_dbOptionReader->open(dbOptionPath.string());

		m_simstringDBInit = true;
	}
}

UmaEventLibrary::UmaEvent UmaEventLibrary::_SearchEventOptions(const std::wstring& eventName)
{
	// 育成ウマ娘のイベントを探す
	if (m_currentIkuseiUmaEvent) {
		for (const auto& umaEvent : m_currentIkuseiUmaEvent->umaEventList) {
			if (umaEvent.eventName == eventName) {
				return umaEvent;
			}
		}
	}

	// サポートカードのイベントを探す
	for (const auto& charaEvent : m_supportEventList) {
		for (const auto& umaEvent : charaEvent->umaEventList) {
			if (umaEvent.eventName == eventName) {
				return umaEvent;
			}
		}
	}
	ATLASSERT(FALSE);
	return UmaEvent();
}

UmaEventLibrary::UmaEvent UmaEventLibrary::_SearchEventOptionsFromBottomOption(const std::wstring& bottomOption)
{
	// 育成ウマ娘のイベントを探す
	if (m_currentIkuseiUmaEvent) {
		for (const auto& umaEvent : m_currentIkuseiUmaEvent->umaEventList) {
			for (auto it = umaEvent.eventOptions.crbegin(); it != umaEvent.eventOptions.crend(); ++it) {
				if (it->option.empty()) {
					continue;
				}
				if (it->option == bottomOption) {	// 最後の選択肢を比較
					return umaEvent;
				}
				break;
			}
		}
	}

	// サポートカードのイベントを探す
	for (const auto& charaEvent : m_supportEventList) {
		for (const auto& umaEvent : charaEvent->umaEventList) {
			for (auto it = umaEvent.eventOptions.crbegin(); it != umaEvent.eventOptions.crend(); ++it) {
				if (it->option.empty()) {
					continue;
				}
				if (it->option == bottomOption) {	// 最後の選択肢を比較
					return umaEvent;
				}
				break;
			}
		}
	}
	ATLASSERT(FALSE);
	return UmaEvent();
}
