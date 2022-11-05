//===----------------------------------------------------------------------===//
//
//                         BusTub
//
// hash_util.h
//
// Identification: src/include/common/util/hash_util.h
//
// Copyright (c) 2015-2019, Carnegie Mellon University Database Group
//
//===----------------------------------------------------------------------===//

#pragma once

#include <algorithm>
#include <cstdlib>
#include <iostream>
#include <string>


using hash_t = std::size_t;

class HashUtil {
 private:
  static const hash_t PRIME_FACTOR = 10000019;

 public:
  static inline auto HashBytes(const char *bytes, size_t length) -> hash_t {
    // https://github.com/greenplum-db/gpos/blob/b53c1acd6285de94044ff91fbee91589543feba1/libgpos/src/utils.cpp#L126
    hash_t hash = length;
    for (size_t i = 0; i < length; ++i) {
      hash = ((hash << 5) ^ (hash >> 27)) ^ bytes[i];
    }
    return hash;
  }

  static inline auto CombineHashes(hash_t l, hash_t r) -> hash_t {
    hash_t both[2] = {};
    both[0] = l;
    both[1] = r;
    return HashBytes(reinterpret_cast<char *>(both), sizeof(hash_t) * 2);
  }

  static inline auto SumHashes(hash_t l, hash_t r) -> hash_t {
    return (l % PRIME_FACTOR + r % PRIME_FACTOR) % PRIME_FACTOR;
  }

  template <typename T>
  static inline auto Hash(const T *ptr) -> hash_t {
    return HashBytes(reinterpret_cast<const char *>(ptr), sizeof(T));
  }

  template <typename T>
  static inline auto HashPtr(const T *ptr) -> hash_t {
    return HashBytes(reinterpret_cast<const char *>(&ptr), sizeof(void *));
  }

  /** @return the hash of the value */
  static inline auto HashValue(const TupleCell &key) -> hash_t {
    switch (key.attr_type()) {
      case INTS: {
        auto raw = *reinterpret_cast<const int32_t *>(key.data());
        return Hash<int32_t>(&raw);
      }
      case FLOATS: {
        auto raw = *reinterpret_cast<const float *>(key.data());
        return Hash<float>(&raw);
      }
      case DATES: {
        auto raw = *reinterpret_cast<const int32_t *>(key.data());
        return Hash<int32_t>(&raw);
      }
      case CHARS: {
        auto raw = key.data();
        auto len = key.length();
        return HashBytes(raw, len);
      }case TEXTS: {
        auto raw = key.data();
        auto len = key.length();
        return HashBytes(raw, len);
      } case NULLS: {
        int32_t raw = 0;
        return Hash<int32_t>(&raw);
      }
      default: {
        assert(false);
      }
    }
  }
};
