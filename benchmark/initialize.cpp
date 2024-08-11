#include <fstream>
#include <iostream>
#include <random>
#include <set>

const int num_initial_ranges = 10000;
const int num_instant_transactions = 10000;
const int range_size = 100;

void initialize_ranges(const std::string &initial_ranges_file, const std::string &instant_ranges_file) {
    std::ofstream initial_out(initial_ranges_file);
    std::ofstream instant_out(instant_ranges_file);
    std::mt19937 rng(0);
    std::uniform_int_distribution<int> range_dist(1, 49);

    // Generate 1 million ranges: 0-100, 200-300, ...
    for (int i = 0; i < num_initial_ranges; ++i) {
        int start = i * 2 * range_size;
        int end = start + range_size;
        initial_out << start << " " << end << "\n";
    }

    // Generate 100,000 ranges randomly within the gaps: 100-200, 300-400, ...
    for (int i = 0; i < num_instant_transactions; ++i) {
        int gap_start = (i * 2 + 1) * range_size;
        int start = gap_start + range_dist(rng);
        int end = start + 49;
        instant_out << start << " " << end << "\n";
    }
}

int main() {
    initialize_ranges("initial_ranges.txt", "instant_ranges.txt");
    return 0;
}