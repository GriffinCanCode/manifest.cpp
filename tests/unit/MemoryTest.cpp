#include "../utils/TestUtils.hpp"
#include "../../src/core/memory/Pool.hpp"

using namespace Manifest::Core::Memory;
using namespace Manifest::Test;

class MemoryTest : public DeterministicTest {};

TEST_F(MemoryTest, BasicPoolOperations) {
    Pool<int> pool(64);
    
    EXPECT_EQ(pool.capacity(), 64u);
    EXPECT_EQ(pool.used(), 0u);
    EXPECT_DOUBLE_EQ(pool.usage_ratio(), 0.0);
    
    int* obj = pool.acquire(42);
    EXPECT_NE(obj, nullptr);
    EXPECT_EQ(*obj, 42);
    EXPECT_EQ(pool.used(), 1u);
    
    pool.release(obj);
    EXPECT_EQ(pool.used(), 0u);
}

// Placeholder - would be expanded with comprehensive memory tests
