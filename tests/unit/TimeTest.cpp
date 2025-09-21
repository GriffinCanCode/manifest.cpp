#include "../utils/TestUtils.hpp"
#include "../../src/core/time/Time.hpp"

using namespace Manifest::Core::Time;
using namespace Manifest::Test;

class TimeTest : public DeterministicTest {};

TEST_F(TimeTest, GameTimeBasics) {
    GameTime game_time;
    
    EXPECT_EQ(game_time.current_turn(), 0u);
    EXPECT_EQ(game_time.current_year(), 1);
    EXPECT_FALSE(game_time.is_paused());
    
    game_time.advance_turn();
    EXPECT_EQ(game_time.current_turn(), 1u);
}

// Placeholder - would be expanded with comprehensive time tests