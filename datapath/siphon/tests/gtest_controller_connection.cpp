#include "gtest/gtest.h"

#include "controller/controller_connection.hpp"

#include "base/thread_pool.hpp"
#include "controller/controller_connection.hpp"
#include "controller/notification_types.hpp"
#include "tests/fake_controller.hpp"


namespace siphon {

class TestControllerConnection : public ControllerConnection {
 protected:
  void onControlMsgReceived(const rapidjson::Document& msg) override {
    ASSERT_TRUE(msg.IsObject());
    ASSERT_TRUE(msg.HasMember("Type"));
    type_ = (ControlMsgType) msg["Type"].GetUint();
  }

 public:
  TestControllerConnection(boost::asio::io_context::strand& strand) :
      ControllerConnection(strand) {}

  ControlMsgType type() const {
    return type_;
  }

 private:
  ControlMsgType type_;
};

class ControllerConnectionTest : public ::testing::Test {
 protected:
  ControllerConnectionTest() : pool_(ios_), controller_(ios_) {}

  void SetUp() override {
    pool_.run();
    controller_.start();
  }

  void TearDown() override {
    controller_.close();
    pool_.stop();
  }

  boost::asio::io_context ios_;
  ThreadPool pool_;
  test::FakeController controller_;
};

TEST_F(ControllerConnectionTest, CanReceiveDataCorrectly) {
  boost::asio::io_context::strand strand(ios_);
  TestControllerConnection conn(strand);
  nodeID_t nodeID = conn.connectAndStartAtNodeID("127.0.0.1",
      test::FakeController::kFakeControllerPort);

  // Assert whether the hostname is successfully reported to the controller.
  ASSERT_EQ(nodeID, test::FakeController::kNodeID);
  ASSERT_EQ(controller_.hostname(), boost::asio::ip::host_name());

  // Receive a control message.
  controller_.sendMessage("{\"Type\": 3}");
  boost::this_thread::sleep_for(boost::chrono::seconds(1));
  EXPECT_EQ(conn.type(), 3);

  pool_.cancelAfterSeconds(1);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

}
