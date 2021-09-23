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

#include <array>
#include <cctype>
#include <regex>
#include <sstream>
#include <string>
#include <vector>

/*' '(0x20)space(SPC)
* '\t'(0x09)horizontal tab(TAB)
* '\n'(0x0a)newline(LF)
* '\v'(0x0b)vertical tab(VT)
* '\f'(0x0c)feed(FF)
* '\r'(0x0d)carriage return (CR)
*/

namespace complex
{
namespace StringUtilities
{

using StringTokenType = std::vector<std::string>;

// 5 statements
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

// 2 statements
inline StringTokenType split(const std::string& str, char delim)
{
  StringTokenType tokens;
  std::string temp(str);
  std::array<char, 1> delims = {delim};
  auto endPos = std::end(temp);
  for_each_token(std::begin(temp), endPos, std::cbegin(delims), std::cend(delims), [&endPos, &tokens](auto first, auto second) {
    if(first != second)
    {
      if(second != endPos)
      {
        *second = '\0';
      }
      tokens.push_back({&*first});
    }
  });
  // std::cout << "Tokens: " << tokens.size() << std::endl;
  return tokens;
}

inline StringTokenType split_2(const std::string& line, char delimiter)
{
  std::stringstream ss(line);

  StringTokenType tokens;
  std::string temp_str;

  while(getline(ss, temp_str, delimiter))
  {
    tokens.push_back(temp_str);
  }
  return tokens;
}

inline std::string replace(std::string str, const std::string& from, const std::string& to)
{
  size_t start_pos = 0;
  while((start_pos = str.find(from, start_pos)) != std::string::npos)
  {
    str.replace(start_pos, from.length(), to);
    start_pos += to.length();
  }
  return str;
}

inline std::string ltrim(const std::string& s)
{
  std::string out = s;
  if(out.empty())
  {
    return out;
  }
  std::string whitespaces(" \t\f\v\n\r");
  std::string::size_type front = out.find_first_not_of(whitespaces);
  if(front != std::string::npos)
  {
    out = out.substr(front);
  }
  else
  {
    out.clear();
  }
  return out;
}

inline std::string rtrim(const std::string& s)
{
  std::string out = s;
  if(out.empty())
  {
    return out;
  }
  std::string whitespaces(" \t\f\v\n\r");
  std::string::size_type back = out.find_last_not_of(whitespaces);
  if(back != std::string::npos)
  {
    out.erase(back + 1);
  }
  else
  {
    out.clear();
  }
  return out;
}

inline std::string trimmed(const std::string& s)
{
  std::string out = s;
  if(out.empty())
  {
    return out;
  }
  std::string whitespaces(" \t\f\v\n\r");
  std::string::size_type back = out.find_last_not_of(whitespaces);
  if(back != std::string::npos)
  {
    out.erase(back + 1);
  }
  else
  {
    out.clear();
  }
  std::string::size_type front = out.find_first_not_of(whitespaces);
  if(front != std::string::npos)
  {
    out = out.substr(front);
  }
  else
  {
    out.clear();
  }
  return out;
}

inline std::string chop(const std::string& s, size_t numElements)
{
  return s.substr(0, s.size() - numElements);
}

template <typename T>
inline std::string number(T arg)
{
  std::stringstream ss;
  ss << arg;
  return ss.str();
}

inline std::string simplified(const std::string& text)
{
  if(text.empty())
  {
    return {""};
  }
  std::string out;
  out = trimmed(text);
  std::string finalString;
  for(const auto& c : out)
  {
    if(std::isspace(c) == 0)
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

} // namespace StringUtilities
} // namespace complex
