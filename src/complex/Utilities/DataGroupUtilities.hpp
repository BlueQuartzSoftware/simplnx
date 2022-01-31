#pragma once

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataMap.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"

#include <vector>

namespace complex
{

/**
 * @brief RemoveInactiveObjects This assumes a single Dimension TupleShape, i.e., a Linear array, (1D).
 *
 * @param dataStructure
 * @param featureDataGroupPath
 * @param activeObjects
 * @param cellFeatureIds
 * @return
 */
bool RemoveInactiveObjects(DataStructure& dataStructure, DataPath& featureDataGroupPath, const std::vector<bool>& activeObjects, Int32Array* cellFeatureIds, size_t currentFeatureCount)
{
  std::cout << "RemoveInactiveObjects() Starting for featureDataGroupPath = " << featureDataGroupPath.toString() << std::endl;
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
    std::cout << "    featureLeveGroup was null" << std::endl;
    return false;
  }
  const DataMap& featureDataMap = featureLevelBaseGroup->getDataMap();
  std::cout << "    featureDataMap.getSize() = " << featureDataMap.getSize() << std::endl;

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
      std::cout << "        " << dataArray->getName() << "  dataArray->getNumberOfTuples() = " << dataArray->getNumberOfTuples() << std::endl;
      if(dataArray->getNumberOfTuples() == activeObjects.size())
      {
        matchingDataArrayPtrs.push_back(dataArray);
      }
    }
  }
  std::cout << "    matchingDataArrayPtrs.size() = " << matchingDataArrayPtrs.size() << std::endl;
  std::cout << "    activeObjects.size() = " << activeObjects.size() << std::endl;

  size_t totalTuples = currentFeatureCount;
  if(activeObjects.size() == totalTuples && acceptableMatrix)
  {
    size_t goodcount = 1;
    std::vector<size_t> newNames(totalTuples, 0);
    std::vector<size_t> removeList;
    std::vector<size_t> keepList;
    keepList.reserve(activeObjects.size());

    std::cout << "   Finding the keep/reject index list" << std::endl;

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

    std::cout << "    removeList.size: " << removeList.size() << std::endl;
    std::cout << "    keepList.size: " << keepList.size() << std::endl;

    if(!removeList.empty())
    {

      for(const auto& dataArray : matchingDataArrayPtrs)
      {
        std::cout << "    Removing Inactive Objects from " << dataArray->getName() << std::endl;

        // IDataArray* p = dataStructure.getDataAs<IDataArray>(header);

        std::string typeName = dataArray->getTypeName();
        if(typeName == "NeighborList<T>")
        {
          dataStructure.removeData(dataArray->getId());
        }
        else
        {
          std::cout << "    Removing Tuples through copy/modify algorithm.." << std::endl;
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
          std::cout << "    Removing original DataArray: " << dataArray->getNumberOfTuples() << std::endl;
          dataStructure.removeData(dataArray->getId());
          // Add in the modified DataArray to the "Feature Data Group"
          // Now chop off the end of the copy and modified array
          copy->getIDataStore()->reshapeTuples({keepList.size()});
          std::cout << "    Add resized original DataArray: " << copy->getNumberOfTuples() << std::endl;
          if(!dataStructure.insert(copy, featureDataGroupPath))
          {
            DataPath copyDataPath = featureDataGroupPath.createChildPath(copy->getName());
            std::cout << "    ! Unable to insert DataArray at " << copyDataPath.toString() << std::endl;
          }
        }
      }
      //      std::vector<size_t> tDims = {totalTuples - removeList.size()};
      //      setTupleDimensions(tDims);

      // Loop over all the points and correct all the feature names
      size_t totalPoints = cellFeatureIds->getNumberOfTuples();
      Int32DataStore& featureIdPtr = cellFeatureIds->getIDataStoreRefAs<Int32DataStore>();
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
