#pragma once

#include "simplnx/Common/Result.hpp"

#include <nonstd/expected.hpp>

#include <filesystem>

namespace nx::core::filesystem
{
inline Result<bool> exists(const std::filesystem::path& path)
{
  std::error_code errorCode;
  bool result = std::filesystem::exists(path, errorCode);
  if(errorCode)
  {
    return MakeErrorResult<bool>(errorCode.value(), errorCode.message());
  }
  return {result};
}
} // namespace nx::core::filesystem
