#include <vector>

class StateList {
public:
    StateList(std::vector<unsigned> max_values);
    bool is_done();
    std::vector<unsigned> get_state();
    void next_state();
    unsigned get_iterations();
private:
    std::vector<unsigned> state;
    std::vector<unsigned> max_state;
    unsigned iterations;
};