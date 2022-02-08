#include "DataGroupUtilities.hpp"

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

  // Get the DataGroup that holds all of the feature Data
  const BaseGroup* featureLevelBaseGroup = dataStructure.getDataAs<const BaseGroup>(featureDataGroupPath);

  if(nullptr == featureLevelBaseGroup)
  {
    return false;
  }
  const DataMap& featureDataMap = featureLevelBaseGroup->getDataMap();

  // Loop over all of the paths from the feature group and remove the data arrays that do NOT have the
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
      if(!activeObjects[i])
      {
        removeList.push_back(i);
        newNames[i] = 0;
      }
      else
      {
        newNames[i] = goodcount;
        goodcount++;
        keepList.push_back(i);
      }
    }

    if(!removeList.empty())
    {

      for(const auto& dataArray : matchingDataArrayPtrs)
      {
        // IDataArray* p = dataStructure.getDataAs<IDataArray>(header);
        std::string typeName = dataArray->getTypeName();
        if(typeName == "NeighborList<T>")
        {
          dataStructure.removeData(dataArray->getId());
        }
        else
        {
          DataObject* dataObjectCopy = dataArray->deepCopy();
          auto dataObjectCopyId = dataObjectCopy->getId();
          std::shared_ptr<IDataArray> copy = dataStructure.getSharedDataAs<IDataArray>(dataObjectCopyId);

          //          IDataArray* copy = dynamic_cast<IDataArray*>(dataObjectCopy);
          size_t destIdx = 0;
          for(const auto& keepIdx : keepList)
          {
            copy->copyTuple(keepIdx, destIdx);
            destIdx++;
          }
          // Remove the original array from the DataStructure
          dataStructure.removeData(dataArray->getId());
          // Add in the modified DataArray to the "Feature Data Group"
          // Now chop off the end of the copy and modified array
          copy->getIDataStore()->reshapeTuples({keepList.size()});
          if(!dataStructure.insert(copy, featureDataGroupPath))
          {
            DataPath copyDataPath = featureDataGroupPath.createChildPath(copy->getName());
          }
        }
      }

      // Loop over all the points and correct all the feature names
      size_t totalPoints = cellFeatureIds.getNumberOfTuples();
      Int32DataStore& featureIdPtr = cellFeatureIds.getIDataStoreRefAs<Int32DataStore>();
      for(size_t i = 0; i < totalPoints; i++)
      {
        if(featureIdPtr[i] >= 0 && featureIdPtr[i] < newNames.size())
        {
          featureIdPtr[i] = static_cast<int32_t>(newNames[featureIdPtr[i]]);
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
} // namespace complex
