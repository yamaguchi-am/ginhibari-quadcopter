#include "filter.h"

bool AverageFilter::Get(float* result) {
  if (count_ < window_size_) {
    return false;
  }
  *result = sum_ / window_size_;
  return true;
}

void AverageFilter::Put(float x) {
  sum_ += x;
  if (count_ < window_size_) {
    count_++;
  } else {
    sum_ -= data_[i_];
  }
  data_[i_] = x;
  if (i_ >= window_size_ - 1) {
    i_ = 0;
  } else {
    i_++;
  }
}
