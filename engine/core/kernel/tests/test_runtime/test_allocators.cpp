// test_allocator.cpp
//
// Copyright (c) N-GINN LLC., 2023-2025. All rights reserved.
//
#include <chrono>
#include <limits>
#include <mutex>
#include <unordered_set>

#include "nau/memory/array_allocator.h"
#include "nau/memory/frame_allocator.h"
#include "nau/memory/general_allocator.h"
#include "nau/memory/nau_allocator_wrapper.h"
#include "nau/memory/platform/aligned_allocator_windows.h"
#include "nau/memory/stack_allocator.h"
#include "nau/memory/standard_aliases.h"
#include "nau/memory/string_allocator.h"

namespace nau::test
{

    template <class Ty>
    class MyStrAllocator
    {
    public:
        using value_type = Ty;
        using size_type = size_t;
        using difference_type = ptrdiff_t;

        MyStrAllocator() noexcept
        {
        }
        ~MyStrAllocator() = default;
        template <class _Other>
        constexpr MyStrAllocator(const MyStrAllocator<_Other>&) noexcept
        {
        }
        MyStrAllocator(const MyStrAllocator&) noexcept = default;
        MyStrAllocator& operator=(const MyStrAllocator&) = default;

        value_type* allocate(const size_t _Count)
        {
            return (value_type*)m_allocator.allocate(_Count * sizeof(value_type));
        }

        void deallocate(const value_type* _Ptr, const size_t _Count)
        {
            m_allocator.deallocate((char*)_Ptr, _Count * sizeof(value_type));
        }

        bool operator!=(MyStrAllocator left) const
        {
            return true;
        }

        StringAllocator m_allocator;
        static constexpr size_t _Minimum_asan_allocation_alignment = 8;
    };

    using MyString = std::basic_string<char, std::char_traits<char>, MyStrAllocator<char>>;

    template <class Ty>
    class MyVecAllocator
    {
    public:
        using value_type = Ty;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using allocator = ArrayAllocator<10 * 1024 * 1024>;

        MyVecAllocator() noexcept
        {
        }
        ~MyVecAllocator() = default;
        template <class _Other>
        constexpr MyVecAllocator(const MyVecAllocator<_Other>&) noexcept
        {
        }
        MyVecAllocator(const MyVecAllocator&) noexcept = default;
        MyVecAllocator& operator=(const MyVecAllocator&) = default;

        value_type* allocate(const size_t _Count)
        {
            return (value_type*)allocator::instance().allocate(_Count * sizeof(value_type));
        }

        value_type* allocate(const size_t _Count, const value_type* _Ptr)
        {
            return (value_type*)allocator::instance().reallocate(_Count * sizeof(value_type), _Ptr);
        }

        void deallocate(const value_type* _Ptr, const size_t _Count)
        {
            allocator::instance().deallocate((void*)_Ptr);
        }

        static constexpr size_t _Minimum_asan_allocation_alignment = 8;
    };

    template <class Ty>
    using MyVector = std::vector<Ty, MyVecAllocator<Ty>>;

    template <class TVal>
    class MyFixedSizeAllocator
    {
        static constexpr size_t getBlockSize()
        {
            return sizeof(TVal) > 16 ? sizeof(TVal) : 16;
        }

    public:
        using value_type = TVal;
        using size_type = size_t;
        using difference_type = ptrdiff_t;
        using allocator = FixedBlocksAllocator<getBlockSize()>;

        MyFixedSizeAllocator() noexcept
        {
        }
        ~MyFixedSizeAllocator() = default;

        template <class _Other>
        constexpr MyFixedSizeAllocator(const MyFixedSizeAllocator<_Other>&) noexcept
        {
        }
        MyFixedSizeAllocator(const MyFixedSizeAllocator&) noexcept = default;
        MyFixedSizeAllocator& operator=(const MyFixedSizeAllocator&) = default;

        value_type* allocate(const size_t _Count)
        {
            EXPECT_LT(_Count, getBlockSize());
            return (value_type*)allocator::instance().allocate(_Count);
        }

        void deallocate(const value_type* _Ptr, const size_t _Count)
        {
            allocator::instance().deallocate((char*)_Ptr);
        }

        bool operator!=(MyFixedSizeAllocator left) const
        {
            return true;
        }

        static constexpr size_t _Minimum_asan_allocation_alignment = 8;
    };

    template <class TKey, class TVal>
    using MyMap = std::map<TKey, TVal, std::less<TKey>, MyFixedSizeAllocator<std::pair<const TKey, TVal>>>;

    template <class TKey>
    using MySet = std::set<TKey, std::less<TKey>, MyFixedSizeAllocator<TKey>>;

    TEST(TestAllocator, StringAllocator)
    {
        MyString str = "test < 16";                                                  // < 16
        str = "test < 32  00000000";                                                 //  <32
        str = "test < 64  000000000000000000000000000000";                           //  <64
        str = "test < 128 0000000000000000000000000000000000000000000000000000000";  //  <128
        str = R"#(test < 256
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                0000000000000)#";                                                    //  <256
        str = R"#(test vector
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                0000000000000)#";                                                    //  > 256

        MyString str2 = R"#(test vector
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                000000000000000000000000000000000000
                0000000000000)#";  //  > 256

        EXPECT_EQ(str, str2);
        EXPECT_NE(str.c_str(), str2.c_str());
    }

    TEST(TestAllocator, UniqueNew)
    {
        struct Color
        {
            int r = 0;
            int g = 0;
            int b = 0;
        };

        auto& block_allocator = FixedBlocksAllocator<sizeof(Color) * 2>::instance();
        auto& array_allocator = ArrayAllocator<sizeof(Color)>::instance();
        auto general_Allocator = GeneralAllocator();
        LocalStackAllocator stack_allocator;
        FrameAllocator frame_allocator;
        IFrameAllocator::setFrameAllocator(&frame_allocator);

        std::vector<alloc_unique_ptr<Color>> colors;

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new(block_allocator) Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new(general_Allocator) Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new(array_allocator) Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new(frame_allocator) Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new(stack_allocator.get()) Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new() Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(stack_new Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(frame_new Color{1, 2, 3});

        for (auto i = 0; i < 1000; ++i)
            colors.push_back(unique_new_ Color{1, 2, 3});

        for (auto& color : colors)
        {
            EXPECT_EQ(color->r, 1);
            EXPECT_EQ(color->g, 2);
            EXPECT_EQ(color->b, 3);
        }

        colors.clear();
        EXPECT_TRUE(stack_allocator->isClear());
        EXPECT_TRUE(frame_allocator.prepareFrame());
    }

    TEST(TestAllocator, VectorAllocator)
    {
        MySet<void*> mem_used;
        for (int r = 0; r < 1000; ++r)
        {
            MyVector<int> test;
            for (int i = 0; i < 10000; i++)
            {
                test.push_back(i);
                mem_used.insert(test.data());
            }

            int acc = 0;
            for (auto x : test)
                acc += x;
        }
        EXPECT_EQ(mem_used.size(), 2);
    }

    TEST(TestAllocator, FixedBlockAllocator)
    {
        MyMap<std::string, int> intMap;

        for (int i = 0; i < 100; ++i)
            intMap[std::to_string(i)] = i;

        for (int i = 0; i < 100; ++i)
            EXPECT_EQ(intMap[std::to_string(i)], i);
    }

    template <size_t blockSize, size_t allocSize, size_t aligment>
    bool testBlockAllocator()
    {
        auto ptr = FixedBlocksAllocator<blockSize>::instance().allocateAligned(allocSize, aligment);
        if (!FixedBlocksAllocator<blockSize>::instance().isAligned(ptr))
        {
            return false;
        }
        memset(ptr, 0, allocSize);
        if (!FixedBlocksAllocator<blockSize>::instance().isValid(ptr))
        {
            return false;
        }
        FixedBlocksAllocator<blockSize>::instance().deallocateAligned(ptr);
        return true;
    }

    TEST(TestAllocator, FixedBlockAllocatorAligned)
    {
        for (unsigned i = 0; i < 1024; ++i)
        {
            EXPECT_TRUE((testBlockAllocator<16, 4, 4>()));
            EXPECT_TRUE((testBlockAllocator<32, 8, 4>()));
        }
    }

    namespace
    {
        template <class alignedAllocatorBase>
        class AlignTestAllocator final : public alignedAllocatorBase
        {
        public:
            AlignTestAllocator()
            {
            }

            virtual ~AlignTestAllocator()
            {
            }

        private:
            void* allocate(size_t size) override
            {
                return ::malloc(size);
            }

            void* reallocate(void* ptr, size_t size) override
            {
                return ::realloc(ptr, size);
            }

            void deallocate(void* ptr) override
            {
                ::free(ptr);
            }

            size_t getSize(const void* ptr) const override
            {
                return 0;
            }
        };
    }  // namespace

    template <class Allocator>
    void TestAlignedAllocator()
    {
        Allocator allocator;
        for (size_t aligment = 2; aligment <= 1024; aligment += aligment)
        {
            for (unsigned size = 1; size < 1024; ++size)
            {
                void* ptr = allocator.allocateAligned(size, aligment);
                ASSERT_TRUE(ptr != nullptr);
                ASSERT_TRUE(isAligned(ptr, aligment));
                ASSERT_TRUE(allocator.getSizeAligned(ptr, aligment) == size);
                ASSERT_TRUE(allocator.isAligned(ptr));
                ASSERT_TRUE(allocator.isValid(ptr));
                size += size;
                ptr = allocator.reallocateAligned(ptr, size, aligment);
                ASSERT_TRUE(ptr != nullptr);
                ASSERT_TRUE(isAligned(ptr, aligment));
                ASSERT_TRUE(allocator.getSizeAligned(ptr, aligment) == size);
                ASSERT_TRUE(allocator.isAligned(ptr));
                ASSERT_TRUE(allocator.isValid(ptr));
                allocator.deallocateAligned(ptr);
            }
        }
    }

    TEST(TestAllocator, IAlignedAllocatorDebugTest)
    {
        TestAlignedAllocator<AlignTestAllocator<IAlignedAllocator>>();
        TestAlignedAllocator<AlignTestAllocator<IAlignedAllocatorDebug>>();
        TestAlignedAllocator<AlignTestAllocator<IAlignedAllocatorWindows>>();
    }

    TEST(TestAllocator, MixedAllocators)
    {
        MyMap<MyString, MyVector<MyString>> intMap;

        for (int i = 0; i < 100; ++i)
        {
            for (int j = i; j < 10; ++j)
                intMap[std::to_string(i).c_str()].push_back(MyString(std::to_string(j).c_str()));
        }

        for (int i = 0; i < 100; ++i)
        {
            for (int j = i, k = 0; j < 10; ++j, ++k)
                EXPECT_EQ(intMap[std::to_string(i).c_str()][k], MyString(std::to_string(j).c_str()));
        }
    }

    TEST(TestAllocator, StackAllocator)
    {
        LocalStackAllocator allocator;
        auto test = stack_new std::vector<int>();

        for (int i = 0; i < 10; ++i)
            test->push_back(i);

        for (int i = 0; i < 10; ++i)
            EXPECT_EQ(i, test->at(i));

        test = nullptr;
        EXPECT_TRUE(allocator->isClear());
    }

    TEST(TestAllocator, FrameAllocator)
    {
        {
            FrameAllocator allocator;
            IFrameAllocator::setFrameAllocator(&allocator);

            auto frameTest = frame_new std::vector<int>();

            for (int i = 0; i < 10; ++i)
                frameTest->push_back(i);

            for (int i = 0; i < 10; ++i)
                EXPECT_EQ(i, frameTest->at(i));

            frameTest = nullptr;
            EXPECT_TRUE(allocator.prepareFrame());

            frameTest = frame_new std::vector<int>();
            EXPECT_FALSE(allocator.prepareFrame());
        }
    }

    TEST(TestAllocator, MultiThread)
    {
        StackAllocatorUnnamed;

        MyVector<alloc_unique_ptr<std::thread>> test_threads;
        MySet<size_t> ids;
        MySet<alloc_unique_ptr<MyString>> ids_strings;

        std::hash<std::thread::id> hash;
        std::mutex mutex;

        for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            test_threads.push_back(stack_new std::thread([&, ParentStackAllocator]
            {
                StackAllocatorInheritParent;

                auto test = stack_new MyString(std::to_string(hash(std::this_thread::get_id())).c_str());
                std::lock_guard lock(mutex);
                ids.insert(hash(std::this_thread::get_id()));
                ids_strings.insert(std::move(test));
            }));
        }

        for (auto& th : test_threads)
            th->join();
        test_threads.clear();

        for (auto id : ids)
        {
            bool contain = false;
            for (auto& strId : ids_strings)
            {
                if (*strId == std::to_string(id).c_str())
                {
                    contain = true;
                    break;
                }
            }
            EXPECT_TRUE(contain);
        }

        for (int i = 0; i < std::thread::hardware_concurrency(); ++i)
        {
            test_threads.push_back(stack_new std::thread([&, allocator = IStackAllocator::getStackAllocator()]
            {
                StackAllocatorInherit(*allocator);
                auto test = stack_new MyString(std::to_string(hash(std::this_thread::get_id())).c_str());

                std::lock_guard lock(mutex);
                ids.insert(hash(std::this_thread::get_id()));
                ids_strings.insert(std::move(test));
            }));
        }

        for (auto& th : test_threads)
            th->join();

        for (auto id : ids)
        {
            bool contain = false;
            for (auto& strId : ids_strings)
            {
                if (*strId == std::to_string(id).c_str())
                {
                    contain = true;
                    break;
                }
            }
            EXPECT_TRUE(contain);
        }

        auto allocator = IStackAllocator::getStackAllocator();
        test_threads.clear();
        EXPECT_FALSE(allocator->isClear());

        ids_strings.clear();
        EXPECT_TRUE(allocator->isClear());
    }

    TEST(TestAllocator, ArrayReallocate)
    {
        constexpr size_t BlockSize = 512;
        constexpr size_t BlockSize2 = BlockSize * 2;
        using Allocator = ArrayAllocator<512>;

        auto ptr = Allocator::instance().allocate(BlockSize);
        memset(ptr, 0, BlockSize);

        ptr = Allocator::instance().reallocate(ptr, BlockSize2);
        memset(ptr, 0, BlockSize2);

        Allocator::instance().deallocate(ptr);
    }

    TEST(TestStandardAliasesAllocator, Vector)
    {
        {
            nau::Vector<int> test_vector;
            for (int i = 0; i < 1000; ++i)
                test_vector.push_back(i);

            for (int i = 0; auto val : test_vector)
            {
                EXPECT_EQ(val, i);
                ++i;
            }
        }

        {
            StackAllocatorUnnamed;
            nau::StackVector<int> test_vector;
            for (int i = 0; i < 1000; ++i)
                test_vector.push_back(i);

            for (int i = 0; auto val : test_vector)
            {
                EXPECT_EQ(val, i);
                ++i;
            }
        }

        {
            FrameAllocator allocator;
            IFrameAllocator::setFrameAllocator(&allocator);
            nau::FrameVector<int> test_vector;
            for (int i = 0; i < 1000; ++i)
                test_vector.push_back(i);

            for (int i = 0; auto val : test_vector)
            {
                EXPECT_EQ(val, i);
                ++i;
            }
        }
    }

    TEST(TestStandardAliasesAllocator, Map)
    {
        {
            nau::Map<int, int> test;
            for (int i = 0; i < 1000; ++i)
                test[i] = i;

            for (auto val : test)
            {
                EXPECT_EQ(val.first, val.second);
            }
        }

        {
            StackAllocatorUnnamed;
            nau::StackMap<int, int> test;
            for (int i = 0; i < 1000; ++i)
                test[i] = i;

            for (auto val : test)
            {
                EXPECT_EQ(val.first, val.second);
            }
        }

        {
            FrameAllocator allocator;
            IFrameAllocator::setFrameAllocator(&allocator);
            nau::FrameMap<int, int> test;
            for (int i = 0; i < 1000; ++i)
                test[i] = i;

            for (auto val : test)
            {
                EXPECT_EQ(val.first, val.second);
            }
        }
    }
}  // namespace nau::test