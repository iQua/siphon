#include "gtest/gtest.h"

#include "coder/udp_coder_interfaces.hpp"
#include "util/message_util.hpp"


namespace siphon {

class MinionGetter : public IMinionStop {
 public:
  MinionGetter() = default;

  Minion* get() {
    return minion_;
  }

  IMinionStop* process(Minion* minion) override {
    minion_ = minion;
    return nullptr;
  }

 private:
  Minion* minion_;
};

class DirectPassCoderTest : public ::testing::Test {
 protected:
  DirectPassCoderTest() {
    minion_ = nullptr;
    msg_ = std::make_unique<Message>();
    pool_.create(1);
    pool_.request(&getter_);
  }

  void SetUp() override {
    encoder_ = std::make_unique<DirectPassEncoder>();
    decoder_ = std::make_unique<DirectPassDecoder>();

    test_payload_ = "This is TestPayload!";
    msg_->mutable_header()->set_session_id("TestSession");
    msg_->mutable_header()->set_seq(0);
    msg_->mutable_header()->set_timestamp(util::CurrentUnixMicros());
    memcpy(msg_->allocateBuffer().data(),
           test_payload_.c_str(),
           test_payload_.size());
    msg_->resetPayload((const uint8_t *) msg_->allocateBuffer().data(),
                       test_payload_.size());

    minion_ = getter_.get();
    ASSERT_TRUE(minion_ != nullptr);
    minion_->swapData(msg_);
  }

  void testEqual() {
    EXPECT_EQ("TestSession",
              minion_->getData()->header()->session_id());
    EXPECT_EQ(0, minion_->getData()->header()->seq());
    std::string recovered_payload(
        (const char*) minion_->getData()->getPayload(),
        minion_->getData()->getPayloadSize());
    EXPECT_EQ(recovered_payload, test_payload_);
  }

  MinionPool pool_;
  MinionGetter getter_;
  Minion* minion_;

  std::unique_ptr<IUDPEncoder> encoder_;
  std::unique_ptr<IUDPDecoder> decoder_;

  std::string test_payload_;
  std::unique_ptr<Message> msg_;
};

TEST_F(DirectPassCoderTest, DataCanBeRecoveredAfterDecodingAndDecoding) {
  // Encode.
  ASSERT_TRUE(encoder_->encode(minion_));

  // Make a copy of the serialized message.
  auto received_msg = std::make_unique<Message>();
  Message::ConstBufferSequence buf = minion_->getData()->toBuffer();
  Message::MutableBufferSequence serialized_buf =
      received_msg->getReceivingBuffer(true);
  boost::asio::buffer_copy(serialized_buf, buf);
  received_msg->fromBuffer(serialized_buf, true);

  // Decode.
  ASSERT_TRUE(decoder_->decode(minion_));
  testEqual();
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}
