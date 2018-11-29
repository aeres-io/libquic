// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_util.h"

#include <ctype.h>
#include <errno.h>
#include <math.h>
#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>

#include <algorithm>
#include <limits>
#include <vector>

#include "base/macros.h"
#include "base/memory/singleton.h"
#include "build/build_config.h"

namespace base {

namespace {

// Force the singleton used by EmptyString[16] to be a unique type. This
// prevents other code that might accidentally use Singleton<string> from
// getting our internal one.
struct EmptyStrings {
  EmptyStrings() = default;
  const std::string s;
  const string16 s16;

  static EmptyStrings* GetInstance() {
    return Singleton<EmptyStrings>::get();
  }
};


// Assuming that a pointer is the size of a "machine word", then
// uintptr_t is an integer type that is also a machine word.
typedef uintptr_t MachineWord;
const uintptr_t kMachineWordAlignmentMask = sizeof(MachineWord) - 1;

inline bool IsAlignedToMachineWord(const void* pointer) {
  return !(reinterpret_cast<MachineWord>(pointer) & kMachineWordAlignmentMask);
}

template<typename T> inline T* AlignToMachineWord(T* pointer) {
  return reinterpret_cast<T*>(reinterpret_cast<MachineWord>(pointer) &
                              ~kMachineWordAlignmentMask);
}

template<size_t size, typename CharacterType> struct NonASCIIMask;
template<> struct NonASCIIMask<4, char16> {
    static inline uint32_t value() { return 0xFF80FF80U; }
};
template<> struct NonASCIIMask<4, char> {
    static inline uint32_t value() { return 0x80808080U; }
};
template<> struct NonASCIIMask<8, char16> {
    static inline uint64_t value() { return 0xFF80FF80FF80FF80ULL; }
};
template<> struct NonASCIIMask<8, char> {
    static inline uint64_t value() { return 0x8080808080808080ULL; }
};
#if defined(WCHAR_T_IS_UTF32)
template<> struct NonASCIIMask<4, wchar_t> {
    static inline uint32_t value() { return 0xFFFFFF80U; }
};
template<> struct NonASCIIMask<8, wchar_t> {
    static inline uint64_t value() { return 0xFFFFFF80FFFFFF80ULL; }
};
#endif  // WCHAR_T_IS_UTF32

}  // namespace


namespace {

template<typename StringType>
StringType ToLowerASCIIImpl(BasicStringPiece<StringType> str) {
  StringType ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++)
    ret.push_back(ToLowerASCII(str[i]));
  return ret;
}

template<typename StringType>
StringType ToUpperASCIIImpl(BasicStringPiece<StringType> str) {
  StringType ret;
  ret.reserve(str.size());
  for (size_t i = 0; i < str.size(); i++)
    ret.push_back(ToUpperASCII(str[i]));
  return ret;
}

}  // namespace

std::string ToLowerASCII(StringPiece str) {
  return ToLowerASCIIImpl<std::string>(str);
}

string16 ToLowerASCII(StringPiece16 str) {
  return ToLowerASCIIImpl<string16>(str);
}


const std::string& EmptyString() {
  return EmptyStrings::GetInstance()->s;
}

const string16& EmptyString16() {
  return EmptyStrings::GetInstance()->s16;
}


template<typename Str>
BasicStringPiece<Str> TrimStringPieceT(BasicStringPiece<Str> input,
                                       BasicStringPiece<Str> trim_chars,
                                       TrimPositions positions) {
  size_t begin = (positions & TRIM_LEADING) ?
      input.find_first_not_of(trim_chars) : 0;
  size_t end = (positions & TRIM_TRAILING) ?
      input.find_last_not_of(trim_chars) + 1 : input.size();
  return input.substr(begin, end - begin);
}

StringPiece TrimString(StringPiece input,
                       const StringPiece& trim_chars,
                       TrimPositions positions) {
  return TrimStringPieceT(input, trim_chars, positions);
}


StringPiece16 TrimWhitespace(StringPiece16 input,
                             TrimPositions positions) {
  return TrimStringPieceT(input, StringPiece16(kWhitespaceUTF16), positions);
}


StringPiece TrimWhitespaceASCII(StringPiece input, TrimPositions positions) {
  return TrimStringPieceT(input, StringPiece(kWhitespaceASCII), positions);
}



template <class Char>
inline bool DoIsStringASCII(const Char* characters, size_t length) {
  MachineWord all_char_bits = 0;
  const Char* end = characters + length;

  // Prologue: align the input.
  while (!IsAlignedToMachineWord(characters) && characters != end) {
    all_char_bits |= *characters;
    ++characters;
  }

  // Compare the values of CPU word size.
  const Char* word_end = AlignToMachineWord(end);
  const size_t loop_increment = sizeof(MachineWord) / sizeof(Char);
  while (characters < word_end) {
    all_char_bits |= *(reinterpret_cast<const MachineWord*>(characters));
    characters += loop_increment;
  }

  // Process the remaining bytes.
  while (characters != end) {
    all_char_bits |= *characters;
    ++characters;
  }

  MachineWord non_ascii_bit_mask =
      NonASCIIMask<sizeof(MachineWord), Char>::value();
  return !(all_char_bits & non_ascii_bit_mask);
}

bool IsStringASCII(const StringPiece& str) {
  return DoIsStringASCII(str.data(), str.length());
}

bool IsStringASCII(const StringPiece16& str) {
  return DoIsStringASCII(str.data(), str.length());
}

bool IsStringASCII(const string16& str) {
  return DoIsStringASCII(str.data(), str.length());
}

#if defined(WCHAR_T_IS_UTF32)
bool IsStringASCII(const std::wstring& str) {
  return DoIsStringASCII(str.data(), str.length());
}
#endif

// Implementation note: Normally this function will be called with a hardcoded
// constant for the lowercase_ascii parameter. Constructing a StringPiece from
// a C constant requires running strlen, so the result will be two passes
// through the buffers, one to file the length of lowercase_ascii, and one to
// compare each letter.
//
// This function could have taken a const char* to avoid this and only do one
// pass through the string. But the strlen is faster than the case-insensitive
// compares and lets us early-exit in the case that the strings are different
// lengths (will often be the case for non-matches). So whether one approach or
// the other will be faster depends on the case.
//
// The hardcoded strings are typically very short so it doesn't matter, and the
// string piece gives additional flexibility for the caller (doesn't have to be
// null terminated) so we choose the StringPiece route.
template<typename Str>
static inline bool DoLowerCaseEqualsASCII(BasicStringPiece<Str> str,
                                          StringPiece lowercase_ascii) {
  if (str.size() != lowercase_ascii.size())
    return false;
  for (size_t i = 0; i < str.size(); i++) {
    if (ToLowerASCII(str[i]) != lowercase_ascii[i])
      return false;
  }
  return true;
}

bool LowerCaseEqualsASCII(StringPiece str, StringPiece lowercase_ascii) {
  return DoLowerCaseEqualsASCII<std::string>(str, lowercase_ascii);
}

bool LowerCaseEqualsASCII(StringPiece16 str, StringPiece lowercase_ascii) {
  return DoLowerCaseEqualsASCII<string16>(str, lowercase_ascii);
}

template<typename Str>
bool StartsWithT(BasicStringPiece<Str> str,
                 BasicStringPiece<Str> search_for,
                 CompareCase case_sensitivity) {
  if (search_for.size() > str.size())
    return false;

  BasicStringPiece<Str> source = str.substr(0, search_for.size());

  switch (case_sensitivity) {
    case CompareCase::SENSITIVE:
      return source == search_for;

    case CompareCase::INSENSITIVE_ASCII:
      return std::equal(
          search_for.begin(), search_for.end(),
          source.begin(),
          CaseInsensitiveCompareASCII<typename Str::value_type>());

    default:
      return false;
  }
}

bool StartsWith(StringPiece str,
                StringPiece search_for,
                CompareCase case_sensitivity) {
  return StartsWithT<std::string>(str, search_for, case_sensitivity);
}

bool StartsWith(StringPiece16 str,
                StringPiece16 search_for,
                CompareCase case_sensitivity) {
  return StartsWithT<string16>(str, search_for, case_sensitivity);
}

template <typename Str>
bool EndsWithT(BasicStringPiece<Str> str,
               BasicStringPiece<Str> search_for,
               CompareCase case_sensitivity) {
  if (search_for.size() > str.size())
    return false;

  BasicStringPiece<Str> source = str.substr(str.size() - search_for.size(),
                                            search_for.size());

  switch (case_sensitivity) {
    case CompareCase::SENSITIVE:
      return source == search_for;

    case CompareCase::INSENSITIVE_ASCII:
      return std::equal(
          source.begin(), source.end(),
          search_for.begin(),
          CaseInsensitiveCompareASCII<typename Str::value_type>());

    default:
      return false;
  }
}

bool EndsWith(StringPiece str,
              StringPiece search_for,
              CompareCase case_sensitivity) {
  return EndsWithT<std::string>(str, search_for, case_sensitivity);
}

bool EndsWith(StringPiece16 str,
              StringPiece16 search_for,
              CompareCase case_sensitivity) {
  return EndsWithT<string16>(str, search_for, case_sensitivity);
}

static const char* const kByteStringsUnlocalized[] = {
  " B",
  " kB",
  " MB",
  " GB",
  " TB",
  " PB"
};

template <class string_type>
inline typename string_type::value_type* WriteIntoT(string_type* str,
                                                    size_t length_with_null) {
  str->reserve(length_with_null);
  str->resize(length_with_null - 1);
  return &((*str)[0]);
}

char* WriteInto(std::string* str, size_t length_with_null) {
  return WriteIntoT(str, length_with_null);
}

wchar_t* WriteInto(std::wstring* str, size_t length_with_null) {
  return WriteIntoT(str, length_with_null);
}

}  // namespace base
