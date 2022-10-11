#include "gtest/gtest.h"

#include <string>

#include "base/message.hpp"
#include "util/message_util.hpp"
#include "util/logger.hpp"

namespace siphon {

class MessageTest : public ::testing::Test {
 protected:
  void SetUp() override {
    test_payload_ = "This is TestPayload!";
    msg1_.mutable_header()->set_session_id("TestSession");
    msg1_.mutable_header()->set_seq(0);
    msg1_.mutable_header()->set_timestamp(util::CurrentUnixMicros());
    memcpy(msg1_.allocateBuffer().data(),
        test_payload_.c_str(),
        test_payload_.size());
    msg1_.resetPayload((const uint8_t *) msg1_.allocateBuffer().data(),
                       test_payload_.size());
  }

  Message msg1_;
  Message msg2_;

  std::string test_payload_;
};

TEST_F(MessageTest, MessageContentCanBeRecoveredAfterTCPTypeSerialization) {
  LOG_INFO << "Originial SessionID: " << msg1_.header()->session_id();
  Message::ConstBufferSequence buf = msg1_.toBuffer();
  EXPECT_EQ(boost::asio::buffer_size(buf),
      msg1_.header()->ByteSize() + test_payload_.size() +
      sizeof(Message::MessageSizeType) + sizeof(Message::HeaderSizeType));
  EXPECT_EQ(msg1_.message_size(),
      msg1_.header()->ByteSize() + test_payload_.size() +
      sizeof(Message::HeaderSizeType));

  // Read the message size in, first.
  boost::asio::mutable_buffer message_size_buf =
      msg2_.getMessageSizeReceivingBuffer();
  ASSERT_EQ(message_size_buf.size(), sizeof(Message::MessageSizeType));
  boost::asio::buffer_copy(message_size_buf, buf);
  // Remove the message size field from buf.
  buf[0] += sizeof(Message::MessageSizeType);

  // Copy it into a new buffer.
  Message::MutableBufferSequence serialized_buf = msg2_.getReceivingBuffer();
  ASSERT_EQ(boost::asio::buffer_size(serialized_buf), msg2_.message_size());
  boost::asio::buffer_copy(serialized_buf, buf);
  msg2_.fromBuffer(serialized_buf);

  // Test msg2_ recovered content.
  LOG_INFO << "Recovered SessionID: " << msg2_.header()->session_id();
  EXPECT_EQ(msg2_.header()->session_id(), "TestSession");
  EXPECT_EQ(msg2_.header()->seq(), 0);
  EXPECT_LE(msg2_.header()->timestamp(), util::CurrentUnixMicros());
  std::string recovered_payload((const char*) msg2_.getPayload(),
                                msg2_.getPayloadSize());
  EXPECT_EQ(recovered_payload, test_payload_);

  // If serialize msg2_ again, the size and content would be the same.
  Message::ConstBufferSequence buf2 = msg2_.toBuffer();
  buf2[0] += sizeof(Message::MessageSizeType);  // The same goes with buf.
  size_t length = boost::asio::buffer_size(buf2);
  ASSERT_EQ(boost::asio::buffer_size(buf), length);
  auto p1 = boost::asio::buffers_begin(buf);
  auto p2 = boost::asio::buffers_begin(buf2);
  for (int i = 0; i < length; ++i) {
    EXPECT_EQ(*p1, *p2);
    ++p1;
    ++p2;
  }
}

TEST_F(MessageTest, MessageContentCanBeRecoveredAfterUDPTypeSerialization) {
  LOG_INFO << "Originial SessionID: " << msg1_.header()->session_id();
  Message::ConstBufferSequence buf = msg1_.toBuffer();
  EXPECT_EQ(boost::asio::buffer_size(buf),
            msg1_.header()->ByteSize() + test_payload_.size() +
                sizeof(Message::MessageSizeType) + sizeof(Message::HeaderSizeType));
  EXPECT_EQ(msg1_.message_size(),
            msg1_.header()->ByteSize() + test_payload_.size() +
                sizeof(Message::HeaderSizeType));

  // Copy it into the msg2_ UDP receiving buffer.
  Message::MutableBufferSequence serialized_buf =
      msg2_.getReceivingBuffer(true);
  boost::asio::buffer_copy(serialized_buf, buf);
  msg2_.fromBuffer(serialized_buf, true);

  // Test msg2_ recovered content.
  LOG_INFO << "Recovered SessionID: " << msg2_.header()->session_id();
  EXPECT_EQ(msg2_.header()->session_id(), "TestSession");
  EXPECT_EQ(msg2_.header()->seq(), 0);
  EXPECT_LE(msg2_.header()->timestamp(), util::CurrentUnixMicros());
  std::string recovered_payload((const char*) msg2_.getPayload(),
                                msg2_.getPayloadSize());
  EXPECT_EQ(recovered_payload, test_payload_);

  // If serialize msg2_ again, the size and content would be the same.
  Message::ConstBufferSequence buf2 = msg2_.toBuffer();
  size_t length = boost::asio::buffer_size(buf2);
  ASSERT_EQ(boost::asio::buffer_size(buf), length);
  auto p1 = boost::asio::buffers_begin(buf);
  auto p2 = boost::asio::buffers_begin(buf2);
  for (int i = 0; i < length; ++i) {
    EXPECT_EQ(*p1, *p2);
    ++p1;
    ++p2;
  }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}
