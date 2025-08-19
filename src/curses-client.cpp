#include <curses.h>
#include <optional>
#include <memory>
#include <nlohmann/json.hpp>

#include "display_impl.hpp"
#include "transition.hpp"
#include "sequence.hpp"
#include "log_util.hpp"

using json = nlohmann::json;

static void init_curses() {
    initscr();
    timeout(0);
    cbreak();
    noecho();
    halfdelay(10);
    
    nonl();
    intrflush(stdscr, false);
    keypad(stdscr, true);
}

static void print_help_text(const sequence::SequenceManager* seq_mgr) {
    addstr("\nt: time");
    addstr("\na: supported characters");
    addstr("\nb: time + long text");
    addstr("\n0: reset scroll offset");
    addstr("\ns: toggle scrolling enabled/disabled");
    addstr("\n+/-: change brightness");
    addstr("\nc: toggle center/left alignment");
    addstr("\nC: clear sequence");
    addstr("\n\nTransitions:");
    addstr("\n1: wipe left    2: wipe right");
    addstr("\n3: dissolve     4: scroll up");
    addstr("\n5: scroll down  6: split center");
    addstr("\n7: split sides  8: random");
    
    // Sequence Manager State Information
    addstr("\n\n=== Sequence Manager State ===");
    
    if (seq_mgr && seq_mgr->isActive()) {
        // Show sequence count and current index
        std::string seq_info = "\nSequence count: " + std::to_string(seq_mgr->getSequenceCount());
        seq_info += "  Current index: " + std::to_string(seq_mgr->getCurrentSequenceIndex());
        addstr(seq_info.c_str());
        
        // Show current sequence ID
        std::string current_id = seq_mgr->getCurrentSequenceId();
        if (!current_id.empty()) {
            std::string current_info = "\nCurrent ID: " + current_id;
            addstr(current_info.c_str());
        }
        
        // Show all active sequence IDs
        auto active_ids = seq_mgr->getActiveSequenceIds();
        if (!active_ids.empty()) {
            addstr("\nActive IDs: ");
            for (size_t i = 0; i < active_ids.size(); ++i) {
                if (i > 0) addstr(", ");
                addstr(active_ids[i].c_str());
            }
        }
    } else {
        addstr("\nSequence Manager: INACTIVE");
    }
    
    addstr("\n\nq: exit");
    refresh();
}

std::string abc_string = "!\"#$%&'()*+,-./0123456789:;<=>?@ AB°CDEFGHIJKLMNOPQRSTUVWXYZ[\\]^_`abcdefghijklmnopqrstuvwxyz{|}~\xe6\xf8\xe5\xc6\xd8\xc5";

int main() {
    sequence::SequenceManager* sequence_manager_ptr = nullptr;
    init_curses();
    
    auto preUpdate = [] { clear(); };
    auto postUpdate = [&sequence_manager_ptr] { 
        if (sequence_manager_ptr) {
            print_help_text(sequence_manager_ptr);
        }
        refresh(); 
    };
    
    // Display state callback for sequence manager
    auto displayStateCallback = [](const std::string&, const std::string&, int) {
        // Empty callback for curses client
    };

    auto scrollCompleteCallback = [&sequence_manager_ptr] { 
        DEBUG_LOG("Scroll complete");
        sequence_manager_ptr->nextState();
    };
    
    auto display = std::make_unique<display::DisplayImpl>(preUpdate, postUpdate, displayStateCallback, scrollCompleteCallback);
    auto sequence_manager = std::make_shared<sequence::SequenceManager>(
        std::move(display)
    );
    
    // Now set the pointer for the callback
    sequence_manager_ptr = sequence_manager.get();
    
    bool done = false;

    int brightness = 7;
    display::Alignment currentAlignment = display::Alignment::LEFT;
    display::Scrolling currentScrolling = display::Scrolling::ENABLED;
    
    // Configure global display settings through SequenceManager
    sequence_manager->setBrightness(brightness);
    sequence_manager->setScrolling(currentScrolling);
    
    int count = 0;
    while (!done) {
        int c;
        
        if ((c = getch()) != ERR) {
            switch (c) {
                case 'q':
                    done = true;
                    break;
                case 't': {
                    sequence::DisplayState state;
                    state.time_format = "";
                    sequence_manager->clearSequence();
                    sequence_manager->addSequenceState(state, 30.0, 30.0, "display_set");
                    break;
                }
                case 'a': {
                    sequence::DisplayState state;
                    state.text = abc_string;
                    sequence_manager->clearSequence();
                    sequence_manager->addSequenceState(state, 30.0, 30.0, "display_set");
                    break;
                }
                case 'b': {
                    sequence::DisplayState state;
                    state.text = "This is a rather long string. It will have to be scrolled.";
                    state.time_format = "";
                    sequence_manager->clearSequence();
                    sequence_manager->addSequenceState(state, 30.0, 30.0, "display_set");
                    break;
                }
                case 's':
                    if (currentScrolling == display::Scrolling::ENABLED) {
                        currentScrolling = display::Scrolling::DISABLED;
                    } else {
                        currentScrolling = display::Scrolling::ENABLED;
                    }
                    sequence_manager->setScrolling(currentScrolling);
                    break;
                case '0':
                case KEY_HOME:
                    sequence_manager->setScrolling(display::Scrolling::RESET);
                    break;
                case '+':
                    if (brightness < 0xF) {
                        sequence_manager->setBrightness(++brightness);
                    }
                    break;
                case '-':
                    if (brightness > 1) {
                        sequence_manager->setBrightness(--brightness);
                    }
                    break;
                case 'c':
                    if (currentAlignment == display::Alignment::LEFT) {
                        currentAlignment = display::Alignment::CENTER;
                    } else {
                        currentAlignment = display::Alignment::LEFT;
                    }
                    sequence_manager->setAlignment(currentAlignment);
                    break;
                
                // Transition demonstrations - now using DisplayState objects!
                case 'C':
                    sequence_manager->clearSequence(true);
                    break;
                case '1': {
                    sequence::DisplayState state;
                    state.text = "Lorem ipsum dolor sit amet" + std::to_string(count++);
                    state.transition_type = transition::Type::WIPE_LEFT;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo1");
                    break;
                }
                case '2': {
                    sequence::DisplayState state;
                    state.text = "consectetur adipiscing elit";
                    state.transition_type = transition::Type::WIPE_RIGHT;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo2");
                    break;
                }
                case '3': {
                    sequence::DisplayState state;
                    state.text = "sed do eiusmod tempor incididunt ut labore et dolore magna aliqua";
                    state.transition_type = transition::Type::DISSOLVE;
                    state.transition_duration = 2.0;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo3");
                    break;
                }
                case '4': {
                    sequence::DisplayState state;
                    state.text = "ut enim ad minim veniam";
                    state.time_format = "";
                    state.transition_type = transition::Type::SCROLL_UP;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo4");
                    break;
                }
                case '5': {
                    sequence::DisplayState state;
                    state.text = "quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat";
                    state.time_format = "";
                    state.transition_type = transition::Type::SCROLL_DOWN;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo5");
                    break;
                }
                case '6': {
                    sequence::DisplayState state;
                    state.text = "duis aute irure dolor";
                    state.time_format = "";
                    state.transition_type = transition::Type::SPLIT_CENTER;
                    state.transition_duration = 1.2;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo6");
                    break;
                }
                case '7': {
                    sequence::DisplayState state;
                    state.text = "culpa qui officia deserunt mollit";
                    state.transition_type = transition::Type::SPLIT_SIDES;
                    state.transition_duration = 1.2;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo7");
                    break;
                }
                case '8': {
                    sequence::DisplayState state;
                    state.text = "labore et dolore magna aliqua";
                    state.time_format = "";
                    state.transition_type = transition::Type::RANDOM;
                    sequence_manager->addSequenceState(state, 3.0, 10.0, "demo8");
                    break;
                }
            }
        }
    }
    
    // SequenceManager handles display lifecycle
    endwin();
    
    // Disable file logging to restore normal console output
    debug::Logger::disableFileLogging();
    
    return EXIT_SUCCESS;
}