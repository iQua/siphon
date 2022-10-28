//
// Created by Shuhao Liu on 2018-07-13.
//

#ifndef SIPHON_CODING_PARAMETERS_HPP
#define SIPHON_CODING_PARAMETERS_HPP

#include <atomic>


namespace siphon {

class CodingParameters {
 public:
  typedef uint32_t Type;

 public:
  CodingParameters() : params_(0) {
  }

  CodingParameters(uint8_t T, uint8_t B, uint8_t N, uint8_t counter) :
      params_((uint32_t)((counter << 24) + (N << 16) + (B << 8) + T)) {
  }

  CodingParameters(const uint8_t* buf) {
    uint32_t param = *((uint32_t*) buf);
    params_.store(param);
  }

  void incrementCounter() {
    params_.fetch_add(1 << 24);
  }

  uint32_t readEncodedAndIncrement() {
    return params_.fetch_add((uint32_t)(1 << 24));
  }

  uint32_t readEncoded() const {
    return params_.load();
  }

  void get(uint8_t& T, uint8_t& B, uint8_t& N) const {
    uint32_t encoded = params_.load();
    T = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    B = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    N = (uint8_t) (encoded & 0xFF);
  }

  void get(uint8_t& T, uint8_t& B, uint8_t& N, uint8_t& counter) const {
    uint32_t encoded = params_.load();
    T = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    B = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    N = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    counter = (uint8_t) (encoded & 0xFF);
  }

  void set(uint8_t T, uint8_t B, uint8_t N, uint8_t counter = 0) {
    params_.store((uint32_t)(
        (counter << 24) + (N << 16) + (B << 8) + T));
  }

  void set(uint32_t params, bool reset_counter = false) {
    if (reset_counter)
      params_.store(params & ((uint32_t) 0xFFFFFF));
    else
      params_.store(params);
  }

  static uint32_t encode(uint8_t T, uint8_t B, uint8_t N, uint8_t counter) {
    return (uint32_t(counter << 24) + uint32_t(N << 16) + uint32_t(B << 8) + uint32_t(T));
  }

  static void decode(uint32_t encoded,
                     uint8_t& T, uint8_t& B, uint8_t& N, uint8_t& counter) {
    T = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    B = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    N = (uint8_t) (encoded & 0xFF);
    encoded = encoded >> 8;
    counter = (uint8_t) (encoded & 0xFF);
  }

  void serializeTo(uint8_t* buf) const {
    uint32_t* converted_buf = (uint32_t*) buf;
    *converted_buf = params_.load();
  }

  void deserializeFrom(const uint8_t* buf) {
    uint32_t param = *((uint32_t*) buf);
    params_.store(param);
  }

 private:
  std::atomic<uint32_t> params_;
};


}

#endif // SIPHON_CODING_PARAMETERS_HPP
