/**
*  USE THIS AS
*  {CSimple_timer t("WHATEVER YOU ARE TIMING");
*        THE CODE YOU ARE TIMING
*    }
* 
*  THEN AT THE END OF MAIN PUT
*  CSimple_timer::print_timing_results();   
*/

#include <iostream>
#include <chrono>
#include <map>
#include <string>
#include <cmath> // For std::sqrt and std::pow
#include <thread> // For std::this_thread::sleep_for

// Using to define time units (you can change milliseconds to something else if needed)
//using time_units = std::chrono::milliseconds;
using time_units = std::chrono::microseconds;

// Struct to store cumulative timings and call counts
struct TimerData {
    int call_count = 0;
    std::chrono::microseconds total_duration = time_units(0);
};

// Global table to store the timings for each string
std::map<std::string, TimerData> table;

class CSimple_timer {
public:
    // Constructor: start the timer and set the operation description
    CSimple_timer(const std::string &what) : time_what(what) {
        start_time = std::chrono::high_resolution_clock::now();
        
        // Either update the existing entry or create a new one
        if (table.find(time_what) == table.end()) {
            table[time_what] = TimerData(); // Insert new entry if not found
        }
        table[time_what].call_count++; // Increment call count
    }

    // Destructor: stop the timer and update the cumulative time
    ~CSimple_timer() {
        auto end_time = std::chrono::high_resolution_clock::now();
        auto duration = std::chrono::duration_cast<time_units>(end_time - start_time);
        
        // Update the total duration in the table  map
        table[time_what].total_duration += duration;

        // Optionally print the result for this instance
        std::cout << "Duration for '" << time_what << "': " << duration.count() << " micro s" << std::endl;
    }//destructor

    // Static method to print all timing results
    static void print_timing_results() {
        std::cout << "\nTiming results:\n";
        for (const auto &entry : table) {
            std::cout << entry.first << " - Total time: " << entry.second.total_duration.count()
                      << " ms, Calls: " << entry.second.call_count << std::endl;
        }
    }

private:
    std::string time_what; // What are we timing
    std::chrono::high_resolution_clock::time_point start_time; // Start time
};
//////////////////
