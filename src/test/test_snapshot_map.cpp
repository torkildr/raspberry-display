#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include "snapshot_map.hpp"

#include <catch2/catch_all.hpp>

static void verify_snapshot_results(const std::map<std::string, int>& actual, 
                                   const std::vector<std::pair<std::string, int>>& expected) {
    std::vector<std::string> actual_keys;
    for (const auto& [key, value] : actual) {
        actual_keys.push_back(key);
    }
    
    std::vector<std::string> expected_keys;
    for (const auto& [key, value] : expected) {
        expected_keys.push_back(key);
    }
    
    REQUIRE(actual_keys == expected_keys);
    
    for (const auto& [key, expected_value] : expected) {
        REQUIRE(actual.at(key) == expected_value);
    }
}

TEST_CASE("updating key while iterating", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);

    std::map<std::string, int> actual;

    auto snap = map.snapshot();
    for (auto [k, v] : snap) {
        if (k == "a") {
            map.insert_or_assign("b", 12);
        }
        if (k == "b") {
            map.insert_or_assign("b", 42);
        }
        actual[k] = v;
    }

    verify_snapshot_results(actual, {
        {"a", 1},
        {"b", 12}
    });
}

TEST_CASE("adding key while iterating", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);

    std::map<std::string, int> actual;

    auto snap = map.snapshot();
    for (auto [k, v] : snap) {
        if (k == "b") {
            map.insert_or_assign("c", 3); // update
        }
        actual[k] = v;
    }

    verify_snapshot_results(actual, {
        {"a", 1},
        {"b", 2}
    });
}

TEST_CASE("deleting key while iterating", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);

    std::map<std::string, int> actual;

    auto snap = map.snapshot();
    for (auto [k, v] : snap) {
        if (k == "a") {
            map.erase("b"); // removal
        }
        actual[k] = v;
    }

    verify_snapshot_results(actual, {
        {"a", 1}
    });
}
