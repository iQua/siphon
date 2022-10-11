#ifndef SIPHON_TEST_FAKE_CONNECTION_UTIL
#define SIPHON_TEST_FAKE_CONNECTION_UTIL

#include <string>

#include <gtest/gtest.h>

#include "base/minion_pool.hpp"
#include "coder/udp_coder_interfaces.hpp"
#include "networking/connection_interfaces.hpp"
#include "util/logger.hpp"
#include "util/message_util.hpp"

namespace siphon {
namespace test {

class FakeMessageGenerator : public IMinionStop {
 public:
  FakeMessageGenerator(ISender* sender) : sender_(sender), seq_(0) {}

  IMinionStop* process(Minion* minion) override {
    // Generate Message.
    std::string payload = "TestPayload";
    auto msg = std::make_unique<Message>();
    msg->mutable_header()->set_session_id("TestSession");
    msg->mutable_header()->set_seq(seq_++);
    msg->mutable_header()->set_timestamp(util::CurrentUnixMicros());
    memcpy(msg->allocateBuffer().data(),
           payload.c_str(),
           payload.size());
    msg->resetPayload((const uint8_t *) msg->allocateBuffer().data(),
                      payload.size());

    // Swap it with the minion and send it.
    minion->swapData(msg);
    return sender_;
  }

 private:
  ISender* sender_;
  size_t seq_;
};

class FakeCrossbar : public IMinionStop {
 public:
  FakeCrossbar(MinionPool* pool) : pool_(pool), seq_(0) {}

  IMinionStop* process(Minion* minion) override {
    EXPECT_EQ("TestSession",
              minion->getData()->header()->session_id());
    EXPECT_EQ(seq_++, minion->getData()->header()->seq());
    std::string recovered_payload(
        (const char*) minion->getData()->getPayload(),
        minion->getData()->getPayloadSize());
    EXPECT_EQ(recovered_payload, "TestPayload");
    return pool_;
  }

 private:
  MinionPool* pool_;
  size_t seq_;
};

}
}

#endif
