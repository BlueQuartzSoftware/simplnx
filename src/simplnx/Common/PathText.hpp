#pragma once

#include "simplnx/simplnx_export.hpp"

#include <filesystem>
#include <string>

namespace nx::core
{
/**
 * @brief Renders a path as UTF-8 text for a diagnostic, without throwing for its contents.
 *
 * A diagnostic is often built while another failure is being reported. If building it threw, the conversion error would replace the
 * failure the caller meant to report, or turn an expected error result into an exception. `std::filesystem::path::string()` throws on
 * Windows for characters that the active code page cannot represent, and `u8string()` throws for an unpaired UTF-16 surrogate. On
 * Windows this function converts the native UTF-16 name with `WideCharToMultiByte(CP_UTF8, 0, ...)`, without `WC_ERR_INVALID_CHARS`,
 * so invalid code units become U+FFFD; elsewhere it returns the native bytes unchanged.
 * @param path Path to describe.
 * @return The path as UTF-8 text, with unconvertible code units replaced; empty for an empty path.
 * @throws std::bad_alloc Only when the text cannot be allocated.
 */
SIMPLNX_EXPORT std::string PathToUtf8Text(const std::filesystem::path& path);
} // namespace nx::core
