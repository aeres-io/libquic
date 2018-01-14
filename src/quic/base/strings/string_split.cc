// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/strings/string_split.h"

#include <stddef.h>

#include "base/strings/string_util.h"

namespace base {

namespace {

// PieceToOutputType converts a StringPiece as needed to a given output type,
// which is either the same type of StringPiece (a NOP) or the corresponding
// non-piece string type.
//
// The default converter is a NOP, it works when the OutputType is the
// correct StringPiece.
template<typename Str, typename OutputType>
OutputType PieceToOutputType(BasicStringPiece<Str> piece) {
  return piece;
}
template<>  // Convert StringPiece to std::string
std::string PieceToOutputType<std::string, std::string>(StringPiece piece) {
  return piece.as_string();
}
template<>  // Convert StringPiece16 to string16.
string16 PieceToOutputType<string16, string16>(StringPiece16 piece) {
  return piece.as_string();
}

// Returns either the ASCII or UTF-16 whitespace.
template<typename Str> BasicStringPiece<Str> WhitespaceForType();
template<> StringPiece16 WhitespaceForType<string16>() {
  return kWhitespaceUTF16;
}
template<> StringPiece WhitespaceForType<std::string>() {
  return kWhitespaceASCII;
}

// Optimize the single-character case to call find() on the string instead,
// since this is the common case and can be made faster. This could have been
// done with template specialization too, but would have been less clear.
//
// There is no corresponding FindFirstNotOf because StringPiece already
// implements these different versions that do the optimized searching.
size_t FindFirstOf(StringPiece piece, char c, size_t pos) {
  return piece.find(c, pos);
}
size_t FindFirstOf(StringPiece16 piece, char16 c, size_t pos) {
  return piece.find(c, pos);
}
size_t FindFirstOf(StringPiece piece, StringPiece one_of, size_t pos) {
  return piece.find_first_of(one_of, pos);
}
size_t FindFirstOf(StringPiece16 piece, StringPiece16 one_of, size_t pos) {
  return piece.find_first_of(one_of, pos);
}

// General string splitter template. Can take 8- or 16-bit input, can produce
// the corresponding string or StringPiece output, and can take single- or
// multiple-character delimiters.
//
// DelimiterType is either a character (Str::value_type) or a string piece of
// multiple characters (BasicStringPiece<Str>). StringPiece has a version of
// find for both of these cases, and the single-character version is the most
// common and can be implemented faster, which is why this is a template.
template<typename Str, typename OutputStringType, typename DelimiterType>
static std::vector<OutputStringType> SplitStringT(
    BasicStringPiece<Str> str,
    DelimiterType delimiter,
    WhitespaceHandling whitespace,
    SplitResult result_type) {
  std::vector<OutputStringType> result;
  if (str.empty())
    return result;

  size_t start = 0;
  while (start != Str::npos) {
    size_t end = FindFirstOf(str, delimiter, start);

    BasicStringPiece<Str> piece;
    if (end == Str::npos) {
      piece = str.substr(start);
      start = Str::npos;
    } else {
      piece = str.substr(start, end - start);
      start = end + 1;
    }

    if (whitespace == TRIM_WHITESPACE)
      piece = TrimString(piece, WhitespaceForType<Str>(), TRIM_ALL);

    if (result_type == SPLIT_WANT_ALL || !piece.empty())
      result.push_back(PieceToOutputType<Str, OutputStringType>(piece));
  }
  return result;
}

}  // namespace


std::vector<StringPiece> SplitStringPiece(StringPiece input,
                                          StringPiece separators,
                                          WhitespaceHandling whitespace,
                                          SplitResult result_type) {
  if (separators.size() == 1) {
    return SplitStringT<std::string, StringPiece, char>(
        input, separators[0], whitespace, result_type);
  }
  return SplitStringT<std::string, StringPiece, StringPiece>(
      input, separators, whitespace, result_type);
}


}  // namespace base
