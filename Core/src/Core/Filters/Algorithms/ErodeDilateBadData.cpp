#include "ErodeDilateBadData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;
namespace
{
class ErodeDilateBadDataTransferDataImpl
{
public:
  ErodeDilateBadDataTransferDataImpl() = delete;
  ErodeDilateBadDataTransferDataImpl(const ErodeDilateBadDataTransferDataImpl&) = default;

  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadData* filterAlg, size_t totalPoints, ChoicesParameter::ValueType operation, const Int32Array& featureIds, const std::vector<int32_t>& neighbors,
                                     const std::shared_ptr<IDataArray>& dataArrayPtr)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_Operation(operation)
  , m_FeatureIds(featureIds)
  , m_Neighbors(neighbors)
  , m_DataArrayPtr(dataArrayPtr)
  {
  }
  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadDataTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(const ErodeDilateBadDataTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(ErodeDilateBadDataTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~ErodeDilateBadDataTransferDataImpl() = default;

  void operator()() const
  {
    auto start = std::chrono::steady_clock::now();
    for(size_t i = 0; i < m_TotalPoints; i++)
    {
      auto now = std::chrono::steady_clock::now();
      //// Only send updates every 1 second
      if(std::chrono::duration_cast<std::chrono::milliseconds>(now - start).count() > 1000)
      {
        float progress = static_cast<float>(i) / static_cast<float>(m_TotalPoints) * 100.0f;
        m_FilterAlg->updateProgress(fmt::format("Processing {}: {}% completed", m_DataArrayPtr->getName(), static_cast<int>(progress)));
        start = std::chrono::steady_clock::now();
      }

      int32_t featureName = m_FeatureIds[i];
      int32_t neighbor = m_Neighbors[i];
      if(neighbor >= 0)
      {
        if((featureName == 0 && m_FeatureIds[neighbor] > 0 && m_Operation == k_DilateIndex) || (featureName > 0 && m_FeatureIds[neighbor] == 0 && m_Operation == k_ErodeIndex))
        {
          m_DataArrayPtr->copyTuple(neighbor, i);
        }
      }
    }
  }

private:
  ErodeDilateBadData* m_FilterAlg = nullptr;
  size_t m_TotalPoints = 0;
  ChoicesParameter::ValueType m_Operation = 0;
  std::vector<int32> m_Neighbors;
  const std::shared_ptr<IDataArray> m_DataArrayPtr;
  const Int32Array& m_FeatureIds;
};
} // namespace

// -----------------------------------------------------------------------------
ErodeDilateBadData::ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateBadData::~ErodeDilateBadData() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateBadData::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
void ErodeDilateBadData::updateProgress(const std::string& progMessage)
{
  m_MessageHandler({IFilter::Message::Type::Info, progMessage});
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateBadData::operator()()
{
  const auto& m_FeatureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  size_t totalPoints = m_FeatureIds.getNumberOfTuples();

  std::vector<int32> m_Neighbors(totalPoints, -1);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64_t, 3> dims = {
      static_cast<int64_t>(udims[0]),
      static_cast<int64_t>(udims[1]),
      static_cast<int64_t>(udims[2]),
  };

  int32_t good = 1;
  int32_t feature = 0;
  int32_t current = 0;
  int32_t most = 0;
  int64_t neighborPoint = 0;
  size_t numFeatures = 0;

  for(size_t i = 0; i < totalPoints; i++)
  {
    int32_t featureName = m_FeatureIds[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  int64_t neighpoints[6] = {neighpoints[0] = -dims[0] * dims[1], neighpoints[1] = -dims[0], neighpoints[2] = -1, neighpoints[3] = 1, neighpoints[4] = dims[0], neighpoints[5] = dims[0] * dims[1]};

  std::vector<int32_t> n(numFeatures + 1, 0);

  for(int32_t iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    for(int64_t k = 0; k < dims[2]; k++)
    {
      int64_t kstride = dims[0] * dims[1] * k;
      for(int64_t j = 0; j < dims[1]; j++)
      {
        int64_t jstride = dims[0] * j;
        for(int64_t i = 0; i < dims[0]; i++)
        {
          int64_t count = kstride + jstride + i;
          int32_t featureName = m_FeatureIds[count];
          if(featureName == 0)
          {
            current = 0;
            most = 0;
            for(int32_t neighPointIdx = 0; neighPointIdx < 6; neighPointIdx++)
            {
              good = 1;
              neighborPoint = count + neighpoints[neighPointIdx];
              if(neighPointIdx == 0 && (k == 0 || !m_InputValues->ZDirOn))
              {
                good = 0;
              }
              else if(neighPointIdx == 5 && (k == (dims[2] - 1) || !m_InputValues->ZDirOn))
              {
                good = 0;
              }
              else if(neighPointIdx == 1 && (j == 0 || !m_InputValues->YDirOn))
              {
                good = 0;
              }
              else if(neighPointIdx == 4 && (j == (dims[1] - 1) || !m_InputValues->YDirOn))
              {
                good = 0;
              }
              else if(neighPointIdx == 2 && (i == 0 || !m_InputValues->XDirOn))
              {
                good = 0;
              }
              else if(neighPointIdx == 3 && (i == (dims[0] - 1) || !m_InputValues->XDirOn))
              {
                good = 0;
              }
              if(good == 1)
              {
                feature = m_FeatureIds[neighborPoint];
                if(m_InputValues->Operation == ::k_ErodeIndex && feature > 0)
                {
                  m_Neighbors[neighborPoint] = count;
                }
                if(feature > 0 && m_InputValues->Operation == k_DilateIndex)
                {
                  n[feature]++;
                  current = n[feature];
                  if(current > most)
                  {
                    most = current;
                    m_Neighbors[count] = neighborPoint;
                  }
                }
              }
            }
            if(m_InputValues->Operation == k_DilateIndex)
            {
              for(int32_t neighPointIdx = 0; neighPointIdx < 6; neighPointIdx++)
              {
                good = 1;
                neighborPoint = count + neighpoints[neighPointIdx];
                if(neighPointIdx == 0 && k == 0)
                {
                  good = 0;
                }
                if(neighPointIdx == 5 && k == (dims[2] - 1))
                {
                  good = 0;
                }
                if(neighPointIdx == 1 && j == 0)
                {
                  good = 0;
                }
                if(neighPointIdx == 4 && j == (dims[1] - 1))
                {
                  good = 0;
                }
                if(neighPointIdx == 2 && i == 0)
                {
                  good = 0;
                }
                if(neighPointIdx == 3 && i == (dims[0] - 1))
                {
                  good = 0;
                }
                if(good == 1)
                {
                  feature = m_FeatureIds[neighborPoint];
                  n[feature] = 0;
                }
              }
            }
          }
        }
      }
    }

    // Build up a list of the DataArrays that we are going to operate on.
    std::vector<std::shared_ptr<IDataArray>> voxelArrays = complex::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

    ParallelTaskAlgorithm parallelTask;
    for(const auto& voxelArray : voxelArrays)
    {
      parallelTask.execute(ErodeDilateBadDataTransferDataImpl(this, totalPoints, m_InputValues->Operation, m_FeatureIds, m_Neighbors, voxelArray));
    }
  }

  return {};
}
