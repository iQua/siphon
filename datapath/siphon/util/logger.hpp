/**
 * @file Logger.hpp
 * @brief Defining the Logger class, which wraps the Boost Trivial Logging.
 *
 * @author Shuhao Liu (shuhao@ece.toronto.edu)
 * @date November 2, 2015
 *
 * @ingroup fundamentals
 */

#ifndef LOGGER_HPP
#define LOGGER_HPP

#define BOOST_LOG_DYN_LINK 1

#include <boost/log/trivial.hpp>
#include <boost/log/sources/global_logger_storage.hpp>

/**
 * Define a log file written to.
 */
// #define LOGFILE "logfile.log"

/*!
  \def LOG_TRACE
  Log Macro. LogLevel: Trace. Usage: LOG_TRACE << "A trace message"
  \def LOG_DEBUG
  Log Macro. LogLevel: Debug. Usage: LOG_DEBUG << "A debug message"
  \def LOG_INFO
  Log Macro. LogLevel: Info. Usage: LOG_INFO << "A info message"
  \def LOG_WARNING
  Log Macro. LogLevel: Warning. Usage: LOG_WARNING << "A warning message"
  \def LOG_ERROR
  Log Macro. LogLevel: Error. Usage: LOG_ERROR << "A error message"
  \def LOG_FATAL
  Log Macro. LogLevel: Fatal. Usage: LOG_FATAL << "A fatal message"
*/

/**
 * Log messages with severity >= SEVERITY_THRESHOLD are written.
 */
#define SEVERITY_THRESHOLD logging::trivial::debug

/**
 * Register a global logger
 */
BOOST_LOG_GLOBAL_LOGGER(logger, boost::log::sources::severity_logger_mt<
    boost::log::trivial::severity_level>)

/**
 * A helper macro.
 */
#define LOG(severity) \
    BOOST_LOG_SEV(logger::get(), boost::log::trivial::severity)

/**
 * Log Macros
 */
#define LOG_TRACE   LOG(trace)
#define LOG_DEBUG   LOG(debug)
#define LOG_INFO    LOG(info)
#define LOG_WARNING LOG(warning)
#define LOG_ERROR   LOG(error)
#define LOG_FATAL   LOG(fatal)


#endif

