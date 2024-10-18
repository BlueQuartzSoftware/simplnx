#pragma once

#include "simplnx/Common/StringLiteral.hpp"

#include <fmt/core.h>
#include <fmt/xchar.h>

template <class CharT>
struct fmt::formatter<nx::core::BasicStringLiteral<CharT>>
{
  static constexpr const CharT* GetFormatString()
  {
    if constexpr(std::is_same_v<CharT, char>)
    {
      return "{}";
    }
    if constexpr(std::is_same_v<CharT, wchar_t>)
    {
      return L"{}";
    }
    if constexpr(std::is_same_v<CharT, char16_t>)
    {
      return u"{}";
    }
    if constexpr(std::is_same_v<CharT, char32_t>)
    {
      return U"{}";
    }
  }

  constexpr typename basic_format_parse_context<CharT>::iterator parse(basic_format_parse_context<CharT>& ctx)
  {
    return ctx.begin();
  }

  typename buffer_context<CharT>::iterator format(const nx::core::BasicStringLiteral<CharT>& p, buffer_context<CharT>& ctx) const
  {
    static constexpr const CharT* formatStr = GetFormatString();

    return fmt::format_to(ctx.out(), formatStr, p.view());
  }
};
