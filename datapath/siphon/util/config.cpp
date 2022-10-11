//
// Created by Shuhao Liu on 2018-03-04.
//

#include "util/config.hpp"

#include "rapidjson/filereadstream.h"
#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#include "util/logger.hpp"


namespace siphon {
namespace util {

Config* Config::instance_ = nullptr;

const Config* Config::get() {
  if (instance_ == nullptr) instance_ = new Config();
  return instance_;
}

Config* Config::getMutable() {
  if (instance_ == nullptr) instance_ = new Config();
  return instance_;
}

bool Config::loadFromConfigurationFile(const std::string& file_path) {
  if (file_path.empty()) {
    return false;
  }

  using namespace rapidjson;
  FILE *config;

  try {
    config = fopen(file_path.c_str(), "r");
    if (!config) {
      LOG_ERROR << "Given configuration " << file_path << " does not exist.";
      return false;
    }
    char readBuffer[65536];
    FileReadStream is(config, readBuffer, sizeof(readBuffer));
    Document d;
    d.ParseStream(is);
    configWithJsonObject(d);
  }
  catch (std::exception &e) {
    LOG_ERROR << "Error: " << e.what();
    return false;
  }

  fclose(config);
  return true;
}

void Config::configWithJsonObject(const rapidjson::Document& d) {
  // kMaxMessageSize
  if (d.HasMember("MaxMessageSize") && d["MaxMessageSize"].IsUint64()) {
    kMaxMessageSize = d["MaxMessageSize"].GetUint64();
  }

  // UDP or TCP
  if (d.HasMember("TCP")) {
    kUseTCP = true;
    kUseUDP = false;

    // Parse TCP options here.
  }
  else if (d.HasMember("UDP")) {
    assert(d["UDP"].IsObject());
    // Parse UDP options here.
    const rapidjson::Value& udp_options = d["UDP"];
    if (udp_options.HasMember("CoderName") &&
        udp_options["CoderName"].IsString()) {
      kUDPCoderName = udp_options["CoderName"].GetString();
    }
    if (udp_options.HasMember("CoderDelayConstraint") &&
        udp_options["CoderDelayConstraint"].IsInt()) {
      kDefaultUDPCoderDelayConstraint =
          (int) udp_options["CoderDelayConstraint"].GetInt();
    }
    if (udp_options.HasMember("CoderLossRate") &&
        udp_options["CoderLossRate"].IsInt()) {
      kDefaultUDPCoderLossRate =
          (int) udp_options["CoderLossRate"].GetInt();
    }
    if (udp_options.HasMember("CoderLossBurst") &&
        udp_options["CoderLossBurst"].IsInt()) {
      kDefaultUDPCoderLossBurst =
          (int) udp_options["CoderLossBurst"].GetInt();
    }
  }
  // Else, use UDP with default options.

  if (d.HasMember("PseudoSessions") && d["PseudoSessions"].IsArray()) {
    using namespace rapidjson;
    const Value& sessions = d["PseudoSessions"];
    for (SizeType i = 0; i < sessions.Size(); i++) {
      const Value& session = sessions[i];
      if (!session.HasMember("SessionID") || !session["SessionID"].IsString() ||
          !session.HasMember("Src") || !session["Src"].IsUint() ||
          !session.HasMember("Dst") || !session["Dst"].IsUint() ||
          !session.HasMember("Rate") || !session["Rate"].IsNumber() ||
          !session.HasMember("BurstSize") || !session["BurstSize"].IsUint64()) {
        StringBuffer buffer;
        Writer<StringBuffer> writer(buffer);
        session.Accept(writer);
        LOG_WARNING << "Missing required field in \"PseudoSessions\", skipping:"
                       " " << buffer.GetString();
        continue;
      }

      const std::string sessionID = session["SessionID"].GetString();
      const std::string sessionType = session["SessionType"].GetString();
      nodeID_t src = session["Src"].GetUint();
      nodeID_t dst = session["Dst"].GetUint();
      double rate = session["Rate"].GetDouble();
      size_t burst = session["BurstSize"].GetUint64();
      const std::string originalDataPath = session["OriDataPath"].GetString();

      if (session.HasMember("MessageSize") &&
          session["MessageSize"].IsUint64()) {
        pseudo_session_configs_.emplace_back(sessionID, sessionType, originalDataPath, src, dst, rate, burst,
            session["MessageSize"].GetUint64(), session["PayloadSize"].GetUint64());
        // All the session should have same message size and
	    kMessageSize = session["MessageSize"].GetUint64();
	    kPayloadSize = session["PayloadSize"].GetUint64();
	    kOriDataPath = session["OriDataPath"].GetString();
      }
      else {
        pseudo_session_configs_.emplace_back(sessionID, sessionType, originalDataPath, src, dst, rate, burst,
            util::Config::get()->kMaxMessageSize, session["PayloadSize"].GetUint64());
      }
    }
  }
}

}
}

