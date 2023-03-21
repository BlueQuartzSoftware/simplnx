#include "ErodeDilateBadData.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Utilities/DataGroupUtilities.hpp"
#include "complex/Utilities/MessageUtilities.hpp"
#include "complex/Utilities/ParallelTaskAlgorithm.hpp"

using namespace complex;
namespace
{
class ErodeDilateBadDataTransferDataImpl
{
public:
  ErodeDilateBadDataTransferDataImpl() = delete;
  ErodeDilateBadDataTransferDataImpl(const ErodeDilateBadDataTransferDataImpl&) = default;

  ErodeDilateBadDataTransferDataImpl(ChoicesParameter::ValueType operation, const Int32Array& featureIds, const std::vector<int64>& neighbors, const std::shared_ptr<IDataArray>& dataArrayPtr,
                                     const std::atomic_bool& shouldCancel, ThreadSafeTaskMessenger& messenger)
  : m_Operation(operation)
  , m_FeatureIds(featureIds)
  , m_Neighbors(neighbors)
  , m_DataArrayPtr(dataArrayPtr)
  , m_ShouldCancel(shouldCancel)
  , m_Messenger(messenger)
  {
  }
  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadDataTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(const ErodeDilateBadDataTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(ErodeDilateBadDataTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~ErodeDilateBadDataTransferDataImpl() = default;

  void operator()() const
  {
    const auto arrayID = m_DataArrayPtr->getId();
    const auto progressIncrement = m_Messenger.getProgressIncrement(arrayID);
    const auto totalPoints = m_Messenger.getTotalElements(arrayID);
    usize progressCount = 0;

    for(usize i = 0; i < totalPoints; i++)
    {
      if(m_ShouldCancel)
      {
        return;
      }

      const int32 featureName = m_FeatureIds[i];
      const int64 neighbor = m_Neighbors[i];
      if(neighbor >= 0)
      {
        if((featureName == 0 && m_FeatureIds[neighbor] > 0 && m_Operation == k_ErodeIndex) || (featureName > 0 && m_FeatureIds[neighbor] == 0 && m_Operation == k_DilateIndex))
        {
          m_DataArrayPtr->copyTuple(neighbor, i);
        }
      }

      progressCount++;
      if(progressCount > progressIncrement)
      {
        m_Messenger.updateProgress(progressCount, arrayID);
      }
    }
    m_Messenger.updateProgress(progressCount, arrayID);
  }

private:
  ChoicesParameter::ValueType m_Operation = 0;
  std::vector<int64> m_Neighbors;
  const std::shared_ptr<IDataArray> m_DataArrayPtr;
  const Int32Array& m_FeatureIds;
  ThreadSafeTaskMessenger& m_Messenger;
  const std::atomic_bool& m_ShouldCancel;
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
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const usize totalPoints = featureIds.getNumberOfTuples();

  std::vector<int64> neighbors(totalPoints, -1);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  usize numFeatures = 0;
  for(usize i = 0; i < totalPoints; i++)
  {
    const int32 featureName = featureIds[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  std::array<int64, 6> neighpoints = {-dims[0] * dims[1], -dims[0], -1, 1, dims[0], dims[0] * dims[1]};

  std::vector<int32> featureCount(numFeatures + 1, 0);

  for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    for(int64 zIndex = 0; zIndex < dims[2]; zIndex++)
    {
      const int64 zStride = dims[0] * dims[1] * zIndex;
      for(int64 yIndex = 0; yIndex < dims[1]; yIndex++)
      {
        const int64 yStride = dims[0] * yIndex;
        for(int64 xIndex = 0; xIndex < dims[0]; xIndex++)
        {
          const int64 voxelIndex = zStride + yStride + xIndex;
          const int32 featureName = featureIds[voxelIndex];
          if(featureName == 0)
          {
            int32 most = 0;
            for(int32 neighPointIdx = 0; neighPointIdx < 6; neighPointIdx++)
            {
              const int64 neighborPoint = voxelIndex + neighpoints[neighPointIdx];
              if(neighPointIdx == 0 && (zIndex == 0 || !m_InputValues->ZDirOn))
              {
                continue;
              }
              if(neighPointIdx == 5 && (zIndex == (dims[2] - 1) || !m_InputValues->ZDirOn))
              {
                continue;
              }
              if(neighPointIdx == 1 && (yIndex == 0 || !m_InputValues->YDirOn))
              {
                continue;
              }
              if(neighPointIdx == 4 && (yIndex == (dims[1] - 1) || !m_InputValues->YDirOn))
              {
                continue;
              }
              if(neighPointIdx == 2 && (xIndex == 0 || !m_InputValues->XDirOn))
              {
                continue;
              }
              if(neighPointIdx == 3 && (xIndex == (dims[0] - 1) || !m_InputValues->XDirOn))
              {
                continue;
              }

              const int32 feature = featureIds[neighborPoint];
              if(m_InputValues->Operation == ::k_DilateIndex && feature > 0)
              {
                neighbors[neighborPoint] = voxelIndex;
              }
              if(feature > 0 && m_InputValues->Operation == k_ErodeIndex)
              {
                featureCount[feature]++;
                const int32 current = featureCount[feature];
                if(current > most)
                {
                  most = current;
                  neighbors[voxelIndex] = neighborPoint;
                }
              }
            }
            if(m_InputValues->Operation == k_ErodeIndex)
            {
              for(int32 neighPointIdx = 0; neighPointIdx < 6; neighPointIdx++)
              {
                const int64 neighborPoint = voxelIndex + neighpoints[neighPointIdx];
                if(neighPointIdx == 0 && zIndex == 0)
                {
                  continue;
                }
                if(neighPointIdx == 5 && zIndex == (dims[2] - 1))
                {
                  continue;
                }
                if(neighPointIdx == 1 && yIndex == 0)
                {
                  continue;
                }
                if(neighPointIdx == 4 && yIndex == (dims[1] - 1))
                {
                  continue;
                }
                if(neighPointIdx == 2 && xIndex == 0)
                {
                  continue;
                }
                if(neighPointIdx == 3 && xIndex == (dims[0] - 1))
                {
                  continue;
                }

                const int32 feature = featureIds[neighborPoint];
                featureCount[feature] = 0;
              }
            }
          }
        }
      }
    }

    // Build up a list of the DataArrays that we are going to operate on.
    const std::vector<std::shared_ptr<IDataArray>> voxelArrays = complex::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

    ParallelTaskAlgorithm taskRunner;
    ThreadSafeTaskMessenger messenger(m_MessageHandler);
    for(const auto& voxelArray : voxelArrays)
    {
      // We need to skip updating the FeatureIds until all the other arrays are updated
      // since we actually depend on the feature Ids values.
      if(voxelArray->getName() == m_InputValues->FeatureIdsArrayPath.getTargetName())
      {
        continue;
      }
      messenger.addArray(voxelArray->getId(), totalPoints, voxelArray->getName());
      taskRunner.execute(ErodeDilateBadDataTransferDataImpl(m_InputValues->Operation, featureIds, neighbors, voxelArray, getCancel(), messenger));
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

    // Now update the feature Ids
    auto featureIDataArray = m_DataStructure.getSharedDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
    messenger.addArray(featureIDataArray->getId(), totalPoints, featureIDataArray->getName());
    taskRunner.setParallelizationEnabled(false); // Do this to make the next call synchronous
    taskRunner.execute(ErodeDilateBadDataTransferDataImpl(m_InputValues->Operation, featureIds, neighbors, featureIDataArray, getCancel(), messenger));
  }

  return {};
}
