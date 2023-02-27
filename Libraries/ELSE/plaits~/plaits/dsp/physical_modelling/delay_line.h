// Copyright 2014 Olivier Gillet.
//
// Author: Olivier Gillet (ol.gillet@gmail.com)
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
// 
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
// 
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
// 
// See http://creativecommons.org/licenses/MIT/ for more information.
//
// -----------------------------------------------------------------------------
//
// Delay line (same implementation as from stmlib, but does not own its buffer).

#ifndef PLAITS_DSP_PHYSICAL_MODELLING_DELAY_LINE_H_
#define PLAITS_DSP_PHYSICAL_MODELLING_DELAY_LINE_H_

#include <algorithm>

#include "stmlib/dsp/dsp.h"

namespace plaits {

template<typename T, size_t max_delay>
class DelayLine {
 public:
  DelayLine() { }
  ~DelayLine() { }
  
  void Init(float* buffer) {
    line_ = buffer;
    Reset();
  }
  
  void Reset() {
    std::fill(&line_[0], &line_[max_delay], T(0));
    write_ptr_ = 0;
  }
  
  inline void Write(const T sample) {
    line_[write_ptr_] = sample;
    write_ptr_ = (write_ptr_ - 1 + max_delay) % max_delay;
  }
  
  inline const T Allpass(const T sample, size_t delay, const T coefficient) {
    T read = line_[(write_ptr_ + delay) % max_delay];
    T write = sample + coefficient * read;
    Write(write);
    return -write * coefficient + read;
  }

  inline const T WriteRead(const T sample, float delay) {
    Write(sample);
    return Read(delay);
  }
  
  inline const T Read(float delay) const {
    MAKE_INTEGRAL_FRACTIONAL(delay)
    const T a = line_[(write_ptr_ + delay_integral) % max_delay];
    const T b = line_[(write_ptr_ + delay_integral + 1) % max_delay];
    return a + (b - a) * T(delay_fractional);
  }
  
  inline const T ReadHermite(float delay) const {
    MAKE_INTEGRAL_FRACTIONAL(delay)
    int32_t t = (write_ptr_ + delay_integral + max_delay);
    const T xm1 = line_[(t - 1) % max_delay];
    const T x0 = line_[(t) % max_delay];
    const T x1 = line_[(t + 1) % max_delay];
    const T x2 = line_[(t + 2) % max_delay];
    const T c = (x1 - xm1) * 0.5f;
    const T v = x0 - x1;
    const T w = c + v;
    const T a = w + v + (x2 - x0) * 0.5f;
    const T b_neg = w + a;
    const T f = delay_fractional;
    return (((a * f) - b_neg) * f + c) * f + x0;
  }

 private:
  size_t write_ptr_;
  T* line_;
  
  DISALLOW_COPY_AND_ASSIGN(DelayLine);
};

}  // namespace plaits

#endif  // PLAITS_DSP_PHYSICAL_MODELLING_DELAY_LINE_H_
