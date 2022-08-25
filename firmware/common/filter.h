#ifndef FILTER_H_
#define FILTER_H_

// A moving averager filter.
class AverageFilter {
 public:
  AverageFilter(int window_size) : window_size_(window_size), sum_(0) {
    data_ = new float[window_size];
    i_ = 0;
    count_ = 0;
  }
  bool Get(float* result);
  void Put(float x);

 private:
  const int window_size_;
  float sum_;
  int i_;
  int count_;
  float* data_;
};

#endif  // ifndef FILTER_H_
