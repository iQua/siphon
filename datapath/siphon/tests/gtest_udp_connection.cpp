#include "gtest/gtest.h"

#include "networking/udp_connection_manager.hpp"

#include "base/thread_pool.hpp"
#include "coder/udp_coder_factory.hpp"
#include "tests/fake_connection_test_util.hpp"
#include "util/config.hpp"

/*
namespace siphon {

class UDPConnectionTest : public ::testing::Test {
 protected:
  UDPConnectionTest() : threads_(ios_) {
    util::Config::getMutable()->kUDPCoderName = "test";
    pool_.create(10);
  }

  void SetUp() override {
    threads_.run();
    crossbar_ = std::make_unique<test::FakeCrossbar>(&pool_);
    manager_ = std::make_unique<UDPConnectionManager>(
        ios_, &pool_, 1);
    manager_->init(crossbar_.get());
    ISender* sender = manager_->create("127.0.0.1", 1);
    generator_ = std::make_unique<test::FakeMessageGenerator>(sender);
  }

  void TearDown() override {
    threads_.stop();
  }

  boost::asio::io_context ios_;
  ThreadPool threads_;

  MinionPool pool_;

  std::unique_ptr<test::FakeCrossbar> crossbar_;
  std::unique_ptr<UDPConnectionManager> manager_;
  std::unique_ptr<test::FakeMessageGenerator> generator_;
};

TEST_F(UDPConnectionTest, CanReceiveDataCorrectly) {
  ASSERT_EQ("test", util::Config::get()->kUDPCoderName);

  // Generate 1st message.
  pool_.request(generator_.get());
  boost::this_thread::sleep_for(boost::chrono::milliseconds(300));
  // Now transfer is completed and acknowledged.
  test::FakeEncoder* encoder = (test::FakeEncoder*)
      ((UDPSender*) manager_->getSender(1))->encoders_["TestSession"].get();
  test::FakeDecoder* decoder = (test::FakeDecoder*)
      ((UDPReceiver*) manager_->getReceiver(1))->decoders_["TestSession"].get();
  uint32_t params = encoder->getCodingParams();
  EXPECT_EQ(params, decoder->getEncodedParameters() & 0xFFFFFF);

  // Generate 2nd message.
  params += 0x1010101;
  pool_.request(generator_.get());
  boost::this_thread::sleep_for(boost::chrono::milliseconds(300));
  EXPECT_EQ(params, decoder->getEncodedParameters());

  threads_.cancelAfterSeconds(1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}
*/