// Copyright 2024 N-GINN LLC. All rights reserved.

// Copyright (C) 2024  Gaijin Games KFT.  All rights reserved
#pragma once
#include <string>
#include <string_view>

// calls Cb(std::string_view found_view)
//  if Cb returns false, tokenize_const_string will return false

template <typename Cb>
inline bool tokenize_const_string(std::string_view str, const char* delim, Cb cb)
{
  size_t start, end = 0;
  while (end < str.length() && str[end] != 0)
  {
    start = end;
    while (start < str.length() && str[start] && strchr(delim, str[start]) != nullptr)
      start++; // skip initial delimeters
    end = start;
    while (end < str.length() && str[end] && strchr(delim, str[end]) == nullptr)
      end++; // skip to end of word

    if (end - start != 0) // just ignore zero-length strings.
        if (!cb(std::string_view(str.data() + start, end - start)))
            return false;
    ++end;
  }
  return true;
}
