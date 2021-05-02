/**
*
*/

#include "stdafx.h"
#include "Logger.h"
#include <codecvt>
#include <locale>
#include <io.h>
#include <fcntl.h>

namespace attrs = boost::log::attributes;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace keywords = boost::log::keywords;
namespace logging = boost::log;

//Defines a global logger initialization routine
BOOST_LOG_GLOBAL_LOGGER_INIT(my_logger, logger_t)
{
	logger_t lg;

	logging::add_common_attributes();

#if 0
	auto flsink = logging::add_file_log(
		boost::log::keywords::file_name = SYS_LOGFILE,
		boost::log::keywords::format = (
		expr::stream << expr::format_date_time<     boost::posix_time::ptime >("TimeStamp", "%Y-%m-%d %H:%M:%S")
		<< " [" << expr::attr<     boost::log::trivial::severity_level >("Severity") << "]: "
		<< expr::wmessage
		)
		);

	flsink->imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
#endif

	boost::shared_ptr<logging::core> core = logging::core::get();

#if 0
	boost::shared_ptr<sinks::text_ostream_backend> ostream_backend =
		boost::make_shared<sinks::text_ostream_backend>();
	ostream_backend->add_stream(
		boost::shared_ptr<std::ostream>(&std::clog, logging::empty_deleter()));
	ostream_backend->auto_flush(true);

	typedef sinks::synchronous_sink<sinks::text_ostream_backend> sink_ostream_t;
	boost::shared_ptr<sink_ostream_t> sink_ostream(new sink_ostream_t(ostream_backend));
	sink_ostream->set_formatter(
		expr::format("[%1%] [%2%]\t%3%")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S.%f")
		% logging::trivial::severity
		% expr::message);

	core->add_sink(sink_ostream);
#endif

	// ログのファイル出力を設定
	auto text_backend = boost::make_shared<sinks::text_file_backend>(
			//boost::log::keywords::file_name = SYS_LOGFILE
			boost::log::keywords::file_name = LogFileName()
		);
	text_backend->auto_flush(true);

	typedef sinks::synchronous_sink<sinks::text_file_backend> sink_text_t;
	boost::shared_ptr<sink_text_t> sink_text(new sink_text_t(text_backend));

	sink_text->set_formatter(
		expr::format("%1% [%2%]: %3%")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
		% logging::trivial::severity
		% expr::wmessage);
	sink_text->imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
	core->add_sink(sink_text);

#ifdef _DEBUG
	// ログのコンソール出力を設定
	//setlocale(LC_ALL, "");

	::AllocConsole();
	//::SetConsoleOutputCP(CP_UTF8);
	FILE* out = nullptr; 
	freopen_s(&out, "CON", "w", stdout);

	std::wcout.imbue(std::locale(""));

	auto console_sink = logging::add_console_log(std::wcout);
	console_sink->set_formatter(
		expr::format(L"%1% [%2%]: %3%")
		% expr::format_date_time<boost::posix_time::ptime>("TimeStamp", "%Y-%m-%d %H:%M:%S")
		% logging::trivial::severity
		% expr::wmessage);
	console_sink->imbue(std::locale(std::locale(), new std::codecvt_utf8_utf16<wchar_t>));
#endif

	logging::core::get()->set_filter
		(
#ifdef _DEBUG
		logging::trivial::severity >= logging::trivial::info
#else
		logging::trivial::severity >= /*logging::trivial::info*/ logging::trivial::warning
#endif
		);

	return lg;
}














