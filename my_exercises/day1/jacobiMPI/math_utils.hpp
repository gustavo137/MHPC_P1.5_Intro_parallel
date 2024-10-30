#ifndef MATH_UTILS_H
#define MATH_UTILS_H

#include <iostream>
#include <vector>
#include <stdexcept>

template <typename T>
size_t max_element(const std::vector<T>& v) {
  if (v.empty()) {
    throw std::invalid_argument("Vector is empty");
  }
  T max = v[0];
  for (size_t i = 1; i < v.size(); i++) {
    if (v[i] > max) {
      max = v[i];
    }
  }
  return max;
}

template <typename T>
T min_element(const std::vector<T>& v) {
  if (v.empty()) {
    throw std::invalid_argument("Vector is empty");
  }
  T min = v[0];
  for (size_t i = 1; i < v.size(); i++) {
    if (v[i] < min) {
        min = v[i];
    }
  }
  return min;
}

template <typename T>
T accumulate_elements(const std::vector<T>& v, T base_value){
  if (v.empty()) {
    throw std::invalid_argument("Vector is empty");
  }
  T sum = base_value;
  for (size_t i = 0; i < v.size(); i++) {
    sum += v[i];
  }
  return sum;
}

template <typename T>
double average(const std::vector<T>& v) {
  return accumulate_elements<T>(v, 0) / static_cast<double>(v.size());
}

#endif
