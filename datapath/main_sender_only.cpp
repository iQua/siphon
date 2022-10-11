#include <boost/asio.hpp>

#include <iostream>
#include <string>

#include "aggregator.hpp"


void usage() {
  std::cout << "Usage: siphon controller_url [config_file]\nExiting...\n"
            << std::endl;
}


int main(int argc, char** argv) {
  if (argc != 2 && argc != 3) {
    usage();
    return -1;
  }

  try {
    std::string url(argv[1]);

    if (argc == 3) {
      std::string config_file = std::string(argv[2]);
      siphon::util::Config::getMutable()->loadFromConfigurationFile(
          config_file);
    }
    // Set to sender only mode.
    siphon::util::Config::getMutable()->kLocalDebugNoReceivingSocket = true;

    boost::asio::io_context ios;
    siphon::Aggregator aggregator(ios);
    aggregator.start(url);

    // Block here
    aggregator.waitUntilErrorDetected();
  }
  catch (const std::exception& e) {
    LOG_FATAL << "ERROR encountered: " << e.what();
    return -1;
  }
  catch (const boost::system::error_code& ec) {
    LOG_FATAL << "ERROR encountered: " << ec.message();
    return -1;
  }

  return 0;
}