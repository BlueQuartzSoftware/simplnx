#include "FilterUtilities.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"

#include <fmt/core.h>

#include <string>

namespace fs = std::filesystem;

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

  // Throw a warning to inform the user that the neighbor list arrays could be deleted by this filter
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

  // Inform users that the following arrays are going to be modified in place
  // Feature Data is going to be modified
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

// -----------------------------------------------------------------------------
void AppendRenumberedFeatureAMWarnings(const DataStructure& dataStructure, const DataPath& cellFeatureAmPath, const DataPath& featureIdsArrayPath,
                                       std::vector<IFilter::PreflightValue>& preflightUpdatedValues)
{
  const auto* srcCellFeatureData = dataStructure.getDataAs<AttributeMatrix>(cellFeatureAmPath);
  if(srcCellFeatureData == nullptr)
  {
    return;
  }

  std::string neighborListWarningMsg;
  std::string arrayWarningMsg;
  for(const auto& [identifier, object] : *srcCellFeatureData)
  {
    std::string objectText = "\n" + cellFeatureAmPath.toString() + "/" + object->getName();
    if(dynamic_cast<const IDataArray*>(object.get()) != nullptr)
    {
      arrayWarningMsg += objectText;
    }
    else if(dynamic_cast<const INeighborList*>(object.get()) != nullptr)
    {
      neighborListWarningMsg += objectText;
    }
  }

  if(!neighborListWarningMsg.empty())
  {
    preflightUpdatedValues.push_back(
        {"Invalidated NeighborLists",
         fmt::format(
             "This filter will modify the Cell Level Array(s) '{}' which causes all feature level NeighborLists to become invalid. These NeighborLists will not be copied to the new geometry:{}",
             featureIdsArrayPath.toString(), neighborListWarningMsg)});
  }
  if(!arrayWarningMsg.empty())
  {
    preflightUpdatedValues.push_back(
        {"Stale Arrays", fmt::format("This filter will modify the Cell Level Array(s) '{}', due to the dependent nature of feature level arrays, the pruning process will cause the data contained "
                                     "within to be stale. It's highly recommended to recalculate the following:{}",
                                     featureIdsArrayPath.toString(), arrayWarningMsg)});
  }
}

// -----------------------------------------------------------------------------
void AppendCopiedAMStaleWarning(const DataStructure& dataStructure, const std::vector<DataPath>& childPaths, nx::core::Result<OutputActions>& resultOutputActions)
{
  std::string amWarningMsg;
  for(const auto& childPath : childPaths)
  {
    if(dataStructure.getDataAs<AttributeMatrix>(childPath) != nullptr)
    {
      amWarningMsg += "\n" + childPath.toString();
    }
  }
  if(!amWarningMsg.empty())
  {
    resultOutputActions.m_Warnings.emplace_back(
        -4015,
        fmt::format("Attribute Matrices with arrays dependent upon Cell Data will be copied across as-is. It's highly recommended to recalculate the child arrays in the following:{}", amWarningMsg));
  }
}

} // namespace nx::core
