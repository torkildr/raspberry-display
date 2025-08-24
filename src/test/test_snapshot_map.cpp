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

TEST_CASE("iterator handles erased keys gracefully", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);
    map.insert_or_assign("c", 3);

    std::map<std::string, int> actual;

    auto snap = map.snapshot();
    auto it = snap.begin();
    
    // Erase the current key that iterator is pointing to
    map.erase("b");
    
    // Iterator should automatically advance to next valid key
    for (; it != snap.end(); ++it) {
        auto [k, v] = *it;
        actual[k] = v;
    }

    verify_snapshot_results(actual, {
        {"a", 1},
        {"c", 3}
    });
}

TEST_CASE("iterator handles multiple erased keys during iteration", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);
    map.insert_or_assign("c", 3);
    map.insert_or_assign("d", 4);

    std::map<std::string, int> actual;

    auto snap = map.snapshot();
    for (auto [k, v] : snap) {
        // Erase keys during iteration to test iterator resilience
        if (k == "a") {
            map.erase("b"); // erase next key
            map.erase("c"); // erase key after next
        }
        actual[k] = v;
    }

    // Should only get 'a' and 'd' since b and c were erased
    verify_snapshot_results(actual, {
        {"a", 1},
        {"d", 4}
    });
}

TEST_CASE("iterator handles all keys erased during iteration", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);

    std::map<std::string, int> actual;

    auto snap = map.snapshot();
    
    // Erase all keys before iteration starts
    map.erase("a");
    map.erase("b");
    
    // Iterator should skip all invalid keys
    for (auto [k, v] : snap) {
        actual[k] = v;
    }

    // Should get no results since all keys were erased
    REQUIRE(actual.empty());
}

TEST_CASE("iterator segfault reproduction - advance_to_valid beyond bounds", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("a", 1);
    map.insert_or_assign("b", 2);
    map.insert_or_assign("c", 3);

    auto snap = map.snapshot();
    auto it = snap.begin();
    
    // Erase all keys to force advance_to_valid to go beyond bounds
    map.erase("a");
    map.erase("b");
    map.erase("c");
    
    // Test that dereferencing throws instead of segfaulting
    REQUIRE_THROWS(*it);

    // Test that we can iterate safely without segfault
    std::map<std::string, int> actual;
    for (auto [k, v] : snap) {
        actual[k] = v;
    }
    REQUIRE(actual.empty());
}

TEST_CASE("first element goes away", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("seq1", 100);
    map.insert_or_assign("seq2", 200);

    auto snap = map.snapshot();

    map.erase("seq1");
    
    auto it = snap.begin();   
    REQUIRE(it != snap.end());

    auto current_iterator = *it;
    REQUIRE(current_iterator.first == "seq2");
    REQUIRE(current_iterator.second == 200);
}

TEST_CASE("snapshot iterator moves, but begin is frozen in time", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("seq1", 100);
    map.insert_or_assign("seq2", 200);

    auto snap = map.snapshot();
    auto it = snap.begin();
    
    REQUIRE(it != snap.end());
    REQUIRE((*it).first == "seq1");
    
    map.erase("seq1");

    auto current_iterator = snap.begin();
    REQUIRE(current_iterator != snap.end());
    REQUIRE((*current_iterator).first == "seq2");
}

TEST_CASE("first and only element goes away", "[snapshot_map]") {
    SnapshotMap<int> map;
    map.insert_or_assign("seq1", 100);

    auto snap = map.snapshot();
    

    map.erase("seq1");
    auto it = snap.begin();
    REQUIRE(it == snap.end());
    
    REQUIRE_THROWS(*it);
}
