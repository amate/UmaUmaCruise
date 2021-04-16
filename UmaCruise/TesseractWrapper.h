#pragma once

#include <opencv2\opencv.hpp>


namespace TesseractWrapper {

bool	TesseractInit();
void	TesseractTerm();

std::wstring TextFromImage(cv::Mat targetImage);

}	// namespace TesseractWrapper

