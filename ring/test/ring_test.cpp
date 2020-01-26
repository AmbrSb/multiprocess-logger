#include <gtest/gtest.h>
#include <ring.h>

using ::testing::EmptyTestEventListener;
using ::testing::InitGoogleTest;
using ::testing::Test;
using ::testing::TestEventListeners;
using ::testing::TestInfo;
using ::testing::TestPartResult;
using ::testing::UnitTest;

namespace {

using namespace std::literals;

TEST(Ring, Create) {
    auto r = ring_init("Ring.Create", 50, sizeof(elem));
    ASSERT_NE(r, nullptr);
    ring_free(r);
}

TEST(Ring, CreateTooLarge) {
    auto r = ring_init("Ring.CreateTooLarge",
                        RING_CAPACITY + 1,
                        8);
    ASSERT_EQ(r, nullptr);
}

TEST(Ring, Push) {
    ring* r = nullptr;
    r = ring_init("Ring.Push", 50, sizeof(elem));
    ASSERT_NE(r, nullptr);
    elem e1 {11};
    auto ret = ring_enqueue(r, &e1);
    ASSERT_EQ(ret, 0);
    ring_free(r);
}

TEST(Ring, PushPop) {
    auto r = ring_init("Ring.PushPop", 50, sizeof(elem));
    elem e1 {1234};
    auto ret = ring_enqueue(r, &e1);
    elem* e2;
    ret = ring_dequeue(r, &e2);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(e2->id, 1234);
    ring_free(r);
}

TEST(Ring, PushLookupPop) {
    auto r = ring_init("Ring.PushLookupPop", 50, sizeof(elem));
    elem e1 {1234};
    auto ret = ring_enqueue(r, &e1);

    auto rx = ring_lookup("Ring.PushLookupPop");
    elem* e2;
    ret = ring_dequeue(rx, &e2);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(e2->id, 1234);
    ring_free(r);
}

TEST(Ring, PushPopString) {
    auto r = ring_init("Ring.PushPopString", 50, sizeof(elem));
    elem e1 {1234, "hello"};
    auto ret = ring_enqueue(r, &e1);
    elem* e2;
    ret = ring_dequeue(r, &e2);
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(e2->id, 1234);
    ASSERT_STREQ(reinterpret_cast<char*>(e2->data),
                 reinterpret_cast<char*>(e1.data));
    ring_free(r);
}

TEST(Ring, DequeueAfterEmpty) {
    auto r = ring_init("Ring.DequeueAfterEmpty",
                        100,
                        sizeof(elem));
    elem* e;
    auto ret = ring_dequeue(r, &e);
    ASSERT_EQ(ret, -1);
    ring_free(r);
}

TEST(Ring, PushToCapacity) {
    auto r = ring_init("Ring.PushToCapacity",
                        RING_CAPACITY,
                        sizeof(elem));
    for (size_t i = 0; i < RING_CAPACITY; i++) {
        elem e {i};
        auto ret = ring_enqueue(r, &e);
        ASSERT_EQ(ret, 0);
    }
    for (size_t i = 0; i < RING_CAPACITY; i++) {
        elem *e;
        auto ret = ring_dequeue(r, &e);
        ASSERT_EQ(ret, 0);
        ASSERT_EQ(e->id, i);
    }
    ring_free(r);
}

}
