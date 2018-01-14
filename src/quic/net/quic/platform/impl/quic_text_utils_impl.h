// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_QUIC_PLATFORM_IMPL_QUIC_TEXT_UTILS_IMPL_H_
#define NET_QUIC_PLATFORM_IMPL_QUIC_TEXT_UTILS_IMPL_H_

#include <algorithm>
#include <cstdint>
#include <string>
#include <sstream>
#include <vector>

#include "base/base64.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "net/base/hex_utils.h"
#include "base/strings/string_piece.h"

namespace net {

// google3 implementation of QuicTextUtils.
class QuicTextUtilsImpl {
 public:
  // Returns true of |data| starts with |prefix|, case sensitively.
  static bool StartsWith(base::StringPiece data, base::StringPiece prefix) {
    return base::StartsWith(data, prefix, base::CompareCase::SENSITIVE);
  }

  // Returns true of |data| ends with |suffix|, case insensitively.
  static bool EndsWithIgnoreCase(base::StringPiece data, base::StringPiece suffix) {
    return base::EndsWith(data, suffix, base::CompareCase::INSENSITIVE_ASCII);
  }

  // Returns a new std::string in which |data| has been converted to lower case.
  static std::string ToLower(base::StringPiece data) {
    return base::ToLowerASCII(data);
  }


  // Returns a new std::string representing |in|.
  static std::string Uint64ToString(uint64_t in) {
    return base::NumberToString(in);
  }

  // This converts |length| bytes of binary to a 2*|length|-character
  // hexadecimal representation.
  // Return value: 2*|length| characters of ASCII std::string.
  static std::string HexEncode(base::StringPiece data) {
    return base::ToLowerASCII(::base::HexEncode(data.data(), data.size()));
  }

  static std::string Hex(uint32_t v) {
    std::stringstream ss;
    ss << std::hex << v;
    return ss.str();
  }

 
  // Base64 encodes with no padding |data_len| bytes of |data| into |output|.
  static void Base64Encode(const uint8_t* data,
                           size_t data_len,
                           std::string* output) {
    base::Base64Encode(
        std::string(reinterpret_cast<const char*>(data), data_len), output);
    // Remove padding.
    size_t len = output->size();
    if (len >= 2) {
      if ((*output)[len - 1] == '=') {
        len--;
        if ((*output)[len - 1] == '=') {
          len--;
        }
        output->resize(len);
      }
    }
  }
};

}  // namespace net

#endif  // NET_QUIC_PLATFORM_IMPL_QUIC_TEXT_UTILS_IMPL_H_
