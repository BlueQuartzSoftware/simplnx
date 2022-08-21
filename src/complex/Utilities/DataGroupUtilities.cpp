#include "DataGroupUtilities.hpp"

#include "complex/DataStructure/BaseGroup.hpp"

namespace complex
{
bool RemoveInactiveObjects(DataStructure& dataStructure, DataPath& featureDataGroupPath, const std::vector<bool>& activeObjects, Int32Array& cellFeatureIds, size_t currentFeatureCount)
{
  bool acceptableMatrix = false;
  // Only valid for feature or ensemble type matrices
  //  if(m_Type == AttributeMatrix::Type::VertexFeature || m_Type == AttributeMatrix::Type::VertexEnsemble || m_Type == AttributeMatrix::Type::EdgeFeature ||
  //     m_Type == AttributeMatrix::Type::EdgeEnsemble || m_Type == AttributeMatrix::Type::FaceFeature || m_Type == AttributeMatrix::Type::FaceEnsemble || m_Type == AttributeMatrix::Type::CellFeature
  //     || m_Type == AttributeMatrix::Type::CellEnsemble)
  {
    acceptableMatrix = true;
  }

  // Get the DataGroup that holds all the feature Data
  const BaseGroup* featureLevelBaseGroup = dataStructure.getDataAs<const BaseGroup>(featureDataGroupPath);

  if(nullptr == featureLevelBaseGroup)
  {
    return false;
  }
  const DataMap& featureDataMap = featureLevelBaseGroup->getDataMap();

  // Loop over all the paths from the feature group and remove the data arrays that do NOT have the
  // same number of Tuples as the 'activeObjects' vector
  std::vector<std::shared_ptr<IDataArray>> matchingDataArrayPtrs;

  for(const auto& entry : featureDataMap)
  {
    DataObject::IdType entryId = entry.first;
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
  if(activeObjects.size() == totalTuples && acceptableMatrix)
  {
    size_t goodcount = 1;
    std::vector<size_t> newNames(totalTuples, 0);
    std::vector<size_t> removeList;
    std::vector<size_t> keepList;
    keepList.reserve(activeObjects.size());

    for(int32_t i = 1; i < activeObjects.size(); i++)
    {
      if(activeObjects[i])
      {
        newNames[i] = goodcount;
        goodcount++;
        keepList.push_back(i);
      }
      else
      {
        removeList.push_back(i);
        newNames[i] = 0;
      }
    }

    if(!removeList.empty())
    {
      for(const auto& dataArray : matchingDataArrayPtrs)
      {
        // Do the update "in place". This works because the keepList _should_ be sorted lowest to
        // highest. So we are constantly grabbing values from further in the array and copying
        // them to an closer to the front location in the array.
        size_t destIdx = 1;
        for(const auto& keepIdx : keepList)
        {
          dataArray->copyTuple(keepIdx, destIdx);
          destIdx++;
        }
        // Now chop off the end of the copy and modified array
        dataArray->getIDataStore()->reshapeTuples({keepList.size() + 1});
      }

      // Loop over all the points and correct all the feature names
      size_t totalPoints = cellFeatureIds.getNumberOfTuples();
      auto& featureIds = cellFeatureIds.getIDataStoreRefAs<Int32DataStore>();
      bool featureIdsChanged = false;
      for(size_t i = 0; i < totalPoints; i++)
      {
        if(featureIds[i] >= 0 && featureIds[i] < newNames.size())
        {
          featureIds[i] = static_cast<int32_t>(newNames[featureIds[i]]);
          featureIdsChanged = true;
        }
      }

      if(featureIdsChanged)
      {
        auto result = GetAllChildDataPaths(dataStructure, featureDataGroupPath, DataObject::Type::NeighborList);
        if(result.has_value())
        {
          std::vector<DataPath> neighborListDataPaths = result.value();
          for(const auto& neighborListDataPath : neighborListDataPaths)
          {
            dataStructure.removeData(neighborListDataPath);
          }
        }
      }
    }
  }
  else
  {
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
std::vector<std::shared_ptr<IDataArray>> GenerateDataArrayList(const DataStructure& dataStructure, const DataPath& dataArrayPath, const std::vector<DataPath>& ignoredDataPaths)
{
  std::vector<std::shared_ptr<IDataArray>> arrays;
  DataPath parentPath = dataArrayPath.getParent();
  const auto& parent = dataStructure.getDataRefAs<BaseGroup>(parentPath);
  auto childArrays = parent.findAllChildrenOfType<IDataArray>();
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

std::optional<std::vector<DataPath>> GetAllChildDataPaths(const DataStructure& dataStructure, const DataPath& parentGroup, DataObject::Type dataObjectType)
{
  std::vector<DataPath> childDataObjects;
  try
  {
    const auto& featureAttributeMatrix = dataStructure.getDataRefAs<BaseGroup>(parentGroup); // this may throw.
    std::vector<std::string> childrenNames = featureAttributeMatrix.getDataMap().getNames();

    for(const auto& childName : childrenNames)
    {
      DataPath childPath = parentGroup.createChildPath(childName);
      const DataObject* dataObject = dataStructure.getData(childPath);
      if(dataObject->getDataObjectType() == dataObjectType)
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

} // namespace complex
