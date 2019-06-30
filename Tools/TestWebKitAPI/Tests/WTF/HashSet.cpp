/*
 * Copyright (C) 2012 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"

#include "Counters.h"
#include "MoveOnly.h"
#include <functional>
#include <wtf/HashSet.h>
#include <wtf/RefPtr.h>

namespace TestWebKitAPI {

template<int initialCapacity>
    struct InitialCapacityTestHashTraits : public WTF::UnsignedWithZeroKeyHashTraits<int> {
    static const int minimumTableSize = initialCapacity;
};

template<unsigned size>
void testInitialCapacity()
{
    const unsigned initialCapacity = WTF::HashTableCapacityForSize<size>::value;
    HashSet<int, DefaultHash<int>::Hash, InitialCapacityTestHashTraits<initialCapacity> > testSet;

    // Initial capacity is null.
    ASSERT_EQ(0u, testSet.capacity());

    // Adding items up to size should never change the capacity.
    for (size_t i = 0; i < size; ++i) {
        testSet.add(i);
        ASSERT_EQ(initialCapacity, static_cast<unsigned>(testSet.capacity()));
    }

    // Adding items up to less than half the capacity should not change the capacity.
    unsigned capacityLimit = initialCapacity / 2 - 1;
    for (size_t i = size; i < capacityLimit; ++i) {
        testSet.add(i);
        ASSERT_EQ(initialCapacity, static_cast<unsigned>(testSet.capacity()));
    }

    // Adding one more item increase the capacity.
    testSet.add(initialCapacity);
    EXPECT_GT(static_cast<unsigned>(testSet.capacity()), initialCapacity);
}

template<unsigned size> void generateTestCapacityUpToSize();
template<> void generateTestCapacityUpToSize<0>()
{
}
template<unsigned size> void generateTestCapacityUpToSize()
{
    generateTestCapacityUpToSize<size - 1>();
    testInitialCapacity<size>();
}

TEST(WTF_HashSet, InitialCapacity)
{
    generateTestCapacityUpToSize<128>();
}

TEST(WTF_HashSet, MoveOnly)
{
    HashSet<MoveOnly> hashSet;

    for (size_t i = 0; i < 100; ++i) {
        MoveOnly moveOnly(i + 1);
        hashSet.add(WTFMove(moveOnly));
    }

    for (size_t i = 0; i < 100; ++i)
        EXPECT_TRUE(hashSet.contains(MoveOnly(i + 1)));

    for (size_t i = 0; i < 100; ++i)
        EXPECT_TRUE(hashSet.remove(MoveOnly(i + 1)));

    EXPECT_TRUE(hashSet.isEmpty());

    for (size_t i = 0; i < 100; ++i)
        hashSet.add(MoveOnly(i + 1));

    for (size_t i = 0; i < 100; ++i)
        EXPECT_TRUE(hashSet.take(MoveOnly(i + 1)) == MoveOnly(i + 1));

    EXPECT_TRUE(hashSet.isEmpty());

    for (size_t i = 0; i < 100; ++i)
        hashSet.add(MoveOnly(i + 1));

    HashSet<MoveOnly> secondSet;

    for (size_t i = 0; i < 100; ++i)
        secondSet.add(hashSet.takeAny());

    EXPECT_TRUE(hashSet.isEmpty());

    for (size_t i = 0; i < 100; ++i)
        EXPECT_TRUE(secondSet.contains(MoveOnly(i + 1)));
}


TEST(WTF_HashSet, UniquePtrKey)
{
    ConstructorDestructorCounter::TestingScope scope;

    HashSet<std::unique_ptr<ConstructorDestructorCounter>> set;

    auto uniquePtr = std::make_unique<ConstructorDestructorCounter>();
    set.add(WTFMove(uniquePtr));

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    set.clear();

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);
}

TEST(WTF_HashSet, UniquePtrKey_FindUsingRawPointer)
{
    HashSet<std::unique_ptr<int>> set;

    auto uniquePtr = std::make_unique<int>(5);
    int* ptr = uniquePtr.get();
    set.add(WTFMove(uniquePtr));

    auto it = set.find(ptr);
    ASSERT_TRUE(it != set.end());
    EXPECT_EQ(ptr, it->get());
    EXPECT_EQ(5, *it->get());
}

TEST(WTF_HashSet, UniquePtrKey_ContainsUsingRawPointer)
{
    HashSet<std::unique_ptr<int>> set;

    auto uniquePtr = std::make_unique<int>(5);
    int* ptr = uniquePtr.get();
    set.add(WTFMove(uniquePtr));

    EXPECT_EQ(true, set.contains(ptr));
}

TEST(WTF_HashSet, UniquePtrKey_RemoveUsingRawPointer)
{
    ConstructorDestructorCounter::TestingScope scope;

    HashSet<std::unique_ptr<ConstructorDestructorCounter>> set;

    auto uniquePtr = std::make_unique<ConstructorDestructorCounter>();
    ConstructorDestructorCounter* ptr = uniquePtr.get();
    set.add(WTFMove(uniquePtr));

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    bool result = set.remove(ptr);
    EXPECT_EQ(true, result);

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);
}

TEST(WTF_HashSet, UniquePtrKey_TakeUsingRawPointer)
{
    ConstructorDestructorCounter::TestingScope scope;

    HashSet<std::unique_ptr<ConstructorDestructorCounter>> set;

    auto uniquePtr = std::make_unique<ConstructorDestructorCounter>();
    ConstructorDestructorCounter* ptr = uniquePtr.get();
    set.add(WTFMove(uniquePtr));

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);

    auto result = set.take(ptr);
    EXPECT_EQ(ptr, result.get());

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(0u, ConstructorDestructorCounter::destructionCount);
    
    result = nullptr;

    EXPECT_EQ(1u, ConstructorDestructorCounter::constructionCount);
    EXPECT_EQ(1u, ConstructorDestructorCounter::destructionCount);
}

TEST(WTF_HashSet, CopyEmpty)
{
    {
        HashSet<unsigned> foo;
        HashSet<unsigned> bar(foo);

        EXPECT_EQ(0u, bar.capacity());
        EXPECT_EQ(0u, bar.size());
    }
    {
        HashSet<unsigned> foo({ 1, 5, 64, 42 });
        EXPECT_EQ(4u, foo.size());
        foo.remove(1);
        foo.remove(5);
        foo.remove(42);
        foo.remove(64);
        HashSet<unsigned> bar(foo);

        EXPECT_EQ(0u, bar.capacity());
        EXPECT_EQ(0u, bar.size());
    }
}

TEST(WTF_HashSet, CopyAllocateAtLeastMinimumCapacity)
{
    HashSet<unsigned> foo({ 42 });
    EXPECT_EQ(1u, foo.size());
    HashSet<unsigned> bar(foo);

    EXPECT_EQ(8u, bar.capacity());
    EXPECT_EQ(1u, bar.size());
}

TEST(WTF_HashSet, CopyCapacityIsNotOnBoundary)
{
    // Starting at 4 because the minimum size is 8.
    // With a size of 8, a medium load can be up to 3.3333->3.
    // Adding 1 to 3 would reach max load.
    // While correct, that's not really what we care about here.
    for (unsigned size = 4; size < 100; ++size) {
        HashSet<unsigned> source;
        for (unsigned i = 1; i < size + 1; ++i)
            source.add(i);

        HashSet<unsigned> copy1(source);
        HashSet<unsigned> copy2(source);
        HashSet<unsigned> copy3(source);

        EXPECT_EQ(size, copy1.size());
        EXPECT_EQ(size, copy2.size());
        EXPECT_EQ(size, copy3.size());
        for (unsigned i = 1; i < size + 1; ++i) {
            EXPECT_TRUE(copy1.contains(i));
            EXPECT_TRUE(copy2.contains(i));
            EXPECT_TRUE(copy3.contains(i));
        }
        EXPECT_FALSE(copy1.contains(size + 2));
        EXPECT_FALSE(copy2.contains(size + 2));
        EXPECT_FALSE(copy3.contains(size + 2));

        EXPECT_TRUE(copy2.remove(1));
        EXPECT_EQ(copy1.capacity(), copy2.capacity());
        EXPECT_FALSE(copy2.contains(1));

        EXPECT_TRUE(copy3.add(size + 2).isNewEntry);
        EXPECT_EQ(copy1.capacity(), copy3.capacity());
        EXPECT_TRUE(copy3.contains(size + 2));
    }
}

TEST(WTF_HashSet, RefPtrNotZeroedBeforeDeref)
{
    struct DerefObserver {
        NEVER_INLINE void ref()
        {
            ++count;
        }
        NEVER_INLINE void deref()
        {
            --count;
            observedBucket = bucketAddress->get();
        }
        unsigned count { 1 };
        const RefPtr<DerefObserver>* bucketAddress { nullptr };
        const DerefObserver* observedBucket { nullptr };
    };

    auto observer = std::make_unique<DerefObserver>();

    HashSet<RefPtr<DerefObserver>> set;
    set.add(adoptRef(observer.get()));

    auto iterator = set.find(observer.get());
    EXPECT_TRUE(iterator != set.end());

    observer->bucketAddress = iterator.get();

    EXPECT_TRUE(observer->observedBucket == nullptr);
    EXPECT_TRUE(set.remove(observer.get()));

    // It if fine to either leave the old value intact at deletion or already set it to the deleted
    // value.
    // A zero would be a incorrect outcome as it would mean we nulled the bucket before an opaque
    // call.
    EXPECT_TRUE(observer->observedBucket == observer.get() || observer->observedBucket == RefPtr<DerefObserver>::hashTableDeletedValue());
    EXPECT_EQ(observer->count, 0u);
}


TEST(WTF_HashSet, UniquePtrNotZeroedBeforeDestructor)
{
    struct DestructorObserver {
        ~DestructorObserver()
        {
            observe();
        }
        std::function<void()> observe;
    };

    const std::unique_ptr<DestructorObserver>* bucketAddress = nullptr;
    const DestructorObserver* observedBucket = nullptr;
    std::unique_ptr<DestructorObserver> observer(new DestructorObserver { [&]() {
        observedBucket = bucketAddress->get();
    }});

    const DestructorObserver* observerAddress = observer.get();

    HashSet<std::unique_ptr<DestructorObserver>> set;
    auto addResult = set.add(WTFMove(observer));

    EXPECT_TRUE(addResult.isNewEntry);
    EXPECT_TRUE(observedBucket == nullptr);

    bucketAddress = addResult.iterator.get();

    EXPECT_TRUE(observedBucket == nullptr);
    EXPECT_TRUE(set.remove(*addResult.iterator));

    EXPECT_TRUE(observedBucket == observerAddress || observedBucket == reinterpret_cast<const DestructorObserver*>(-1));
}


} // namespace TestWebKitAPI
