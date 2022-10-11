/**
 * @file logger.cpp
 * @brief Defining the Logger class, which wraps the Boost Trivial Logging.
 *
 * @author Shuhao Liu (shuhao@ece.toronto.edu)
 * @date November 2, 2015
 *
 * @ingroup basics
 *
 * @todo Add the colored display to console logs.
 */

#include "util/logger.hpp"

#include <boost/log/core/core.hpp>
#include <boost/log/expressions/formatters/date_time.hpp>
#include <boost/log/expressions.hpp>
#include <boost/log/sinks/sync_frontend.hpp>
#include <boost/log/sinks/text_ostream_backend.hpp>
#include <boost/log/sources/severity_logger.hpp>
#include <boost/log/support/date_time.hpp>
#include <boost/log/trivial.hpp>
#include <boost/core/null_deleter.hpp>
#include <boost/log/utility/setup/common_attributes.hpp>
#include <boost/make_shared.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>
#include <ostream>

#define ANSI_CLEAR          "\x1B[0;00m"
#define ANSI_GREY           "\x1B[0;37m"
#define ANSI_WHITE          "\x1B[1;37m"
#define ANSI_GREEN          "\x1B[1;32m"
#define ANSI_YELLOW         "\x1B[1;33m"
#define ANSI_RED            "\x1B[1;31m"
#define ANSI_WHITE_ON_RED   "\x1B[1;37;41m"

namespace logging = boost::log;
namespace src = boost::log::sources;
namespace expr = boost::log::expressions;
namespace sinks = boost::log::sinks;
namespace attrs = boost::log::attributes;

BOOST_LOG_ATTRIBUTE_KEYWORD(line_id, "LineID", unsigned int)
BOOST_LOG_ATTRIBUTE_KEYWORD(timestamp, "TimeStamp", boost::posix_time::ptime)
BOOST_LOG_ATTRIBUTE_KEYWORD(severity, "Severity",
                            logging::trivial::severity_level)

void colouredFormatter(boost::log::record_view const& rec,
                       boost::log::formatting_ostream& strm) {
  using namespace std;
  // Init the new formatter
  logging::formatter formatter = expr::stream
      << setw(6) << setfill('0') << line_id << setfill(' ') << '|'
      << expr::format_date_time(timestamp, "%H:%M:%S.%f") << " ";

  formatter(rec, strm);
  using namespace boost::log::trivial;
  severity_level level = logging::extract<severity_level>(
      "Severity", rec).get();
  switch (level) {
  case trace:
    strm << ANSI_GREY;
    break;
  case debug:
    strm << ANSI_GREEN;
    break;
  case info:
    strm << ANSI_WHITE;
    break;
  case warning:
    strm << ANSI_YELLOW;
    break;
  case error:
    strm << ANSI_RED;
    break;
  case fatal:
    strm << ANSI_WHITE_ON_RED;
    break;
  }

  strm <<  '[' << setw(7) << setfill(' ') << level << setfill(' ') << ']'
      << " - " << rec[expr::smessage] << ANSI_CLEAR;
}

BOOST_LOG_GLOBAL_LOGGER_INIT(logger, src::severity_logger_mt) {
  src::severity_logger_mt<boost::log::trivial::severity_level> logger;

  // add attributes
  logger.add_attribute("LineID", attrs::counter<unsigned int>(1));
  logger.add_attribute("TimeStamp", attrs::local_clock());

  // add a text sink
  typedef sinks::synchronous_sink<sinks::text_ostream_backend> text_sink;
  boost::shared_ptr<text_sink> sink = boost::make_shared<text_sink>();

#ifdef LOGFILE
  // add a logfile stream to our sink
  sink->locked_backend()->add_stream(boost::make_shared<std::ofstream>(LOGFILE));
#endif

  // add "console" output stream to our sink
  sink->locked_backend()->add_stream(
      boost::shared_ptr<std::ostream>(&std::clog, boost::null_deleter()));

  // specify the format of the log message
  sink->set_formatter(&colouredFormatter);

  // only messages with severity >= SEVERITY_THRESHOLD are written
  sink->set_filter(severity >= SEVERITY_THRESHOLD);

  // "register" our sink
  logging::core::get()->add_sink(sink);

  return logger;
}