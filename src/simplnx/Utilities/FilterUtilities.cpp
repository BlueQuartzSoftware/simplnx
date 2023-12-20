#include "FilterUtilities.hpp"

#include <fmt/core.h>

#include <string>

namespace nx::core
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

// -----------------------------------------------------------------------------
void AppendDataObjectModifications(const DataStructure& dataStructure, std::vector<DataObjectModification>& modifiedActions, const DataPath& parentPath, const std::vector<DataPath>& ignoredDataPaths)
{
  std::optional<std::vector<DataPath>> result = nx::core::GetAllChildArrayDataPaths(dataStructure, parentPath, ignoredDataPaths);
  if(!result)
  {
    return;
  }

  for(const auto& child : result.value())
  {
    modifiedActions.push_back(DataObjectModification{child, DataObjectModification::ModifiedType::Modified, dataStructure.getDataRef(child).getDataObjectType()});
  }
}

} // namespace nx::core
