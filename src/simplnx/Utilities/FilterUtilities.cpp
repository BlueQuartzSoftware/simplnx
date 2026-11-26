#include "FilterUtilities.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FileUtilities.hpp"

#include <fmt/format.h>
#include <fmt/ranges.h>

#include <algorithm>
#include <filesystem>
#include <string>
#include <system_error>

namespace fs = std::filesystem;

namespace nx::core
{
namespace
{
constexpr int32 k_UnchangedAttributeMatricesWarning = -4041;

void CollectAttributeMatrixPaths(const DataObject& object, const DataPath& path, const std::vector<DataObject::IdType>& excludedIds, std::vector<std::string>& paths)
{
  if(std::ranges::find(excludedIds, object.getId()) != excludedIds.end())
  {
    return;
  }
  if(dynamic_cast<const AttributeMatrix*>(&object) != nullptr)
  {
    paths.push_back(path.toString());
    return;
  }
  if(const auto* groupPtr = dynamic_cast<const BaseGroup*>(&object); groupPtr != nullptr)
  {
    for(const auto& [id, child] : *groupPtr)
    {
      if(dynamic_cast<const BaseGroup*>(child.get()) != nullptr)
      {
        CollectAttributeMatrixPaths(*child, path.createChildPath(child->getName()), excludedIds, paths);
      }
    }
  }
}
} // namespace

// -----------------------------------------------------------------------------
Result<> CreateOutputDirectories(const fs::path& outputPath)
{
  constexpr int32 k_PathStatusError = -4090;
  constexpr int32 k_CreateDirectoryError = -4010;
  std::error_code statusError;
  const bool outputExists = fs::exists(outputPath, statusError);
  if(statusError)
  {
    return MakeErrorResult(k_PathStatusError, fmt::format("Unable to inspect output path '{}'. Check directory access. Operating system error {}: {}",
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
    return MakeErrorResult(k_CreateDirectoryError, fmt::format("Unable to create output directory '{}'. Check the path and write permissions. Operating system error {}: {}",
                                                             FileUtilities::detail::PathForDiagnostic(outputPath), creationError.value(), creationError.message()));
  }
  if(statusError)
  {
    return MakeErrorResult(k_PathStatusError, fmt::format("Unable to inspect output path '{}' after the directory creation attempt. Check directory access. Operating system error {}: {}",
                                                        FileUtilities::detail::PathForDiagnostic(outputPath), statusError.value(), statusError.message()));
  }
  return MakeErrorResult(k_CreateDirectoryError,
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
    modifiedActions.push_back(
        DataObjectModification{.modifiedPath = child, .modifiedType = DataObjectModification::ModifiedType::Modified, .dataObjectType = dataStructure.getDataRef(child).getDataObjectType()});
  }
}

// -----------------------------------------------------------------------------
void MarkDataPathModified(const DataStructure& dataStructure, nx::core::Result<OutputActions>& resultOutputActions, const DataPath& targetDataPath)
{
  resultOutputActions.value().modifiedActions.push_back(DataObjectModification{
      .modifiedPath = targetDataPath, .modifiedType = DataObjectModification::ModifiedType::Modified, .dataObjectType = dataStructure.getDataRef(targetDataPath).getDataObjectType()});
}

// -----------------------------------------------------------------------------
IFilter::PreflightResult NeighborListRemovalPreflightCode(const DataStructure& dataStructure, const DataPath& /*featureIdsPath*/, const DataPath& numNeighborsPath,
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
    return {nonstd::make_unexpected(std::vector<Error>{
        Error{.code = k_FetchChildArrayError, .message = fmt::format("Errors were encountered trying to retrieve the neighbor list children of group '{}'", featureGroupDataPath.toString())}})};
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
    resultOutputActions.warnings().push_back(Warning{.code = k_NeighborListRemoval, .message = ss});
  }
  return {};
}

// -----------------------------------------------------------------------------
void AppendStaleFeatureArraysPreflightValue(const DataStructure& dataStructure, const DataPath& featureAttributeMatrixPath, const DataPath& featureIdsPath,
                                            std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  const auto* featureAttributeMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(featureAttributeMatrixPath);
  if(featureAttributeMatrixPtr == nullptr)
  {
    return;
  }

  std::string list;
  for(const auto& [identifier, object] : *featureAttributeMatrixPtr)
  {
    if(dynamic_cast<const IDataArray*>(object.get()) != nullptr)
    {
      list += "\n" + featureAttributeMatrixPath.createChildPath(object->getName()).toString();
    }
  }

  if(!list.empty())
  {
    preflightUpdatedValues.push_back({"Stale Arrays", fmt::format("This filter modifies '{}' and removes Features from '{}'. The values in the following Feature Data Arrays are not recomputed and "
                                                                  "will be stale. Recompute any of the following Data Arrays:{}",
                                                                  featureIdsPath.toString(), featureAttributeMatrixPath.toString(), list)});
  }
}

// -----------------------------------------------------------------------------
void AppendUnchangedAttributeMatricesWarning(const DataStructure& dataStructure, const std::vector<DataPath>& childPaths, const std::vector<DataPath>& excludedPaths, std::string_view outputGeometry,
                                             Result<OutputActions>& resultOutputActions, bool keptInPlace)
{
  std::vector<DataObject::IdType> excludedIds;
  for(const auto& path : excludedPaths)
  {
    if(const auto* objectPtr = dataStructure.getData(path); objectPtr != nullptr)
    {
      excludedIds.push_back(objectPtr->getId());
    }
  }

  std::vector<std::string> paths;
  for(const auto& path : childPaths)
  {
    if(const auto* objectPtr = dataStructure.getData(path); objectPtr != nullptr)
    {
      CollectAttributeMatrixPaths(*objectPtr, path, excludedIds, paths);
    }
  }
  if(!paths.empty())
  {
    std::ranges::sort(paths);
    resultOutputActions.warnings().push_back(
        {k_UnchangedAttributeMatricesWarning, fmt::format("These Attribute Matrices are {} the {} unchanged:\n{}\n"
                                                          "If one holds Feature data, its values describe the original geometry. Recompute them. Ensemble arrays retain their stored values. "
                                                          "Recompute any Ensemble statistics that depend on the geometry's cells or elements.",
                                                          keptInPlace ? "kept in" : "copied to", outputGeometry, fmt::join(paths, "\n"))});
  }
}

} // namespace nx::core
