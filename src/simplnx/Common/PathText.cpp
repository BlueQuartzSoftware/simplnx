#include "simplnx/Common/PathText.hpp"

#ifdef _WIN32
#include <cstddef>
#include <limits>

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#include <windows.h>
#endif

namespace nx::core
{
std::string PathToUtf8Text(const std::filesystem::path& path)
{
#ifdef _WIN32
  const std::wstring& wide = path.native();
  if(wide.empty() || wide.size() > static_cast<std::size_t>((std::numeric_limits<int>::max)()))
  {
    return {};
  }
  const int wideLength = static_cast<int>(wide.size());
  const int textLength = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, nullptr, 0, nullptr, nullptr);
  if(textLength <= 0)
  {
    return {};
  }
  std::string text(static_cast<std::size_t>(textLength), '\0');
  const int written = WideCharToMultiByte(CP_UTF8, 0, wide.data(), wideLength, text.data(), textLength, nullptr, nullptr);
  text.resize(static_cast<std::size_t>(written > 0 ? written : 0));
  return text;
#else
  return path.native();
#endif
}
} // namespace nx::core
