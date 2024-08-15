#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

using namespace nx::core;

namespace
{
template <typename T>
void FillBadDataUpdateTuples(const Int32AbstractDataStore& featureIds, AbstractDataStore<T>& outputDataStore, const std::vector<int32>& neighbors)
{
  usize start = 0;
  usize stop = outputDataStore.getNumberOfTuples();
  const usize numComponents = outputDataStore.getNumberOfComponents();
  for(usize tupleIndex = start; tupleIndex < stop; tupleIndex++)
  {
    const int32 featureName = featureIds[tupleIndex];
    const int32 neighbor = neighbors[tupleIndex];
    if(neighbor == tupleIndex)
    {
      continue;
    }

    if(featureName < 0 && neighbor != -1 && featureIds[static_cast<size_t>(neighbor)] > 0)
    {
      for(usize i = 0; i < numComponents; i++)
      {
        auto value = outputDataStore[neighbor * numComponents + i];
        outputDataStore[tupleIndex * numComponents + i] = value;
      }
    }
  }
}

struct FillBadDataUpdateTuplesFunctor
{
  template <typename T>
  void operator()(const Int32AbstractDataStore& featureIds, IDataArray* outputIDataArray, const std::vector<int32>& neighbors)
  {
    auto& outputStore = outputIDataArray->getIDataStoreRefAs<AbstractDataStore<T>>();
    FillBadDataUpdateTuples(featureIds, outputStore, neighbors);
  }
};
} // namespace

// -----------------------------------------------------------------------------
FillBadData::FillBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FillBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
FillBadData::~FillBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& FillBadData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> FillBadData::operator()()
{
  auto& featureIdsStore = m_DataStructure.getDataAs<Int32Array>(m_InputValues->featureIdsArrayPath)->getDataStoreRef();
  const size_t totalPoints = featureIdsStore.getNumberOfTuples();

  std::vector<int32> neighbors(totalPoints, -1);

  std::vector<bool> alreadyChecked(totalPoints, false);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->inputImageGeometry);

  const SizeVec3 udims = selectedImageGeom.getDimensions();

  Int32Array* cellPhasesPtr = nullptr;

  if(m_InputValues->storeAsNewPhase)
  {
    cellPhasesPtr = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  }

  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  size_t count = 1;
  // int32_t good = 1;
  // int64_t neighbor = 0;
  // int64_t index = 0;
  // float32 x = 0.0F;
  //  float32 y = 0.0F;
  //  float32 z = 0.0F;
  // int64 column = 0;
  // int64 row = 0;
  // int64 plane = 0;
  //  int64 neighborPoint = 0;
  //  int32 featureName = 0;
  //  int32 feature = 0;
  size_t numFeatures = 0;
  size_t maxPhase = 0;

  for(size_t i = 0; i < totalPoints; i++)
  {
    int32 featureName = featureIdsStore[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  if(m_InputValues->storeAsNewPhase)
  {
    for(size_t i = 0; i < totalPoints; i++)
    {
      if((*cellPhasesPtr)[i] > maxPhase)
      {
        maxPhase = (*cellPhasesPtr)[i];
      }
    }
  }

  std::array<int64_t, 6> neighborPoints = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};
  std::vector<int64_t> currentVisitedList;

  for(size_t iter = 0; iter < totalPoints; iter++)
  {
    alreadyChecked[iter] = false;
    if(featureIdsStore[iter] != 0)
    {
      alreadyChecked[iter] = true;
    }
  }

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(!alreadyChecked[i] && featureIdsStore[i] == 0)
    {
      currentVisitedList.push_back(static_cast<int64_t>(i));
      count = 0;
      while(count < currentVisitedList.size())
      {
        int64_t index = currentVisitedList[count];
        int64 column = index % dims[0];
        int64 row = (index / dims[0]) % dims[1];
        int64 plane = index / (dims[0] * dims[1]);
        for(int32_t j = 0; j < 6; j++)
        {
          int64_t neighbor = index + neighborPoints[j];
          if(j == 0 && plane == 0)
          {
            continue;
          }
          if(j == 5 && plane == (dims[2] - 1))
          {
            continue;
          }
          if(j == 1 && row == 0)
          {
            continue;
          }
          if(j == 4 && row == (dims[1] - 1))
          {
            continue;
          }
          if(j == 2 && column == 0)
          {
            continue;
          }
          if(j == 3 && column == (dims[0] - 1))
          {
            continue;
          }
          if(featureIdsStore[neighbor] == 0 && !alreadyChecked[neighbor])
          {
            currentVisitedList.push_back(neighbor);
            alreadyChecked[neighbor] = true;
          }
        }
        count++;
      }
      if((int32_t)currentVisitedList.size() >= m_InputValues->minAllowedDefectSizeValue)
      {
        for(const auto& currentIndex : currentVisitedList)
        {
          featureIdsStore[currentIndex] = 0;
          if(m_InputValues->storeAsNewPhase)
          {
            (*cellPhasesPtr)[currentIndex] = static_cast<int32>(maxPhase) + 1;
          }
        }
      }
      if((int32_t)currentVisitedList.size() < m_InputValues->minAllowedDefectSizeValue)
      {
        for(const auto& currentIndex : currentVisitedList)
        {
          featureIdsStore[currentIndex] = -1;
        }
      }
      currentVisitedList.clear();
    }
  }

  std::vector<int32_t> featureNumber(numFeatures + 1, 0);

  while(count != 0)
  {
    count = 0;
    for(size_t i = 0; i < totalPoints; i++)
    {
      int32 featureName = featureIdsStore[i];
      if(featureName < 0)
      {
        count++;
        // int32 current = 0;
        int32 most = 0;
        auto xIndex = static_cast<float32>(i % dims[0]);
        auto yIndex = static_cast<float32>((i / dims[0]) % dims[1]);
        auto zIndex = static_cast<float32>(i / (dims[0] * dims[1]));
        for(int32_t j = 0; j < 6; j++)
        {
          auto neighborPoint = static_cast<int64_t>(i + neighborPoints[j]);
          if(j == 0 && zIndex == 0)
          {
            continue;
          }
          if(j == 5 && zIndex == static_cast<float32>(dims[2] - 1))
          {
            continue;
          }
          if(j == 1 && yIndex == 0)
          {
            continue;
          }
          if(j == 4 && yIndex == static_cast<float32>(dims[1] - 1))
          {
            continue;
          }
          if(j == 2 && xIndex == 0)
          {
            continue;
          }
          if(j == 3 && xIndex == static_cast<float32>(dims[0] - 1))
          {
            continue;
          }

          int32 feature = featureIdsStore[neighborPoint];
          if(feature > 0)
          {
            featureNumber[feature]++;
            int32 current = featureNumber[feature];
            if(current > most)
            {
              most = current;
              neighbors[i] = static_cast<int32>(neighborPoint);
            }
          }
        }
        for(int32_t j = 0; j < 6; j++)
        {
          int64 neighborPoint = static_cast<int64>(i) + neighborPoints[j];
          if(j == 0 && zIndex == 0)
          {
            continue;
          }
          if(j == 5 && zIndex == static_cast<float32>(dims[2] - 1))
          {
            continue;
          }
          if(j == 1 && yIndex == 0)
          {
            continue;
          }
          if(j == 4 && yIndex == static_cast<float32>(dims[1] - 1))
          {
            continue;
          }
          if(j == 2 && xIndex == 0)
          {
            continue;
          }
          if(j == 3 && xIndex == static_cast<float32>(dims[0] - 1))
          {
            continue;
          }

          int32 feature = featureIdsStore[neighborPoint];
          if(feature > 0)
          {
            featureNumber[feature] = 0;
          }
        }
      }
    }

    std::optional<std::vector<DataPath>> allChildArrays = GetAllChildDataPaths(m_DataStructure, selectedImageGeom.getCellDataPath(), DataObject::Type::DataArray, m_InputValues->ignoredDataArrayPaths);
    std::vector<DataPath> voxelArrayNames;
    if(allChildArrays.has_value())
    {
      voxelArrayNames = allChildArrays.value();
    }

    for(const auto& cellArrayPath : voxelArrayNames)
    {
      if(cellArrayPath == m_InputValues->featureIdsArrayPath)
      {
        continue;
      }
      auto* oldCellArray = m_DataStructure.getDataAs<IDataArray>(cellArrayPath);

      ExecuteDataFunction(FillBadDataUpdateTuplesFunctor{}, oldCellArray->getDataType(), featureIdsStore, oldCellArray, neighbors);
    }

    // We need to update the FeatureIds array _LAST_ since the above operations depend on that values in that array
    FillBadDataUpdateTuples<int32>(featureIdsStore, featureIdsStore, neighbors);
  }
  return {};
}
