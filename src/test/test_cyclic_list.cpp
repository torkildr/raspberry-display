#include <catch2/catch_test_macros.hpp>
#include <string>
#include <thread>
#include <vector>
#include <chrono>

#include "cyclic_list.hpp"

struct TestData {
    std::string value;
    int number;
    
    TestData(const std::string& v, int n) : value(v), number(n) {}
    TestData() : value(""), number(0) {}
};

TEST_CASE("CyclicList basic operations", "[cyclic_list]") {
    CyclicList<std::string, TestData> list;
    
    SECTION("Empty list") {
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);
        REQUIRE(list.first() == nullptr);
    }
    
    SECTION("Single element") {
        auto elem = list.insert("test1", TestData("hello", 42));
        
        REQUIRE(!list.empty());
        REQUIRE(list.size() == 1);
        REQUIRE(list.first() != nullptr);
        REQUIRE(list.first()->getId() == "test1");
        REQUIRE(list.first()->getData().value == "hello");
        REQUIRE(list.first()->getData().number == 42);
        
        // Test cyclic behavior - next should point to itself
        REQUIRE(elem->next() == elem);
    }
    
    SECTION("Last element deleted") {
        auto elem = list.insert("test1", TestData("hello", 42));
        
        REQUIRE(!list.empty());
        REQUIRE(list.size() == 1);

        REQUIRE(list.first() != nullptr);
        REQUIRE(list.first()->getId() == "test1");

        auto first = list.first();
        list.erase("test1");

        REQUIRE(first != nullptr);
        REQUIRE(first->getId() == "test1");

        REQUIRE(first->next() == nullptr);
    }
    
    SECTION("Multiple elements - sorted order") {
        list.insert("c", TestData("third", 3));
        list.insert("a", TestData("first", 1));
        list.insert("b", TestData("second", 2));
        
        REQUIRE(list.size() == 3);
        
        auto first = list.first();
        REQUIRE(first->getId() == "a");
        
        auto second = first->next();
        REQUIRE(second->getId() == "b");
        
        auto third = second->next();
        REQUIRE(third->getId() == "c");
        
        // Test cyclic behavior
        REQUIRE(third->next() == first);
    }
    
    SECTION("Ever changing, order should be correct") {
        list.insert("a", TestData("alfa", 1));
        list.insert("b", TestData("bravo", 2));
        
        REQUIRE(list.size() == 2);
        
        auto first = list.first();
        REQUIRE(first->getId() == "a");
        
        list.insert("a", TestData("alfa v2", 2));
        list.insert("b", TestData("bravo v2", 3));
        list.insert("c", TestData("gamma", 3));
        
        auto second = first->next();
        REQUIRE(second->getId() == "b");
        REQUIRE(second->getData().value == "bravo v2");
        
        auto third = second->next();
        REQUIRE(third->getId() == "c");

        list.insert("d", TestData("delta", 4));
        auto fourth = third->next();
        REQUIRE(fourth->getId() == "d");

        // Test cyclic behavior
        REQUIRE(fourth->next() == first);
    }
    
    SECTION("Insert or replace") {
        list.insert("test", TestData("original", 1));
        REQUIRE(list.size() == 1);
        REQUIRE(list.first()->getData().value == "original");
        
        // Insert with same ID should replace
        list.insert("test", TestData("updated", 2));
        REQUIRE(list.size() == 1);
        REQUIRE(list.first()->getData().value == "updated");
        REQUIRE(list.first()->getData().number == 2);
    }
    
    SECTION("Erase elements") {
        list.insert("a", TestData("first", 1));
        list.insert("b", TestData("second", 2));
        list.insert("c", TestData("third", 3));
        
        REQUIRE(list.size() == 3);
        
        // Erase middle element
        REQUIRE(list.erase("b"));
        REQUIRE(list.size() == 2);
        
        auto first = list.first();
        REQUIRE(first->getId() == "a");
        REQUIRE(first->next()->getId() == "c");
        REQUIRE(first->next()->next() == first);
        
        // Erase non-existent element
        REQUIRE(!list.erase("nonexistent"));
        REQUIRE(list.size() == 2);
    }
    
    SECTION("Clear list") {
        list.insert("a", TestData("first", 1));
        list.insert("b", TestData("second", 2));
        
        REQUIRE(list.size() == 2);
        
        list.clear();
        
        REQUIRE(list.empty());
        REQUIRE(list.size() == 0);
        REQUIRE(list.first() == nullptr);
    }
    
    SECTION("Find elements") {
        list.insert("a", TestData("first", 1));
        list.insert("b", TestData("second", 2));
        list.insert("c", TestData("third", 3));
        
        auto found = list.find("b");
        REQUIRE(found != nullptr);
        REQUIRE(found->getId() == "b");
        REQUIRE(found->getData().value == "second");
        
        auto not_found = list.find("nonexistent");
        REQUIRE(not_found == nullptr);
    }
    
    SECTION("forEach iteration") {
        list.insert("c", TestData("third", 3));
        list.insert("a", TestData("first", 1));
        list.insert("b", TestData("second", 2));
        
        std::vector<std::string> ids;
        std::vector<std::string> values;
        
        list.forEach([&](const std::string& id, const TestData& data) {
            ids.push_back(id);
            values.push_back(data.value);
        });
        
        REQUIRE(ids.size() == 3);
        REQUIRE(ids[0] == "a");
        REQUIRE(ids[1] == "b");
        REQUIRE(ids[2] == "c");
        
        REQUIRE(values[0] == "first");
        REQUIRE(values[1] == "second");
        REQUIRE(values[2] == "third");
    }
}

TEST_CASE("CyclicList thread safety", "[cyclic_list][thread_safety]") {
    CyclicList<std::string, TestData> list;
    
    SECTION("Concurrent insertions") {
        const int num_threads = 4;
        const int items_per_thread = 10;
        std::vector<std::thread> threads;
        
        for (int t = 0; t < num_threads; ++t) {
            threads.emplace_back([&list, t]() {
                for (int i = 0; i < items_per_thread; ++i) {
                    std::string id = "thread" + std::to_string(t) + "_item" + std::to_string(i);
                    list.insert(id, TestData("value" + std::to_string(i), t * 100 + i));
                }
            });
        }
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        REQUIRE(list.size() == num_threads * items_per_thread);
    }
    
    SECTION("Concurrent read/write operations") {
        // Pre-populate list
        for (int i = 0; i < 10; ++i) {
            list.insert("item" + std::to_string(i), TestData("value" + std::to_string(i), i));
        }
        
        std::atomic<bool> stop_flag{false};
        std::vector<std::thread> threads;
        
        // Reader thread
        threads.emplace_back([&list, &stop_flag]() {
            while (!stop_flag.load()) {
                list.forEach([](const std::string&, const TestData&) {
                    // Just iterate
                });
                std::this_thread::sleep_for(std::chrono::milliseconds(1));
            }
        });
        
        // Writer thread
        threads.emplace_back([&list, &stop_flag]() {
            int counter = 100;
            while (!stop_flag.load()) {
                list.insert("dynamic" + std::to_string(counter), TestData("dynamic", counter));
                counter++;
                std::this_thread::sleep_for(std::chrono::milliseconds(2));
            }
        });
        
        // Let them run for a short time
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        stop_flag = true;
        
        for (auto& thread : threads) {
            thread.join();
        }
        
        // Should still be in a valid state
        REQUIRE(list.size() > 10);
    }
}

TEST_CASE("CyclicList element modification", "[cyclic_list]") {
    CyclicList<std::string, TestData> list;
    
    SECTION("Modify element data") {
        auto elem = list.insert("test", TestData("original", 42));
        
        REQUIRE(elem->getData().value == "original");
        REQUIRE(elem->getData().number == 42);
        
        // Modify the data
        elem->setData(TestData("modified", 99));
        
        REQUIRE(elem->getData().value == "modified");
        REQUIRE(elem->getData().number == 99);
        
        // Verify through list access too
        auto found = list.find("test");
        REQUIRE(found->getData().value == "modified");
        REQUIRE(found->getData().number == 99);
    }
}
