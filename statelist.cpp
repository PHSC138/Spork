#include "statelist.hpp"
#include <algorithm>

// TODO: Remove this
#include <iostream>

StateList::StateList(std::vector<unsigned> max_values) {
    for(unsigned value : max_values) {
        state.push_back(0);
        max_state.push_back(value);
    }

    iterations = 0;
}

bool StateList::is_done() {
    // Don't return true for initial state
    if (iterations == 0) return false;

    for(unsigned index = 0; index < state.size(); index++) {
        std::cout << "statelist state[" << index << "]: " << state.at(index) << std::endl;
        if (state.at(index) != 0) return false;
    }

    return true;
}

std::vector<unsigned> StateList::get_state() {
    return state;
}

void StateList::next_state() {
    // Increment iterations
    iterations++;

    // Loop backwards through state
    unsigned current;
    for(unsigned i = state.size() - 1; i >= 0 && i < state.size(); i--) {
        // Increment the state of the last element
        current = state.at(i) + 1;
        std::cout << "statelist::next_state(): state.size() - 1 " << state.size() - 1 << " max_state.at(" << i << ") " << max_state.at(i) << " current " << current << std::endl;

        // Check if overflow
        if (current >= max_state.at(i)) {
            current = 0;
            state[i] = current;
        } else { 
            state[i] = current;
            break;
        }
    }

    // Last iteration will set state to [0, 0, 0]
}

unsigned StateList::get_iterations() {
    return iterations;
}