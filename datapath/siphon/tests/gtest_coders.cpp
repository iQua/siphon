//
// Created by Sijia Chen on 2020/4/27.
//

#include "gtest/gtest.h"

//#include "FEC/include/basicOperations.h"
//#include "FEC/include/codingOperations.h"
#include "coder/udp_coder_interfaces.hpp"
#include "util/message_util.hpp"
#include "coder/udp_coder_IReStrC_interfaces.h"
#include "coder/udp_coder_IParityStrC_interfaces.h"
//#include "coder/udp_coder_IFEC_interfaces.h"


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

    class ICodersTest : public ::testing::Test {
    protected:
        ICodersTest() {
            minion_ = nullptr;
            msg_ = std::make_unique<Message>();
            pool_.create(1);
            pool_.request(&getter_);
        }

        // demo1: IReStrEnCoder, IReStrDeCoder
        // demo3: IParityStrEnCoder, IParityStrDeCoder
        // demo4: IFECEnCoder, IFECDeCoder
        void SetUp() override {
            // replace the encoder and decoder here
            src_encoder_ = std::make_unique<IReStrEnCoder>();

            relay_encoder_ = std::make_unique<IReStrEnCoder>();
            relay_decoder_ = std::make_unique<IReStrDeCoder>();

            dst_decoder_ = std::make_unique<IReStrDeCoder>();

            local_nodeID_ = 0;
            peer_nodeID_ = 0;

            msg_->mutable_header()->set_session_id("TestSession");
            msg_->mutable_header()->set_seq(0);
            msg_->mutable_header()->set_timestamp(util::CurrentUnixMicros());


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

        void set_msg_header()
        {
            /* modify the message header */
            auto set_header_fields = [this](Message* message) -> void {
                MessageHeader* header = message->mutable_header();
                header->set_payload_size(message->getPayloadSize());
                header->set_ack(false);
                header->set_src(local_nodeID_);
                header->set_dst(peer_nodeID_);
            };
            /* Set the header for message. */
            set_header_fields(minion_->getData());
            for (auto& msg : *(minion_->getExtraData())) {
                set_header_fields(msg.get());
            }
        }

        MinionPool pool_;
        MinionGetter getter_;
        Minion* minion_;

        size_t local_nodeID_;
        size_t peer_nodeID_;

        // coder for source
        std::unique_ptr<IUDPEncoder> src_encoder_;
        // coder for relay
        std::unique_ptr<IUDPEncoder> relay_encoder_;
        std::unique_ptr<IUDPDecoder> relay_decoder_;
        // coder for dst
        std::unique_ptr<IUDPDecoder> dst_decoder_;

        std::string test_payload_;
        std::unique_ptr<Message> msg_;
    };

    TEST_F(ICodersTest, CoderDetectionInThreenodes) {

        for(int i=0; i<14;i++){

            // the source side
            // Encode.
            ASSERT_TRUE(src_encoder_->encode(minion_));
            // Set the header for message.
            local_nodeID_ = 2;
            peer_nodeID_ = 1;
            set_msg_header();

//
//            // the relay side
//            // decode.
//            ASSERT_TRUE(relay_decoder_->decode(minion_));
//
//            // Encode.
//            ASSERT_TRUE(relay_encoder_->encode(minion_));
//            // Set the header for message.
//            local_nodeID_ = 1;
//            peer_nodeID_ = 3;
//            set_msg_header();


            // the destination
            // Make a copy of the serialized message.
            auto received_msg = std::make_unique<Message>();
            Message::ConstBufferSequence buf = minion_->getData()->toBuffer();
            Message::MutableBufferSequence serialized_buf =
                    received_msg->getReceivingBuffer(true);
            boost::asio::buffer_copy(serialized_buf, buf);
            received_msg->fromBuffer(serialized_buf, true);

            // Decode.
            printf("\nDecoder side: ");
            ASSERT_TRUE(dst_decoder_->decode(minion_));

            // reset the parameters
            local_nodeID_ = 0;
            peer_nodeID_ = 0;
            set_msg_header();
            cout << endl;
        }

    }

    int main(int argc, char **argv) {
        ::testing::InitGoogleTest(&argc, argv);
        return RUN_ALL_TESTS();
    }

}
