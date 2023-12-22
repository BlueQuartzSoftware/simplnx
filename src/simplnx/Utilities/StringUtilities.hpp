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

namespace nx::core
{
namespace StringUtilities
{
inline constexpr StringLiteral k_Whitespaces = " \t\f\v\n\r";

template <class InputIt, class ForwardIt, class BinOp>
void for_each_token(InputIt first, InputIt last, ForwardIt s_first, ForwardIt s_last, BinOp binary_op)
{
  while(true)
  {
    const auto pos = std::find_first_of(first, last, s_first, s_last);
    binary_op(first, pos);
    if(pos == last)
    {
      break;
    }
    first = std::next(pos);
  }
}

/**
 * @brief Replace characters in a string
 * @param str Input String
 * @param from Characters to replace (These characters are being replaced)
 * @param to The characters to be used as the replacement
 * @return The modified string
 */
inline std::string replace(std::string str, std::string_view from, std::string_view to)
{
  usize startPos = 0;
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

inline std::vector<std::string> split(std::string_view str, nonstd::span<const char> delimiters, bool consecutiveDelimiters)
{
  std::vector<std::string> tokens;
  auto endPos = str.end();
  for_each_token(str.begin(), endPos, delimiters.cbegin(), delimiters.cend(), [&tokens, &consecutiveDelimiters](auto first, auto second) {
    if(first != second)
    {
      std::string substr = {first, second};
      if(!substr.empty() || !consecutiveDelimiters)
      {
        tokens.push_back(substr);
      }
    }
  });
  return tokens;
}

inline std::vector<std::string> split(std::string_view str, char delim)
{
  std::array<char, 1> delims = {delim};
  return split(str, delims, false);
}

inline std::vector<std::string> split_2(const std::string& line, char delimiter)
{
  std::stringstream ss(line);

  std::vector<std::string> tokens;
  std::string tempStr;

  while(std::getline(ss, tempStr, delimiter))
  {
    tokens.push_back(tempStr);
  }
  return tokens;
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

} // namespace StringUtilities
} // namespace nx::core
