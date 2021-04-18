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
	std::mutex	s_mtx;


	bool TesseractInit()
	{
		return true;
	}

	void TesseractTerm()
	{
		std::unique_lock<std::mutex> lock(s_mtx);
		s_threadTess.clear();
	}

	std::wstring TextFromImage(cv::Mat targetImage)
	{
		Utility::timer timer;

		std::unique_lock<std::mutex> lock(s_mtx);
		auto& ptess = s_threadTess[::GetCurrentThreadId()];
		if (!ptess) {
			ptess.reset(new tesseract::TessBaseAPI);
			if (ptess->Init(NULL, "jpn")) {
				ERROR_LOG << L"Could not initialize tesseract.";
				return L"";
			}
			ptess->SetPageSegMode(tesseract::/*PSM_SINGLE_BLOCK*/PSM_SINGLE_LINE);
		}
		lock.unlock();

		ptess->SetImage((uchar*)targetImage.data, targetImage.size().width, targetImage.size().height, targetImage.channels(), targetImage.step1());
		ptess->Recognize(0);
		std::wstring text = UTF16fromUTF8(ptess->GetUTF8Text()).c_str();

		// whilte space ÇéÊÇËèúÇ≠
		boost::algorithm::trim(text);
		boost::algorithm::replace_all(text, L" ", L"");
		boost::algorithm::replace_all(text, L"\n", L"");

		ATLTRACE(L"TextFromImage %s\n", UTF16fromUTF8(timer.format()).c_str());
		return text;
	}

}	// namespace TesseractWrapper 
