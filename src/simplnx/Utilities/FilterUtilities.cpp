#include "FilterUtilities.hpp"

#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

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
      return MakeErrorResult(-4010, fmt::format("Unable to create output directory {}. Error code from operating system is {}", outputPath.string(), errorCode.value(), errorCode.message()));
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

// -----------------------------------------------------------------------------
IFilter::PreflightResult NeighborListRemovalPreflightCode(const DataStructure& dataStructure, const DataPath& featureIdsPath, const DataPath& numNeighborsPath,
                                                          nx::core::Result<OutputActions>& resultOutputActions)
{
  constexpr int32 k_FetchChildArrayError = -5559;
  constexpr int32 k_NeighborListRemoval = -5558;

  DataPath featureGroupDataPath = numNeighborsPath.getParent();

  // Throw a warning to inform the user that the neighbor list arrays could be deleted by this filter
  std::string ss =
      fmt::format("This filter WILL remove all arrays of type NeighborList from the feature Attribute Matrix '{}'.  These arrays are:\n", featureIdsPath.toString(), featureGroupDataPath.toString());

  auto result = nx::core::GetAllChildDataPaths(dataStructure, featureGroupDataPath, DataObject::Type::NeighborList);
  if(!result.has_value())
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{k_FetchChildArrayError, fmt::format("Errors were encountered trying to retrieve the neighbor list children of group '{}'", featureGroupDataPath.toString())}})};
  }
  std::vector<DataPath> featureNeighborListArrays = result.value();
  for(const auto& featureNeighborList : featureNeighborListArrays)
  {
    ss.append("  " + featureNeighborList.toString() + "\n");
    auto action = std::make_unique<DeleteDataAction>(featureNeighborList);
    resultOutputActions.value().deferredActions.emplace_back(std::move(action));
  }

  // Inform users that the following arrays are going to be modified in place
  // Feature Data is going to be modified
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, featureGroupDataPath, {});

  resultOutputActions.warnings().push_back(Warning{k_NeighborListRemoval, ss});
  return {};
}

} // namespace nx::core
