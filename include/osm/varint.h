#pragma once

#include <cinttypes>
#include <iterator>

#include "utl/helpers/algorithm.h"

#include "protozero/varint.hpp"

namespace osm {

template <typename T = std::int64_t, bool Delta = false>
struct varint {
  struct iterator {
    using iterator_category = std::forward_iterator_tag;
    using difference_type = std::ptrdiff_t;
    using value_type = std::int64_t;
    using pointer = std::int64_t*;
    using reference = std::int64_t const&;

    iterator() = default;
    explicit iterator(std::string_view d)
        : data_(d), state_{d.empty() ? kFin : kFirst} {}

    reference operator*() const {
      if (state_ == kFirst) {
        ++(*const_cast<iterator*>(this));
      }
      return value_;
    }

    iterator& operator++() {
      if (state_ == kLast) {
        state_ = kFin;
        return *this;
      }
      if (data_.empty()) {
        state_ = kLast;
        return *this;
      }
      if (state_ == kFirst) {
        state_ = kMid;
      }
      auto start = data_.data();
      auto const end = start + data_.size();
      auto const prev = value_;
      auto const x = protozero::decode_varint(&start, end);
      if constexpr (std::is_signed_v<T>) {
        if constexpr (sizeof(value_type) == 4) {
          value_ = protozero::decode_zigzag32(x);
        } else {
          static_assert(sizeof(value_type) == 8);
          value_ = protozero::decode_zigzag64(x);
        }
      } else {
        value_ = static_cast<T>(x);
      }
      if constexpr (Delta) {
        value_ += prev;
      }
      data_ = {start, end};
      if (data_.empty()) {
        state_ = kLast;
      }
      return *this;
    }

    iterator operator++(int) {
      iterator tmp = *this;
      ++(*this);
      return tmp;
    }

    friend bool operator==(iterator const& a, iterator const& b) {
      return a.state_ == b.state_;
    }

    friend bool operator!=(iterator const& a, iterator const& b) {
      return !(a == b);
    }

    std::string_view data_{};
    value_type value_{0U};
    enum : std::uint8_t { kFirst, kMid, kLast, kFin } state_{kFin};
  };

  iterator begin() const { return iterator{data_}; }
  iterator end() const { return iterator{}; }

  bool empty() const noexcept { return data_.empty(); }

  std::size_t size() const noexcept {
    return utl::count_if(data_, [](char c) noexcept {
      return (static_cast<unsigned char>(c) & 0x80U) == 0U;
    });
  }

  std::string_view data_;
};

template <typename T>
using delta_varint = varint<T, true>;

}  // namespace osm