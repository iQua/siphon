#include "gtest/gtest.h"

#include "networking/tcp_connection_manager.hpp"

#include "base/thread_pool.hpp"
#include "tests/fake_connection_test_util.hpp"
#include "util/config.hpp"


namespace siphon {

class TCPConnectionTest : public ::testing::Test {
 protected:
  TCPConnectionTest() : threads_(ios_) {
    pool_.create(10);
  }

  void SetUp() override {
    threads_.run();
    crossbar_ = std::make_unique<test::FakeCrossbar>(&pool_);
    manager_ = std::make_unique<TCPConnectionManager>(
        ios_, &pool_, 1);
    manager_->init(crossbar_.get());
  }

  void TearDown() override {
    threads_.stop();
  }

  boost::asio::io_context ios_;
  ThreadPool threads_;

  MinionPool pool_;

  std::unique_ptr<test::FakeCrossbar> crossbar_;
  std::unique_ptr<TCPConnectionManager> manager_;
  std::unique_ptr<test::FakeMessageGenerator> generator_;
};

TEST_F(TCPConnectionTest, NodeBeingConnectedHasASenderCreated) {
  TCPSender sender1(ios_, &pool_, 1, 2, "127.0.0.1");
  generator_ = std::make_unique<test::FakeMessageGenerator>(&sender1);

  // Generate 1st message from sender1.
  pool_.request(generator_.get());
  boost::this_thread::sleep_for(boost::chrono::milliseconds(300));

  // Generate 2nd message from the receiver.
  TCPSender* sender2 = (TCPSender*) manager_->getSender(1);
  ASSERT_TRUE(sender2 != nullptr);
  EXPECT_EQ(sender2->socket()->remote_endpoint().address().to_string(),
            "127.0.0.1");
  EXPECT_NE(sender2->socket()->remote_endpoint().port(),
            util::Config::get()->kTCPListeningPort);

  threads_.cancelAfterSeconds(1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}
