/**
*	@file	ptreeWrapper.h
*/

#pragma once

#include <fstream>
#include <sstream>
#include <codecvt>
#include <filesystem>
#include <boost\property_tree\ptree.hpp>
#include <boost\property_tree\xml_parser.hpp>
#include <boost\property_tree\ini_parser.hpp>
#include "CommonUtility.h"

class ptreeWrapper
{
public:
	using wptree = boost::property_tree::wptree;

	static wptree LoadIniPtree(const std::string& fileName)
	{
		wptree pt;

		fs::path filterPath = GetExeDirectory() / fileName;
		std::wifstream	fs(filterPath);
		if (!fs) {
			return pt;
		}
		fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

		try {
			boost::property_tree::read_ini(fs, pt);
		}
		catch (...) {
		}
		return pt;
	}

	static bool SaveIniPtree(const std::string& fileName, const wptree& pt)
	{
		fs::path filterPath = GetExeDirectory() / fileName;
		std::wofstream	fs(filterPath);
		if (!fs)
			return false;
		fs.imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));

		try {
			boost::property_tree::write_ini(fs, pt);
		}
		catch (...) {
			return false;
		}
		return true;
	}


	static wptree BuildPtreeFromText(const std::wstring& text)
	{
		std::wstringstream ss(text);
		wptree pt;
		try {
			boost::property_tree::read_xml(ss, pt);
		}
		catch (...) {
			
		}
		return pt;
	}

};



























