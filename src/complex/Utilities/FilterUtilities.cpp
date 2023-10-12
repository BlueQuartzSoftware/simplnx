#include "FilterUtilities.hpp"

#include <fmt/core.h>

#include <string>

namespace complex
{

// -----------------------------------------------------------------------------
Result<> CreateOutputDirectories(const fs::path& outputPath)
{
  if(!fs::exists(outputPath))
  {
    std::error_code errorCode;
    // So this looks weird but this can happen on
    // platforms where /tmp is a symlink to /private/tmp. So the original path is
    // /tmp/foo but what was created was /private/tmp/foo. This logic should fix that issue.
    if(!fs::create_directories(outputPath, errorCode) && !fs::exists(outputPath))
    {
      return MakeErrorResult(-4010, fmt::format("Unable to create output directory {}. Error code from operating system is {}", outputPath.string(), errorCode.value()));
    }
  }
  return {};
}
} // namespace complex
