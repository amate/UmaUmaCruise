/**
*
*/

#pragma once

#include <boost/log/expressions.hpp>
#include <boost/log/sources/global_logger_storage.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/log/utility/setup.hpp>

#define INFO_LOG  BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::info)
#define WARN_LOG  BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::warning)
#define ERROR_LOG BOOST_LOG_SEV(my_logger::get(), boost::log::trivial::error)

//#define SYS_LOGFILE             "info.log"
std::string	LogFileName();

//Narrow-char thread-safe logger.
typedef boost::log::sources::wseverity_logger_mt<boost::log::trivial::severity_level> logger_t;

//declares a global logger with a custom initialization
BOOST_LOG_GLOBAL_LOGGER(my_logger, logger_t)























