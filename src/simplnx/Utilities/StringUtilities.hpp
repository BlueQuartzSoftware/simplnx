/* ============================================================================
 * Copyright (c) 2020 BlueQuartz Software, LLC
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * Redistributions of source code must retain the above copyright notice, this
 * list of conditions and the following disclaimer.
 *
 * Redistributions in binary form must reproduce the above copyright notice, this
 * list of conditions and the following disclaimer in the documentation and/or
 * other materials provided with the distribution.
 *
 * Neither the names of any of the BlueQuartz Software contributors
 * may be used to endorse or promote products derived from this software without
 * specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE
 * USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ */
#pragma once

#include "simplnx/Common/StringLiteral.hpp"

#include <fmt/ranges.h>
#include <nonstd/span.hpp>

#include <cctype>
#include <sstream>
#include <vector>

/*' '(0x20)space(SPC)
 * '\t'(0x09)horizontal tab(TAB)
 * '\n'(0x0a)newline(LF)
 * '\v'(0x0b)vertical tab(VT)
 * '\f'(0x0c)feed(FF)
 * '\r'(0x0d)carriage return (CR)
 */

namespace
{
template <bool ProcessEmptyV, class InputIt, class ForwardIt, typename TokenT>
void tokenize(InputIt first, InputIt last, ForwardIt s_first, ForwardIt s_last, std::vector<TokenT>& tokens)
{
  while(true)
  {
    const auto pos = std::find_first_of(first, last, s_first, s_last);
    if(first != pos)
    {
      tokens.emplace_back(std::string{first, pos});
    }
    else
    {
      if constexpr(ProcessEmptyV)
      {
        tokens.emplace_back("");
      }
    }
    if(pos == last)
    {
      break;
    }
    first = std::next(pos);
  }
}

template <bool ConsecutiveAsEmptyV, bool EmptyInitialV, bool EmptyFinalV>
struct SplitTypeOptions
{
  static inline constexpr bool AllowConsecutiveAsEmpty = ConsecutiveAsEmptyV;
  static inline constexpr bool AllowEmptyInital = EmptyInitialV;
  static inline constexpr bool AllowEmptyFinal = EmptyFinalV;
};

using SplitIgnoreEmpty = SplitTypeOptions<false, false, false>;
using SplitAllowAll = SplitTypeOptions<true, true, true>;
using SplitNoStripIgnoreConsecutive = SplitTypeOptions<false, true, true>;
using SplitOnlyConsecutive = SplitTypeOptions<true, false, false>;
using SplitAllowEmptyLeftAnalyze = SplitTypeOptions<true, true, false>;
using SplitAllowEmptyRightAnalyze = SplitTypeOptions<true, false, true>;

template <class SplitTypeOptionsV = SplitIgnoreEmpty>
inline std::vector<std::string> optimized_split(std::string_view str, nonstd::span<const char> delimiters)
{
  if(str.empty())
  {
    return {};
  }
  auto endPos = str.end();
  auto startPos = str.begin();

  std::vector<std::string> tokens;
  tokens.reserve(str.size() / 2);

  if constexpr(SplitTypeOptionsV::AllowEmptyInital)
  {
    if(std::find(delimiters.cbegin(), delimiters.cend(), str[0]) != delimiters.cend())
    {
      tokens.emplace_back("");
      startPos++;
    }
  }

  if constexpr(!SplitTypeOptionsV::AllowEmptyFinal)
  {
    if(std::find(delimiters.cbegin(), delimiters.cend(), str[str.size() - 1]) != delimiters.cend())
    {
      endPos--;
    }
  }

  if constexpr(!SplitTypeOptionsV::AllowConsecutiveAsEmpty)
  {
    tokenize<false>(startPos, endPos, delimiters.cbegin(), delimiters.cend(), tokens);
    if constexpr(SplitTypeOptionsV::AllowEmptyFinal)
    {
      if(std::find(delimiters.cbegin(), delimiters.cend(), str[str.size() - 1]) != delimiters.cend())
      {
        tokens.emplace_back("");
      }
    }
  }
  else
  {
    if constexpr(!SplitTypeOptionsV::AllowEmptyInital)
    {
      if(std::find(delimiters.cbegin(), delimiters.cend(), str[0]) != delimiters.cend())
      {
        startPos++;
      }
    }
    tokenize<true>(startPos, endPos, delimiters.cbegin(), delimiters.cend(), tokens);
  }

  tokens.shrink_to_fit();

  // No Delimiters found
  if(tokens.empty())
  {
    tokens.emplace_back(str);
  }

  return tokens;
}
} // namespace

namespace nx::core::StringUtilities
{
inline constexpr StringLiteral k_Whitespaces = " \t\f\v\n\r";

/**
 * @brief Replace characters in a string. If 'from' is empty, the origin string is returned.
 * @param str Input String
 * @param from Characters to replace (These characters are being replaced)
 * @param to The characters to be used as the replacement
 * @return The modified string
 */
inline std::string replace(std::string str, std::string_view from, std::string_view to)
{
  usize startPos = 0;
  if(from.empty())
  {
    return str;
  }
  while((startPos = str.find(from, startPos)) != std::string::npos)
  {
    str.replace(startPos, from.length(), to);
    startPos += to.length();
  }
  return str;
}

inline std::string ltrim(std::string_view str)
{
  if(str.empty())
  {
    return "";
  }

  std::string::size_type front = str.find_first_not_of(k_Whitespaces);
  if(front == std::string::npos)
  {
    return "";
  }

  return std::string(str.substr(front));
}

inline std::string rtrim(std::string_view str)
{
  if(str.empty())
  {
    return "";
  }

  std::string::size_type back = str.find_last_not_of(k_Whitespaces);
  if(back == std::string::npos)
  {
    return "";
  }

  return std::string(str.substr(0, back + 1));
}

inline std::string trimmed(std::string_view str)
{
  if(str.empty())
  {
    return "";
  }

  std::string::size_type back = str.find_last_not_of(k_Whitespaces);
  if(back == std::string::npos)
  {
    return "";
  }

  std::string::size_type front = str.find_first_not_of(k_Whitespaces);

  return std::string(str.substr(front, back - front + 1));
}

/**
 * @brief
 *
 * @param value input string to check
 * @param startingStr The starting string to check the input string for.
 * @return True or False if the given string starts with the given startingStr
 */
inline bool starts_with(std::string_view value, std::string_view startingStr)
{
  if(startingStr.size() > value.size())
  {
    return false;
  }
  return std::equal(startingStr.begin(), startingStr.end(), value.begin());
}

/**
 * @brief
 *
 * @param value input string to check
 * @param ending The ending to check the input string for.
 * @return True or False if the given string ends with the given ending
 */
inline bool ends_with(std::string_view value, std::string_view ending)
{
  if(ending.size() > value.size())
  {
    return false;
  }
  return std::equal(ending.rbegin(), ending.rend(), value.rbegin());
}

enum SplitType : uint8
{
  IgnoreEmpty,
  AllowAll,
  NoStripIgnoreConsecutive,
  OnlyConsecutive,
  AllowEmptyLeftAnalyze,
  AllowEmptyRightAnalyze
};

inline std::vector<std::string> specific_split(std::string_view str, nonstd::span<const char> delimiters, SplitType splitType)
{
  switch(splitType)
  {
  case IgnoreEmpty:
    return optimized_split<::SplitIgnoreEmpty>(str, delimiters);
  case AllowAll:
    return optimized_split<::SplitAllowAll>(str, delimiters);
  case NoStripIgnoreConsecutive:
    return optimized_split<::SplitNoStripIgnoreConsecutive>(str, delimiters);
  case OnlyConsecutive:
    return optimized_split<::SplitOnlyConsecutive>(str, delimiters);
  case AllowEmptyLeftAnalyze:
    return optimized_split<::SplitAllowEmptyLeftAnalyze>(str, delimiters);
  case AllowEmptyRightAnalyze:
    return optimized_split<::SplitAllowEmptyRightAnalyze>(str, delimiters);
  }

  return {};
}

inline std::vector<std::string> split(std::string_view str, nonstd::span<const char> delimiters, bool consecutiveDelimiters)
{
  if(consecutiveDelimiters)
  {
    // Split Allow All was selected to match QString's base split functionality
    return optimized_split<::SplitAllowAll>(str, delimiters);
  }
  else
  {
    return optimized_split<::SplitIgnoreEmpty>(str, delimiters);
  }
}

inline std::vector<std::string> split(std::string_view str, char delim)
{
  std::array<char, 1> delimiters = {delim};
  return optimized_split<::SplitIgnoreEmpty>(str, delimiters);
}

inline std::string join(nonstd::span<std::string_view> vec, std::string_view delim)
{
  return fmt::format("{}", fmt::join(vec, delim));
}

inline bool contains(std::string_view str, std::string_view val)
{
  return str.find(val) != std::string::npos;
}

inline bool contains(std::string_view str, char val)
{
  return str.find(val) != std::string::npos;
}

inline std::string chop(std::string_view str, usize numElements)
{
  return std::string(str.substr(0, str.size() - numElements));
}
inline std::string chop(std::string_view str, std::string_view chopFrom)
{
  if(str.empty())
  {
    return "";
  }

  std::string::size_type back = str.find_last_not_of(chopFrom);
  if(back == std::string::npos)
  {
    return "";
  }

  return std::string(str.substr(0, back + 1));
}

inline std::string chopr(std::string_view str, usize numElements)
{
  return std::string(str.substr(numElements, str.size() - numElements));
}

template <typename T>
inline std::string number(T arg)
{
  std::stringstream ss;
  if constexpr(std::is_floating_point_v<T>)
  {
    ss.precision(std::numeric_limits<T>::digits10);
  }
  ss << arg;
  return ss.str();
}

inline std::string simplified(std::string_view text)
{
  if(text.empty())
  {
    return "";
  }
  std::string out = trimmed(text);
  std::string finalString;
  for(char c : out)
  {
    if(std::isspace(static_cast<unsigned char>(c)) == 0)
    {
      finalString += c;
    }
    else
    {
      finalString += ' ';
    }
  }
  return finalString;
}

/**
 * @brief Converts a char to its uppercase version according to the current locale.
 * Uses std::toupper internally.
 * @param c
 * @return char
 */
inline char toUpper(char c)
{
  return static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
}

/**
 * @brief Converts a char to its lowercase version according to the current locale.
 * Uses std::tolower internally.
 * @param c
 * @return char
 */
inline char toLower(char c)
{
  return static_cast<char>(std::tolower(static_cast<unsigned char>(c)));
}

/**
 * @brief Converts the given string to uppercase.
 * Assumes that the string is ASCII.
 * @param input
 * @return std::string
 */
inline std::string toUpper(std::string input)
{
  for(char& c : input)
  {
    c = toUpper(c);
  }
  return input;
}

/**
 * @brief Converts the given string to lowercase.
 * Assumes that the string is ASCII.
 * @param input
 * @return std::string
 */
inline std::string toLower(std::string input)
{
  for(char& c : input)
  {
    c = toLower(c);
  }
  return input;
}

} // namespace nx::core::StringUtilities
