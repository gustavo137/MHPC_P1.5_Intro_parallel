#include <iostream>
#include <cmath>    // for std::exp and std::abs
#include <omp.h>
#include "Timer.hpp"

// Function to integrate
double f(double x) {
    return 4.0/(1.0+x*x);
}

//Trapezoidal Rule for the iontegral
double trapezoidal_rule(double a, double b, int n) {
    double h = (b - a) / n;  // n is the subintervals, h width
    double sum = 0.5 * (f(a) + f(b));  // 

    //#pragma omp parallel for reduction(+:sum)
    #pragma omp parallel for 
    for (int i = 1; i < n; ++i) {
        double x_i = a + i * h;
        #pragma omp critical
        {
        sum += f(x_i);
        }
    }
    //std::cout<<sum*h<<std::endl;
    return sum * h;
}

int main() {
    double a = 0.0;  // 
    double b = 1.0;   // 
    double exact_value = M_PI;  //Exact result 
   {CSimple_timer t("Integration PI");
    double integral = trapezoidal_rule(a, b, 1000000);
    }
    //double error = std::abs(integral - exact_value);

    //std::cout << "Integral Approx : " << integral << std::endl;
    //std::cout << "Abs error: " << error << std::endl;

    CSimple_timer::print_timing_results();

    return 0;
}
