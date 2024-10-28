#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>

#include "gtest/gtest.h"
//g++ name.cpp -o name.x -lgtest
// to do hardcodear un elemento de cada file to see if its different from cero and 
//Dim of the matrix
const size_t dim = 100;

// function to sabe the data in a vector 
std::vector<double> my_data_in_vector(const std::string& filename){
    const size_t total_size = (dim + 2) * (dim + 2); // including the bondari conditions

    // Vector to put the data 
    std::vector<double> m1(total_size);

    // open file
    std::ifstream file(filename);
    if (!file) {
        std::cerr << "Error opening the file." << std::endl;
       // return 1;
    }

    // Read the data
    std::string line;
    size_t index = 0;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        double value;
        while (iss >> value) {
            if (index < total_size) {
                m1[index++] = value;
            }
        }
    }

    file.close();
 return m1;
}

//here we are checking if the vectors are the same
TEST(FunctionTest, ChekingOutput) {

    // Act: Call the function.
    std::vector<double> m1 = my_data_in_vector("10.datjacobiParallel");
    std::vector<double> m2 = my_data_in_vector("10.datjacobiSerial");

    // Assert: Check the result.
    EXPECT_EQ(m1,m2);
}

int main(int argc, char **argv) {
 ::testing::InitGoogleTest(&argc, argv);
 return RUN_ALL_TESTS();
 
 return 0;
}

