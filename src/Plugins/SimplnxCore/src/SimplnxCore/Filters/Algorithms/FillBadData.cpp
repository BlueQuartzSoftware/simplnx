#include "FillBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"

using namespace nx::core;

namespace
{

struct FillBadDataUpdateTuplesFunctor
{
  template <typename T>
  void operator()(const Int32Array& featureIds, IDataArray& outputIDataArray, const std::vector<int32>& neighbors)
  {
    using DataArrayType = DataArray<T>;

    DataArrayType outputArray = dynamic_cast<DataArrayType&>(outputIDataArray);
    size_t start = 0;
    size_t stop = outputArray.getNumberOfTuples();
    for(size_t tupleIndex = start; tupleIndex < stop; tupleIndex++)
    {
      const int32 featureName = featureIds[tupleIndex];
      const int32 neighbor = neighbors[tupleIndex];
      if(featureName < 0 && neighbor != -1 && featureIds[static_cast<size_t>(neighbor)] > 0)
      {
        outputArray.copyTuple(neighbor, tupleIndex);
      }
    }
  }
};

template <typename T>
class FillBadDataUpdateTuples
{
public:
  FillBadDataUpdateTuples(const Int32Array& featureIds, DataArray<T>& outputArray, const std::vector<int32>& neighbors)
  : m_FeatureIds(featureIds)
  , m_OutputArray(outputArray)
  , m_Neighbors(neighbors)
  {
  }
  ~FillBadDataUpdateTuples() = default;

  FillBadDataUpdateTuples(const FillBadDataUpdateTuples&) = default;           // Copy Constructor Not Implemented
  FillBadDataUpdateTuples(FillBadDataUpdateTuples&&) = delete;                 // Move Constructor Not Implemented
  FillBadDataUpdateTuples& operator=(const FillBadDataUpdateTuples&) = delete; // Copy Assignment Not Implemented
  FillBadDataUpdateTuples& operator=(FillBadDataUpdateTuples&&) = delete;      // Move Assignment Not Implemented

  void convert(size_t start, size_t stop) const
  {
    for(size_t tupleIndex = start; tupleIndex < stop; tupleIndex++)
    {
      const int32 featureName = m_FeatureIds[tupleIndex];
      const int32 neighbor = m_Neighbors[tupleIndex];
      if(featureName < 0 && neighbor != -1 && m_FeatureIds[static_cast<size_t>(neighbor)] > 0)
      {
        m_OutputArray.copyTuple(neighbor, tupleIndex);
      }
    }
  }

  void operator()() const
  {
    convert(0, m_OutputArray.getNumberOfTuples());
  }

private:
  const Int32Array& m_FeatureIds;
  DataArray<T>& m_OutputArray;
  const std::vector<int32>& m_Neighbors;
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
  auto& m_FeatureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->featureIdsArrayPath);
  const size_t totalPoints = m_FeatureIds.getNumberOfTuples();

  std::vector<int32> m_Neighbors(totalPoints, -1);

  std::vector<bool> m_AlreadyChecked(totalPoints, false);

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
    int32 featureName = m_FeatureIds[i];
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
    m_AlreadyChecked[iter] = false;
    if(m_FeatureIds[iter] != 0)
    {
      m_AlreadyChecked[iter] = true;
    }
  }

  for(size_t i = 0; i < totalPoints; i++)
  {
    if(!m_AlreadyChecked[i] && m_FeatureIds[i] == 0)
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
          if(m_FeatureIds[neighbor] == 0 && !m_AlreadyChecked[neighbor])
          {
            currentVisitedList.push_back(neighbor);
            m_AlreadyChecked[neighbor] = true;
          }
        }
        count++;
      }
      if((int32_t)currentVisitedList.size() >= m_InputValues->minAllowedDefectSizeValue)
      {
        for(const auto& currentIndex : currentVisitedList)
        {
          m_FeatureIds[currentIndex] = 0;
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
          m_FeatureIds[currentIndex] = -1;
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
      int32 featureName = m_FeatureIds[i];
      if(featureName < 0)
      {
        count++;
        // int32 current = 0;
        int32 most = 0;
        float32 xIndex = static_cast<float>(i % dims[0]);
        float32 yIndex = static_cast<float>((i / dims[0]) % dims[1]);
        float32 zIndex = static_cast<float>(i / (dims[0] * dims[1]));
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

          int32 feature = m_FeatureIds[neighborPoint];
          if(feature > 0)
          {
            featureNumber[feature]++;
            int32 current = featureNumber[feature];
            if(current > most)
            {
              most = current;
              m_Neighbors[i] = static_cast<int32>(neighborPoint);
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

          int32 feature = m_FeatureIds[neighborPoint];
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
      auto& oldCellArray = m_DataStructure.getDataRefAs<IDataArray>(cellArrayPath);
      const DataType dataType = oldCellArray.getDataType();

      ExecuteDataFunction(FillBadDataUpdateTuplesFunctor{}, dataType, m_FeatureIds, oldCellArray, m_Neighbors);
    }

    // We need to update the FeatureIds array _LAST_ since the above operations depend on that values in that array
    FillBadDataUpdateTuples<int32>(m_FeatureIds, dynamic_cast<Int32Array&>(m_FeatureIds), m_Neighbors)();
  }
  return {};
}
