#include "stdafx.h"
#include "TesseractWrapper.h"

#include <unordered_map>
#include <mutex>

#include <boost\algorithm\string\trim.hpp>
#include <boost\algorithm\string\replace.hpp>

#include <tesseract\baseapi.h>
#include <leptonica\allheaders.h>

#include "Utility\Logger.h"
#include "Utility\CodeConvert.h"
#include "Utility\timer.h"

using namespace CodeConvert;

namespace TesseractWrapper {

	std::unordered_map<DWORD, std::unique_ptr<tesseract::TessBaseAPI>> s_threadTess;
	std::unordered_map<DWORD, std::unique_ptr<tesseract::TessBaseAPI>> s_threadTessBest;
	std::mutex	s_mtx;


	bool TesseractInit()
	{
		return true;
	}

	void TesseractTerm()
	{
		std::unique_lock<std::mutex> lock(s_mtx);
		s_threadTess.clear();
		s_threadTessBest.clear();
	}

	std::wstring TextFromImage(cv::Mat targetImage)
	{
		INFO_LOG << L"TextFromImage start";
		Utility::timer timer;

		std::unique_lock<std::mutex> lock(s_mtx);
		auto& ptess = s_threadTess[::GetCurrentThreadId()];
		if (!ptess) {
			INFO_LOG << L"new tesseract::TessBaseAPI";
			ptess.reset(new tesseract::TessBaseAPI);
			auto dbFolderPath = GetExeDirectory() / L"tessdata";
			if (ptess->Init(dbFolderPath.string().c_str(), "jpn")) {
				ERROR_LOG << L"Could not initialize tesseract.";
				ATLASSERT(FALSE);
				return L"";
			}
			INFO_LOG << L"ptess->Init success!";
			ptess->SetPageSegMode(tesseract::/*PSM_SINGLE_BLOCK*/PSM_SINGLE_LINE);
		}
		lock.unlock();

		ptess->SetImage((uchar*)targetImage.data, targetImage.size().width, targetImage.size().height, targetImage.channels(), targetImage.step);

		ptess->Recognize(0);
		std::wstring text = UTF16fromUTF8(ptess->GetUTF8Text()).c_str();

		// whilte space ÇéÊÇËèúÇ≠
		boost::algorithm::trim(text);
		boost::algorithm::replace_all(text, L" ", L"");
		boost::algorithm::replace_all(text, L"\n", L"");

		INFO_LOG << L"TextFromImage result: [" << text << L"] processing time: " << UTF16fromUTF8(timer.format());
		return text;
	}

	std::wstring TextFromImageBest(cv::Mat targetImage)
	{
		INFO_LOG << L"TextFromImageBest start";
		Utility::timer timer;

		std::unique_lock<std::mutex> lock(s_mtx);
		auto& ptess = s_threadTessBest[::GetCurrentThreadId()];
		if (!ptess) {
			INFO_LOG << L"new tesseract::TessBaseAPI";
			ptess.reset(new tesseract::TessBaseAPI);
			auto dbFolderPath = GetExeDirectory() / L"tessdata" / L"best";
			if (ptess->Init(dbFolderPath.string().c_str(), "jpn")) {
				ERROR_LOG << L"Could not initialize tesseract.";
				ATLASSERT(FALSE);
				return L"";
			}
			INFO_LOG << L"ptess->Init success!";
			ptess->SetPageSegMode(tesseract::/*PSM_SINGLE_BLOCK*/PSM_SINGLE_LINE);
		}
		lock.unlock();

		ptess->SetImage((uchar*)targetImage.data, targetImage.size().width, targetImage.size().height, targetImage.channels(), targetImage.step);

		ptess->Recognize(0);
		std::wstring text = UTF16fromUTF8(ptess->GetUTF8Text()).c_str();

		// whilte space ÇéÊÇËèúÇ≠
		boost::algorithm::trim(text);
		boost::algorithm::replace_all(text, L" ", L"");
		boost::algorithm::replace_all(text, L"\n", L"");

		INFO_LOG << L"TextFromImageBest result: [" << text << L"] processing time: " << UTF16fromUTF8(timer.format());
		return text;
	}

}	// namespace TesseractWrapper 
