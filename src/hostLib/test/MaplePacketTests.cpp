#include "MockMapleBus.hpp"
#include "MockDreamcastControllerObserver.hpp"
#include "MockDreamcastPeripheral.hpp"
#include "MockMutex.hpp"

#include "hal/MapleBus/MaplePacket.hpp"

#include <memory>
#include <utility>

#include <gtest/gtest.h>
#include <gmock/gmock.h>

using ::testing::_;
using ::testing::Return;
using ::testing::SetArgReferee;
using ::testing::DoAll;

TEST(MaplePacketConstructorTest, one)
{
    uint32_t payload[5] = {0xCCCCCCCC, 0xDDDDDDDD, 0xEEEEEEEE, 0xFFFFFFFF, 0x12345678};
    MaplePacket pkt({.command=0xAA, .recipientAddr=0xBB}, payload, 5);
    EXPECT_EQ(pkt.frame.toWord(), 0xAABB0005);
    ASSERT_EQ(pkt.payload.size(), 5);
    EXPECT_EQ(pkt.payload[0], 0xCCCCCCCC);
    EXPECT_EQ(pkt.payload[1], 0xDDDDDDDD);
    EXPECT_EQ(pkt.payload[2], 0xEEEEEEEE);
    EXPECT_EQ(pkt.payload[3], 0xFFFFFFFF);
    EXPECT_EQ(pkt.payload[4], 0x12345678);
    EXPECT_TRUE(pkt.isValid());
}

TEST(MaplePacketConstructorTest, two)
{
    MaplePacket pkt({.command=0x45, .recipientAddr=0x89}, 0xF8675309);
    EXPECT_EQ(pkt.frame.toWord(), 0x45890001);
    ASSERT_EQ(pkt.payload.size(), 1);
    EXPECT_EQ(pkt.payload[0], 0xF8675309);
    EXPECT_TRUE(pkt.isValid());
}

TEST(MaplePacketConstructorTest, three1)
{
    uint32_t payload[3] = {0x12345678, 0x87654321, 0xABABABAB};
    MaplePacket pkt(MaplePacket::Frame::fromWord(0x11111111), payload, 3);
    EXPECT_EQ(pkt.frame.toWord(), 0x11111103);
    ASSERT_EQ(pkt.payload.size(), 3);
    EXPECT_EQ(pkt.payload[0], 0x12345678);
    EXPECT_EQ(pkt.payload[1], 0x87654321);
    EXPECT_EQ(pkt.payload[2], 0xABABABAB);
}

TEST(MaplePacketConstructorTest, three2)
{
    uint32_t payload[1] = {0x12345678};
    MaplePacket pkt(MaplePacket::Frame::fromWord(0x11111101), payload, 1);
    EXPECT_EQ(pkt.frame.toWord(), 0x11111101);
    ASSERT_EQ(pkt.payload.size(), 1);
    EXPECT_EQ(pkt.payload[0], 0x12345678);
    EXPECT_TRUE(pkt.isValid());
}

TEST(MaplePacketConstructorTest, four1)
{
    MaplePacket pkt(NULL, 0);
    EXPECT_TRUE(pkt.payload.empty());
    EXPECT_FALSE(pkt.isValid()); // No frame word was given
}

TEST(MaplePacketConstructorTest, four2)
{
    uint32_t words[5] = {0x65733fc7, 0x2c1bea03, 0x70574afc, 0xc720a39d, 0x0b34e5b4};
    MaplePacket pkt(words, 5);
    EXPECT_EQ(pkt.frame.toWord(), 0x65733f04);
    ASSERT_EQ(pkt.payload.size(), 4);
    EXPECT_EQ(pkt.payload[0], 0x2c1bea03);
    EXPECT_EQ(pkt.payload[1], 0x70574afc);
    EXPECT_EQ(pkt.payload[2], 0xc720a39d);
    EXPECT_EQ(pkt.payload[3], 0x0b34e5b4);
}

TEST(MaplePacketConstructorTest, four3)
{
    uint32_t words[5] = {0x65733f04, 0x2c1bea03, 0x70574afc, 0xc720a39d, 0x0b34e5b4};
    MaplePacket pkt(words, 5);
    EXPECT_EQ(pkt.frame.toWord(), 0x65733f04);
    ASSERT_EQ(pkt.payload.size(), 4);
    EXPECT_EQ(pkt.payload[0], 0x2c1bea03);
    EXPECT_EQ(pkt.payload[1], 0x70574afc);
    EXPECT_EQ(pkt.payload[2], 0xc720a39d);
    EXPECT_EQ(pkt.payload[3], 0x0b34e5b4);
    EXPECT_TRUE(pkt.isValid());
}

TEST(MaplePacketCopyConstructorTest, one)
{
    uint32_t words[5] = {0x65733f04, 0x2c1bea03, 0x70574afc, 0xc720a39d, 0x0b34e5b4};
    MaplePacket pkt1(words, 5);
    MaplePacket pkt2(pkt1);
    EXPECT_EQ(pkt1, pkt2);
    EXPECT_EQ(pkt2.frame.toWord(), 0x65733f04);
    ASSERT_EQ(pkt2.payload.size(), 4);
    EXPECT_EQ(pkt2.payload[0], 0x2c1bea03);
    EXPECT_EQ(pkt2.payload[1], 0x70574afc);
    EXPECT_EQ(pkt2.payload[2], 0xc720a39d);
    EXPECT_EQ(pkt2.payload[3], 0x0b34e5b4);
    EXPECT_TRUE(pkt2.isValid());
}

TEST(MaplePacketMoveConstructorTest, one)
{
    uint32_t words[5] = {0x65733f04, 0x2c1bea03, 0x70574afc, 0xc720a39d, 0x0b34e5b4};
    MaplePacket pkt1(words, 5);
    MaplePacket pkt2(std::move(pkt1));
    EXPECT_TRUE(pkt1.payload.empty());
    EXPECT_EQ(pkt2.frame.toWord(), 0x65733f04);
    ASSERT_EQ(pkt2.payload.size(), 4);
    EXPECT_EQ(pkt2.payload[0], 0x2c1bea03);
    EXPECT_EQ(pkt2.payload[1], 0x70574afc);
    EXPECT_EQ(pkt2.payload[2], 0xc720a39d);
    EXPECT_EQ(pkt2.payload[3], 0x0b34e5b4);
    EXPECT_TRUE(pkt2.isValid());
}

TEST(MaplePacketDefaultConstructorTest, constructAndSet)
{
    MaplePacket pkt;
    EXPECT_FALSE(pkt.isValid());
    uint32_t words[3] = {0x59ef2302, 0x8abf3c75, 0x86f4b5e2};
    pkt.set(words, 3);
    EXPECT_EQ(pkt.frame.toWord(), 0x59ef2302);
    ASSERT_EQ(pkt.payload.size(), 2);
    EXPECT_EQ(pkt.payload[0], 0x8abf3c75);
    EXPECT_EQ(pkt.payload[1], 0x86f4b5e2);
    EXPECT_TRUE(pkt.isValid());
}

TEST(MaplePacketGettersTest, one)
{
    uint32_t payload[10] = {0xfe020f24, 0x43c50fe9, 0xa8548824, 0xeafa0621, 0xb38885fb,
                            0x298a0866, 0x88671e8a, 0x2a08905a, 0xb5f3eb50, 0xfa9bb913};
    MaplePacket pkt({.command=0x6d, .recipientAddr=0xc4}, payload, 10);
    pkt.frame.senderAddr = 0xc2;
    EXPECT_TRUE(pkt.isValid());
    EXPECT_EQ(pkt.frame.length, 10);
    EXPECT_EQ(pkt.frame.senderAddr, 0xc2);
    EXPECT_EQ(pkt.frame.recipientAddr, 0xc4);
    EXPECT_EQ(pkt.frame.command, 0x6d);
    EXPECT_EQ(pkt.frame.toWord(), 0x6dc4c20a);
    EXPECT_EQ(pkt.getNumTotalBits(), 360);
    EXPECT_EQ(pkt.getTxTimeNs(), 179520);
}
