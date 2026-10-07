#include "FilterUtilities.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FileUtilities.hpp"

#include <fmt/format.h>

#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace nx::core
{

// -----------------------------------------------------------------------------
Result<> CreateOutputDirectories(const fs::path& outputPath)
{
  constexpr int32 pathStatusError = -4090;
  constexpr int32 createDirectoryError = -4010;
  std::error_code statusError;
  const bool outputExists = fs::exists(outputPath, statusError);
  if(statusError)
  {
    return MakeErrorResult(pathStatusError, fmt::format("Unable to inspect output path '{}'. Check directory access. Operating system error {}: {}",
                                                        FileUtilities::detail::PathForDiagnostic(outputPath), statusError.value(), statusError.message()));
  }
  if(outputExists)
  {
    return {};
  }

  std::error_code creationError;
  if(fs::create_directories(outputPath, creationError))
  {
    return {};
  }

  // Another process can create the directory between the first check and the creation attempt.
  if(fs::exists(outputPath, statusError))
  {
    return {};
  }
  if(creationError)
  {
    return MakeErrorResult(createDirectoryError, fmt::format("Unable to create output directory '{}'. Check the path and write permissions. Operating system error {}: {}",
                                                             FileUtilities::detail::PathForDiagnostic(outputPath), creationError.value(), creationError.message()));
  }
  if(statusError)
  {
    return MakeErrorResult(pathStatusError, fmt::format("Unable to inspect output path '{}' after the directory creation attempt. Check directory access. Operating system error {}: {}",
                                                        FileUtilities::detail::PathForDiagnostic(outputPath), statusError.value(), statusError.message()));
  }
  return MakeErrorResult(createDirectoryError,
                         fmt::format("Output directory '{}' no longer exists after the creation attempt. Check the path and try again.", FileUtilities::detail::PathForDiagnostic(outputPath)));
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
void MarkDataPathModified(const DataStructure& dataStructure, nx::core::Result<OutputActions>& resultOutputActions, const DataPath& targetDataPath)
{
  resultOutputActions.value().modifiedActions.push_back(
      DataObjectModification{targetDataPath, DataObjectModification::ModifiedType::Modified, dataStructure.getDataRef(targetDataPath).getDataObjectType()});
}

// -----------------------------------------------------------------------------
IFilter::PreflightResult NeighborListRemovalPreflightCode(const DataStructure& dataStructure, const DataPath& featureIdsPath, const DataPath& numNeighborsPath,
                                                          nx::core::Result<OutputActions>& resultOutputActions)
{
  constexpr int32 k_FetchChildArrayError = -5559;
  constexpr int32 k_NeighborListRemoval = -5558;

  DataPath featureGroupDataPath = numNeighborsPath.getParent();

  // Report that existing NeighborList arrays will be removed.
  std::string ss = fmt::format("This filter will REMOVE all arrays of type NeighborList from the feature Attribute Matrix '{}'.  These arrays are:\n", featureGroupDataPath.toString());

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

  // Report modified feature arrays.
  nx::core::AppendDataObjectModifications(dataStructure, resultOutputActions.value().modifiedActions, featureGroupDataPath, {});

  // Only warn when there is actually something to remove. Warning unconditionally announced that
  // NeighborLists would be removed and then listed none, which is noise on every preflight of every
  // caller, and it silently defeats any test that asserts merely that the warning list is non-empty.
  if(!featureNeighborListArrays.empty())
  {
    resultOutputActions.warnings().push_back(Warning{k_NeighborListRemoval, ss});
  }
  return {};
}

} // namespace nx::core
