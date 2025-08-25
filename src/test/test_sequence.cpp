#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_all.hpp>
#include <thread>
#include <chrono>
#include <atomic>
#include <vector>
#include <memory>

#include "sequence.hpp"
#include "display.hpp"

using namespace sequence;
using namespace std::chrono_literals;

namespace display {
    // Simple test display implementation
    class TestDisplay : public Display {
    public:
        TestDisplay() : Display([](){}, [](){}) {}
        void setBrightness(int) override {}
    private:
        void update() override {}
    };
}

TEST_CASE("SequenceManager null pointer crash reproduction", "[sequence][crash]") {
    SECTION("Direct null pointer scenario") {
        // Create a more direct test that simulates the exact crash condition
        auto display = std::make_unique<display::TestDisplay>();
        SequenceManager manager(std::move(display));
        
        // Add a sequence with very short TTL
        DisplayState state;
        state.text = "Expiring soon";
        manager.addSequenceState("expire_me", state, 0.001, 0.002); // 1ms display, 2ms TTL
        
        // Wait for it to expire
        std::this_thread::sleep_for(5ms);
        
        // This should trigger the processSequence path where expired elements are cleaned up
        // and m_current_element could become null - this will crash without the fix
        manager.nextState();
        manager.nextState();
        manager.nextState();
    }
    
    SECTION("Empty sequence with forced processing") {
        auto display = std::make_unique<display::TestDisplay>();
        SequenceManager manager(std::move(display));
        
        // Start with empty sequence and force processing
        // This can cause m_current_element to be null
        manager.nextState();
        manager.nextState();
        
        // Add and immediately clear
        DisplayState state;
        state.text = "Temporary";
        manager.addSequenceState("temp", state, 1.0, 0.0);
        manager.clearSequence();
        
        // Force processing on empty sequence
        manager.nextState();
    }
    
    SECTION("Rapid sequence modifications") {
        auto display = std::make_unique<display::TestDisplay>();
        SequenceManager manager(std::move(display));
        
        // Add multiple sequences with very short lifetimes
        for (int i = 0; i < 10; i++) {
            DisplayState state;
            state.text = "Test " + std::to_string(i);
            manager.addSequenceState("seq_" + std::to_string(i), state, 0.001, 0.002);
        }
        
        // Wait for expiration
        std::this_thread::sleep_for(10ms);
        
        // Force multiple state transitions - this should trigger the crash without the fix
        for (int i = 0; i < 20; i++) {
            manager.nextState();
            std::this_thread::sleep_for(1ms);
        }
    }
}

// Stress test to increase chances of reproducing the race condition
TEST_CASE("SequenceManager stress test for null pointer", "[sequence][stress]") {
    auto display = std::make_unique<display::TestDisplay>();
    SequenceManager manager(std::move(display));
    
    std::atomic<bool> should_stop{false};
    std::atomic<int> operations{0};
    
    // Multiple threads doing different operations simultaneously
    std::vector<std::thread> threads;
    
    // Thread adding sequences
    threads.emplace_back([&]() {
        int counter = 0;
        while (!should_stop.load() && counter < 100) {
            DisplayState state;
            state.text = "Stress " + std::to_string(counter);
            manager.addSequenceState("stress_" + std::to_string(counter), state, 0.001, 0.005);
            operations++;
            counter++;
            std::this_thread::sleep_for(1ms);
        }
    });
    
    // Thread clearing sequences
    threads.emplace_back([&]() {
        int counter = 0;
        while (!should_stop.load() && counter < 50) {
            manager.clearSequence();
            operations++;
            counter++;
            std::this_thread::sleep_for(3ms);
        }
    });
    
    // Thread forcing state transitions
    threads.emplace_back([&]() {
        int counter = 0;
        while (!should_stop.load() && counter < 200) {
            manager.nextState();
            operations++;
            counter++;
            std::this_thread::sleep_for(500us);
        }
    });
    
    // Let it run for a bit
    std::this_thread::sleep_for(200ms);
    should_stop = true;
    
    for (auto& t : threads) {
        t.join();
    }
    
    REQUIRE(operations.load() > 0);
}
