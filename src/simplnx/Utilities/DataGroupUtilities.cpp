#include "DataGroupUtilities.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/BaseGroup.hpp"

#include <nonstd/span.hpp>

#include <memory>

namespace nx::core
{
FeatureRenumbering ComputeFeatureRenumbering(const std::vector<bool>& activeObjects)
{
  FeatureRenumbering result;
  result.newNames.assign(activeObjects.size(), 0);
  result.keepList.reserve(activeObjects.size());

  size_t goodCount = 1;
  for(size_t i = 1; i < activeObjects.size(); i++)
  {
    if(activeObjects[i])
    {
      result.newNames[i] = goodCount;
      goodCount++;
      result.keepList.push_back(i);
    }
    else
    {
      result.newNames[i] = 0;
      result.anyRemoved = true;
    }
  }
  return result;
}

bool RemoveInactiveObjects(DataStructure& dataStructure, const DataPath& featureDataGroupPath, const std::vector<bool>& activeObjects, Int32AbstractDataStore& cellFeatureIds,
                           size_t currentFeatureCount, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, bool cellFeatureIdsRenumbered)
{
  const auto* featureLevelBaseGroup = dataStructure.getDataAs<const BaseGroup>(featureDataGroupPath);

  if(nullptr == featureLevelBaseGroup)
  {
    return false;
  }
  const DataMap& featureDataMap = featureLevelBaseGroup->getDataMap();

  // Only arrays with one tuple for each active flag contain feature-level data
  // that this operation can compact.
  std::vector<std::shared_ptr<IDataArray>> matchingDataArrayPtrs;

  for(const auto& entry : featureDataMap)
  {
    std::shared_ptr<DataObject> dataObject = entry.second;
    std::shared_ptr<IDataArray> dataArray = std::dynamic_pointer_cast<IDataArray>(dataObject);
    if(nullptr != dataArray)
    {
      if(dataArray->getNumberOfTuples() == activeObjects.size())
      {
        matchingDataArrayPtrs.push_back(dataArray);
      }
    }
  }
  size_t totalTuples = currentFeatureCount;
  if(activeObjects.size() == totalTuples)
  {
    // Use one mapping for the feature arrays and the optional cell-ID pass.
    // A fused caller can apply this mapping before it calls this function.
    const FeatureRenumbering renumbering = ComputeFeatureRenumbering(activeObjects);
    const std::vector<size_t>& newNames = renumbering.newNames;
    const std::vector<size_t>& keepList = renumbering.keepList;

    std::vector<usize> newShape = {keepList.size() + 1};
    if(renumbering.anyRemoved)
    {
      for(const auto& dataArray : matchingDataArrayPtrs)
      {
        if(shouldCancel)
        {
          return false;
        }
        // keepList is ascending. Each source is at or after its destination, so
        // forward in-place copies cannot overwrite an unread source tuple.
        size_t destIdx = 1;
        for(const auto& keepIdx : keepList)
        {
          dataArray->copyTuple(keepIdx, destIdx);
          destIdx++;
        }
        // The AttributeMatrix resize below resizes all child arrays together.
        // Keep this direct-resize alternative disabled to preserve group consistency.
        // dataArray->getIDataStore()->resizeTuples(newShape);
      }

      bool featureIdsChanged = false;
      if(!cellFeatureIdsRenumbered)
      {
        // Fixed-size bulk transfers avoid one disk-backed access for each cell.
        constexpr size_t k_ChunkSize = 65536;
        size_t totalPoints = cellFeatureIds.getNumberOfTuples();
        auto chunkBuf = std::make_unique<int32_t[]>(k_ChunkSize);
        for(size_t offset = 0; offset < totalPoints; offset += k_ChunkSize)
        {
          if(shouldCancel)
          {
            return false;
          }
          size_t count = std::min(k_ChunkSize, totalPoints - offset);
          cellFeatureIds.copyIntoBuffer(offset, nonstd::span<int32_t>(chunkBuf.get(), count));
          bool chunkModified = false;
          for(size_t i = 0; i < count; i++)
          {
            if(chunkBuf[i] >= 0 && static_cast<size_t>(chunkBuf[i]) < newNames.size())
            {
              chunkBuf[i] = static_cast<int32_t>(newNames[chunkBuf[i]]);
              chunkModified = true;
            }
          }
          if(chunkModified)
          {
            cellFeatureIds.copyFromBuffer(offset, nonstd::span<const int32_t>(chunkBuf.get(), count));
            featureIdsChanged = true;
          }
        }
      }
      else
      {
        // An upstream cell-ID pass also invalidates feature neighbor lists.
        featureIdsChanged = true;
      }

      if(featureIdsChanged)
      {
        auto result = GetAllChildDataPaths(dataStructure, featureDataGroupPath, DataObject::Type::NeighborList);
        if(result.has_value())
        {
          std::vector<DataPath> neighborListDataPaths = result.value();
          for(const auto& neighborListDataPath : neighborListDataPaths)
          {
            messageHandler(
                nx::core::IFilter::Message{nx::core::IFilter::Message::Type::Info, fmt::format("NeighborList '{}' will be removed from the DataStructure.", neighborListDataPath.toString())});
          }
        }
      }
    }

    // Resizing the attribute matrix applies the compacted shape to all children.
    auto* featureAttMatrixPtr = dataStructure.getDataAs<AttributeMatrix>(featureDataGroupPath);
    if(featureAttMatrixPtr != nullptr)
    {
      featureAttMatrixPtr->resizeTuples(newShape);
    }
  }
  else
  {
    return false;
  }
  return true;
}

std::vector<std::shared_ptr<IDataArray>> GenerateDataArrayList(const DataStructure& dataStructure, const DataPath& dataArrayPath, const std::vector<DataPath>& ignoredDataPaths)
{
  std::vector<std::shared_ptr<IDataArray>> arrays;
  std::set<std::shared_ptr<IDataArray>> childArrays;
  DataPath parentPath = dataArrayPath.getParent();
  if(parentPath.empty())
  {
    for(const auto& [key, object] : dataStructure.getDataMap())
    {
      if(auto typePtr = std::dynamic_pointer_cast<IDataArray>(object); typePtr != nullptr)
      {
        childArrays.insert(typePtr);
      }
    }
  }
  else
  {
    const auto& parent = dataStructure.getDataRefAs<BaseGroup>(parentPath);
    childArrays = parent.findAllChildrenOfType<IDataArray>();
  }
  for(const auto& childArray : childArrays)
  {
    bool ignore = false;
    DataPath childArrayPath;
    for(const auto& childDataPath : childArray->getDataPaths())
    {
      if(parentPath == childDataPath.getParent())
      {
        childArrayPath = childDataPath;
      }
    }
    for(const auto& ignoredPath : ignoredDataPaths)
    {
      if(childArrayPath == ignoredPath)
      {
        ignore = true;
        break;
      }
    }
    if(!ignore)
    {
      arrays.push_back(childArray);
    }
  }
  return arrays;
}

std::optional<std::vector<DataPath>> GetAllChildDataPaths(const DataStructure& dataStructure, const DataPath& parentGroup, DataObject::Type dataObjectType,
                                                          const std::vector<DataPath>& ignoredDataPaths)
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
      const auto* baseGroup = dataStructure.getDataAs<BaseGroup>(parentGroup);
      if(baseGroup == nullptr)
      {
        return {};
      }
      childrenNames = baseGroup->getDataMap().getNames();
    }

    for(const auto& childName : childrenNames)
    {
      bool ignore = false;
      DataPath childPath = parentGroup.createChildPath(childName);
      for(const auto& ignoredPath : ignoredDataPaths)
      {
        if(childPath == ignoredPath)
        {
          ignore = true;
          break;
        }
      }
      const DataObject* dataObject = dataStructure.getData(childPath);
      if(dataObject != nullptr && !ignore && (dataObjectType == DataObject::Type::DataObject || dataObject->getDataObjectType() == dataObjectType))
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

std::optional<std::vector<DataPath>> GetAllChildDataPaths(const DataStructure& dataStructure, const DataPath& parent)
{
  std::vector<DataPath> childDataObjects;
  const DataObject* dataObject1 = dataStructure.getData(parent);
  if(dataObject1 == nullptr || dataObject1->getDataObjectType() == DataObject::Type::DataArray || dataObject1->getDataObjectType() == DataObject::Type::DynamicListArray ||
     dataObject1->getDataObjectType() == DataObject::Type::NeighborList || dataObject1->getDataObjectType() == DataObject::Type::ScalarData ||
     dataObject1->getDataObjectType() == DataObject::Type::StringArray)
  {
    return {};
  }
  try
  {
    std::vector<std::string> childrenNames;
    if(parent.empty())
    {
      childrenNames = dataStructure.getDataMap().getNames();
    }
    else
    {
      childrenNames = dataStructure.getDataRefAs<BaseGroup>(parent).getDataMap().getNames();
    }

    for(const auto& childName : childrenNames)
    {
      DataPath childPath = parent.createChildPath(childName);
      childDataObjects.push_back(childPath);
    }
  } catch(std::exception& e)
  {
    return {};
  }
  return {childDataObjects};
}

std::optional<std::vector<DataPath>> GetAllChildArrayDataPaths(const DataStructure& dataStructure, const DataPath& parentGroup, const std::vector<DataPath>& ignoredDataPaths)
{
  return GetAllChildDataPathsOfType<IArray>(dataStructure, parentGroup, ignoredDataPaths);
}

std::optional<std::vector<DataPath>> GetAllChildDataPathsRecursive(const DataStructure& dataStructure, const DataPath& parentGroup, const std::vector<DataPath>& ignoredDataPaths)
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
      const auto* parent = dataStructure.getDataAs<BaseGroup>(parentGroup);
      if(parent == nullptr)
      {
        return {};
      }
      childrenNames = parent->getDataMap().getNames();
    }

    for(const auto& childName : childrenNames)
    {
      bool ignore = false;
      DataPath childPath = parentGroup.createChildPath(childName);
      for(const auto& ignoredPath : ignoredDataPaths)
      {
        if(childPath == ignoredPath)
        {
          ignore = true;
          break;
        }
      }
      if(!ignore)
      {
        childDataObjects.push_back(childPath);
        auto childPathChildren = GetAllChildDataPathsRecursive(dataStructure, childPath, ignoredDataPaths);
        if(childPathChildren.has_value())
        {
          childDataObjects.insert(childDataObjects.end(), childPathChildren.value().begin(), childPathChildren.value().end());
        }
      }
    }
  } catch(std::exception& e)
  {
    return {};
  }
  return {childDataObjects};
}

std::optional<std::vector<DataPath>> GetAllChildDataPathsRecursive(const DataStructure& dataStructure, const DataPath& parentGroup, DataObject::Type dataObjectType,
                                                                   const std::vector<DataPath>& ignoredDataPaths)
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
      const auto* parent = dataStructure.getDataAs<BaseGroup>(parentGroup);
      if(parent == nullptr)
      {
        return {};
      }
      childrenNames = parent->getDataMap().getNames();
    }

    for(const auto& childName : childrenNames)
    {
      bool ignore = false;
      DataPath childPath = parentGroup.createChildPath(childName);
      for(const auto& ignoredPath : ignoredDataPaths)
      {
        if(childPath == ignoredPath)
        {
          ignore = true;
          break;
        }
      }
      if(!ignore)
      {
        const DataObject* dataObject = dataStructure.getData(childPath);
        if(dataObject != nullptr && (dataObjectType == DataObject::Type::DataObject || dataObject->getDataObjectType() == dataObjectType))
        {
          childDataObjects.push_back(childPath);
        }
        auto childPathChildren = GetAllChildDataPathsRecursive(dataStructure, childPath, dataObjectType, ignoredDataPaths);
        if(childPathChildren.has_value())
        {
          childDataObjects.insert(childDataObjects.end(), childPathChildren.value().begin(), childPathChildren.value().end());
        }
      }
    }
  } catch(std::exception& e)
  {
    return {};
  }
  return {childDataObjects};
}

bool ContainsDataArrayName(const DataStructure& dataStructure, const DataPath& parentGroup, const std::string& arrayName)
{
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
      DataPath childPath = parentGroup.createChildPath(childName);
      const DataObject* dataObject = dataStructure.getData(childPath);
      if(dynamic_cast<const IDataArray*>(dataObject) != nullptr && childName == arrayName)
      {
        return true;
      }
    }
  } catch(std::exception& e)
  {
    return false;
  }
  return false;
}

} // namespace nx::core
