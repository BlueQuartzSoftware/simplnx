#pragma once

#include <vector>

namespace fonts
{

// Simple Base64 decoder.  This is used at startup to decode the string
// literals containing embedded resource data, namely font files in TTF form.
//
inline void Base64Decode(char const* input, std::vector<unsigned char>& output)
{
  int index = 0;
  int data = 0;
  int held = 0;
  while(int symbol = input[index++])
  {
    if(symbol == '=')
    {
      break;
    }
    // clang-format off
    const int value = ('A' <= symbol && symbol <= 'Z' ? symbol - 'A' :
                       'a' <= symbol && symbol <= 'z' ? symbol - 'a' + 26 :
                       '0' <= symbol && symbol <= '9' ? symbol - '0' + 52 :
                       symbol == '+'                  ? 62 :
                       symbol == '/'                  ? 63 :
                                                        0);
    // clang-format on
    data = data << 6 | value;
    held += 6;
    if(held >= 8)
    {
      held -= 8;
      output.push_back(static_cast<unsigned char>((data >> held) & 0xff));
      data &= (1 << held) - 1;
    }
  }
}
} // namespace fonts
