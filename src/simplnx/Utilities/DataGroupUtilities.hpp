#pragma once

#include "simplnx/DataStructure/BaseGroup.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/simplnx_export.hpp"

#include <memory>
#include <vector>

namespace nx::core
{
/**
 * @struct FeatureRenumbering
 * @brief Holds the stable compaction mapping for inactive feature removal.
 *
 * Feature zero and inactive features map to zero. Active features retain their
 * relative order and map to consecutive IDs that start at one.
 */
struct FeatureRenumbering
{
  std::vector<size_t> newNames; ///< Maps each old feature ID to its compacted ID.
  std::vector<size_t> keepList; ///< Lists active nonzero feature IDs in ascending order.
  bool anyRemoved = false;      ///< True when at least one nonzero feature is inactive.
};

/**
 * @brief Computes the stable feature-ID compaction mapping.
 * @param activeObjects Supplies one active flag for each old feature ID.
 * @return Compaction mapping, ascending keep list, and removal state.
 * @pre activeObjects index zero represents the background feature.
 *
 * RemoveInactiveObjects and fused caller passes use this mapping. The shared
 * mapping keeps cell IDs consistent with feature-array compaction.
 */
SIMPLNX_EXPORT FeatureRenumbering ComputeFeatureRenumbering(const std::vector<bool>& activeObjects);

/**
 * @brief Compacts one-dimensional feature data after inactive feature removal.
 * @param dataStructure Owns the feature group and cell feature IDs.
 * @param featureDataGroupPath Identifies the feature attribute matrix or group.
 * @param activeObjects Supplies one active flag for each current feature tuple.
 * @param cellFeatureIds Supplies cell feature IDs to renumber when required.
 * @param currentFeatureCount Specifies the expected feature tuple count.
 * @param messageHandler Receives information about invalidated neighbor lists.
 * @param shouldCancel Supplies the cancellation flag.
 * @param cellFeatureIdsRenumbered True when the caller already applied
 * ComputeFeatureRenumbering to cellFeatureIds.
 * @return True on completion. False if validation fails or cancellation occurs.
 * @pre The feature group has a one-dimensional tuple shape.
 *
 * The function compacts feature arrays and resizes the feature attribute matrix.
 * It skips the full cell-ID pass when cellFeatureIdsRenumbered is true. It does
 * not remove invalidated neighbor lists. The calling filter schedules their removal.
 */
SIMPLNX_EXPORT bool RemoveInactiveObjects(DataStructure& dataStructure, const DataPath& featureDataGroupPath, const std::vector<bool>& activeObjects, Int32AbstractDataStore& cellFeatureIds,
                                          size_t currentFeatureCount, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, bool cellFeatureIdsRenumbered = false);

/**
 * @brief Gets sibling data arrays and excludes selected paths.
 * @param dataStructure Supplies the sibling arrays.
 * @param dataArrayPath Identifies the parent that contains the siblings.
 * @param ignoredDataPaths Identifies arrays to exclude.
 * @return Shared pointers to the remaining sibling arrays.
 */
SIMPLNX_EXPORT std::vector<std::shared_ptr<IDataArray>> GenerateDataArrayList(const DataStructure& dataStructure, const DataPath& dataArrayPath, const std::vector<DataPath>& ignoredDataPaths);

/**
 * @brief Gets direct child paths that have a selected data-object type.
 * @param dataStructure Supplies the child objects.
 * @param parentGroup Identifies the parent group. An empty path selects the root.
 * @param dataObjectType Selects the required child type. DataObject selects all types.
 * @param ignoredDataPaths Identifies child paths to exclude.
 * @return Matching paths, or std::nullopt if the parent is invalid or access fails.
 */
SIMPLNX_EXPORT std::optional<std::vector<DataPath>> GetAllChildDataPaths(const DataStructure& dataStructure, const DataPath& parentGroup, DataObject::Type dataObjectType,
                                                                         const std::vector<DataPath>& ignoredDataPaths = {});

/**
 * @brief Gets all direct child paths for a group.
 * @param dataStructure Supplies the child objects.
 * @param parent Identifies the parent group.
 * @return Child paths, or std::nullopt if the parent is invalid or access fails.
 */
SIMPLNX_EXPORT std::optional<std::vector<DataPath>> GetAllChildDataPaths(const DataStructure& dataStructure, const DataPath& parent);

template <typename T>
concept DataObjectDerivedType = std::is_base_of_v<DataObject, T>;

/**
 * @brief Gets direct child paths that can convert to a selected data-object type.
 * @tparam TargetObjectT Specifies the required DataObject-derived type.
 * @param dataStructure Supplies the child objects.
 * @param parentGroup Identifies the parent group. An empty path selects the root.
 * @param ignoredDataPaths Identifies child paths to exclude.
 * @return Matching paths, or std::nullopt if access fails.
 */
template <DataObjectDerivedType TargetObjectT>
std::optional<std::vector<DataPath>> GetAllChildDataPathsOfType(const DataStructure& dataStructure, const DataPath& parentGroup, const std::vector<DataPath>& ignoredDataPaths = {})
{
  std::vector<DataPath> childDataObjects;
  try
  {
    std::vector<std::string> childrenNames;
    if(parentGroup.empty())
    {
      childrenNames = dataStructure.getDataMap().getNames();
    }
    else
    {
      childrenNames = dataStructure.getDataRefAs<BaseGroup>(parentGroup).getDataMap().getNames();
    }

    for(const auto& childName : childrenNames)
    {
      bool ignore = false;
      DataPath childPath = parentGroup.createChildPath(childName);
      const DataObject* dataObject = dataStructure.getData(childPath);
      for(const auto& ignoredPath : ignoredDataPaths)
      {
        if(childPath == ignoredPath)
        {
          ignore = true;
          break;
        }
      }
      if(!ignore && dynamic_cast<const TargetObjectT*>(dataObject) != nullptr)
      {
        childDataObjects.push_back(childPath);
      }
    }
  } catch(std::exception& e)
  {
    return {};
  }
  return {childDataObjects};
}

/**
 * @brief Gets direct child paths that identify IArray objects.
 * @param dataStructure Supplies the child objects.
 * @param parentGroup Identifies the parent group. An empty path selects the root.
 * @param ignoredDataPaths Identifies child paths to exclude.
 * @return Array paths, or std::nullopt if access fails.
 */
SIMPLNX_EXPORT std::optional<std::vector<DataPath>> GetAllChildArrayDataPaths(const DataStructure& dataStructure, const DataPath& parentGroup, const std::vector<DataPath>& ignoredDataPaths = {});

/**
 * @brief Gets all descendant paths below a group.
 * @param dataStructure Supplies the descendant objects.
 * @param parentGroup Identifies the traversal root. An empty path selects the data-structure root.
 * @param ignoredDataPaths Identifies paths and subtrees to exclude.
 * @return Descendant paths in depth-first order, or std::nullopt if access fails.
 */
SIMPLNX_EXPORT std::optional<std::vector<DataPath>> GetAllChildDataPathsRecursive(const DataStructure& dataStructure, const DataPath& parentGroup, const std::vector<DataPath>& ignoredDataPaths = {});

/**
 * @brief Gets descendant paths that have a selected data-object type.
 * @param dataStructure Supplies the descendant objects.
 * @param parentGroup Identifies the traversal root. An empty path selects the data-structure root.
 * @param dataObjectType Selects the required type. DataObject selects all types.
 * @param ignoredDataPaths Identifies paths and subtrees to exclude.
 * @return Matching descendant paths, or std::nullopt if access fails.
 */
SIMPLNX_EXPORT std::optional<std::vector<DataPath>> GetAllChildDataPathsRecursive(const DataStructure& dataStructure, const DataPath& parentGroup, DataObject::Type dataObjectType,
                                                                                  const std::vector<DataPath>& ignoredDataPaths = {});

/**
 * @brief Tests whether a group has a direct IDataArray child with a selected name.
 * @param dataStructure Supplies the child objects.
 * @param parentGroup Identifies the parent group. An empty path selects the root.
 * @param arrayName Specifies the target child name.
 * @return True when the named direct child is an IDataArray. Access errors return false.
 */
SIMPLNX_EXPORT bool ContainsDataArrayName(const DataStructure& dataStructure, const DataPath& parentGroup, const std::string& arrayName);

} // namespace nx::core
