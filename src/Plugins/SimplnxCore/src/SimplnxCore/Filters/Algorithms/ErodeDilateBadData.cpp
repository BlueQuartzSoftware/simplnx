#include "ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

using namespace nx::core;
namespace
{
class ErodeDilateBadDataTransferDataImpl
{
public:
  ErodeDilateBadDataTransferDataImpl() = delete;
  ErodeDilateBadDataTransferDataImpl(const ErodeDilateBadDataTransferDataImpl&) = default;

  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadData* filterAlg, usize totalPoints, ChoicesParameter::ValueType operation, const Int32AbstractDataStore& featureIds,
                                     const std::vector<int64>& neighbors, const std::shared_ptr<IDataArray>& dataArrayPtr)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_Operation(operation)
  , m_Neighbors(neighbors)
  , m_DataArrayPtr(dataArrayPtr)
  , m_FeatureIds(featureIds)
  {
  }
  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadDataTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(const ErodeDilateBadDataTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(ErodeDilateBadDataTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~ErodeDilateBadDataTransferDataImpl() = default;

  void operator()() const
  {
    const std::string arrayName = m_DataArrayPtr->getName();
    // Pre-gate on a count so the shared progress mutex is taken about a hundred times per array
    // rather than once per voxel. Matches RequireMinimumSizeFeatures.
    const usize progressIncrement = std::max(m_TotalPoints / 100ULL, 1ULL);
    for(usize i = 0; i < m_TotalPoints; i++)
    {
      if(i % progressIncrement == 0)
      {
        m_FilterAlg->sendThreadSafeProgressMessage(fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_TotalPoints)));
      }

      const int32 featureName = m_FeatureIds[i];
      const int64 neighbor = m_Neighbors[i];
      if(neighbor >= 0)
      {
        if((featureName == 0 && m_FeatureIds[neighbor] > 0 && m_Operation == detail::k_ErodeIndex) || (featureName > 0 && m_FeatureIds[neighbor] == 0 && m_Operation == detail::k_DilateIndex))
        {
          m_DataArrayPtr->copyTuple(neighbor, i);
        }
      }
    }
  }

private:
  ErodeDilateBadData* m_FilterAlg = nullptr;
  usize m_TotalPoints = 0;
  ChoicesParameter::ValueType m_Operation = 0;
  const std::vector<int64>& m_Neighbors;
  const std::shared_ptr<IDataArray> m_DataArrayPtr;
  const Int32AbstractDataStore& m_FeatureIds;
};

/**
 * @brief Masks out face neighbors whose axis has been disabled via the X/Y/Z Direction parameters.
 * Indices follow the VoxelNeighbors<Image3D> ordering: [-Z,-Y,-X,+X,+Y,+Z].
 * @param isValidFaceNeighbor Per-voxel face-neighbor validity, already computed from geometry boundary.
 * @param xDir Whether the X direction is enabled.
 * @param yDir Whether the Y direction is enabled.
 * @param zDir Whether the Z direction is enabled.
 */
void adjustValidNeighbors(std::array<bool, VoxelNeighbors<Image3D>::k_FaceNeighborCount>& isValidFaceNeighbor, bool xDir, bool yDir, bool zDir)
{
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeZNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeZNeighbor] && zDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeYNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeYNeighbor] && yDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeXNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_NegativeXNeighbor] && xDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveXNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveXNeighbor] && xDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveYNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveYNeighbor] && yDir;
  isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveZNeighbor] = isValidFaceNeighbor[VoxelNeighbors<Image3D>::k_PositiveZNeighbor] && zDir;
}
} // namespace

// -----------------------------------------------------------------------------
ErodeDilateBadData::ErodeDilateBadData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateBadDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
, m_Throttle(mesgHandler)
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
void ErodeDilateBadData::sendThreadSafeProgressMessage(const std::string& message)
{
  std::lock_guard<std::mutex> guard(m_ProgressMessage_Mutex);
  m_Throttle.trySendMessage(message);
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateBadData::operator()()
{
  const auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
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

  // Build up a list of the DataArrays that we are going to operate on.
  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<int32> featureCount(numFeatures + 1, 0);

  for(int32 iteration = 0; iteration < m_InputValues->NumIterations; iteration++)
  {
    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Check if the algorithm should cancel
      if(m_ShouldCancel)
      {
        return {};
      }

      const int64 zStride = dims[0] * dims[1] * zIdx;
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        const int64 yStride = dims[0] * yIdx;
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = zStride + yStride + xIdx;
          const int32 featureName = featureIds[voxelIndex];
          if(featureName == 0)
          {
            int32 most = 0;
            // Loop over the 6 face neighbors of the voxel
            std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);
            adjustValidNeighbors(isValidFaceNeighbor, m_InputValues->XDirOn, m_InputValues->YDirOn, m_InputValues->ZDirOn);
            for(const auto& faceIndex : faceNeighborInternalIdx)
            {
              if(!isValidFaceNeighbor[faceIndex])
              {
                continue;
              }
              const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

              const int32 feature = featureIds[neighborPoint];
              if(m_InputValues->Operation == detail::k_DilateIndex && feature > 0)
              {
                neighbors[neighborPoint] = voxelIndex;
              }
              if(feature > 0 && m_InputValues->Operation == detail::k_ErodeIndex)
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
            if(m_InputValues->Operation == detail::k_ErodeIndex)
            {
              // Loop over the 6 face neighbors of the voxel
              for(const auto& faceIndex : faceNeighborInternalIdx)
              {
                if(!isValidFaceNeighbor[faceIndex])
                {
                  continue;
                }
                const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];

                const int32 feature = featureIds[neighborPoint];
                featureCount[feature] = 0;
              }
            }
          }
        }
      }
    }

    ParallelTaskAlgorithm taskRunner;
    taskRunner.setParallelizationEnabled(true);
    for(const auto& voxelArray : voxelArrays)
    {
      // We need to skip updating the FeatureIds until all the other arrays are updated
      // since we actually depend on the feature Ids values.
      if(voxelArray->getName() == m_InputValues->FeatureIdsArrayPath.getTargetName())
      {
        continue;
      }

      taskRunner.execute(ErodeDilateBadDataTransferDataImpl(this, totalPoints, m_InputValues->Operation, featureIds, neighbors, voxelArray));
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

    // Now update the feature Ids
    auto featureIDataArray = m_DataStructure.getSharedDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
    taskRunner.setParallelizationEnabled(false); // Do this to make the next call synchronous
    taskRunner.execute(ErodeDilateBadDataTransferDataImpl(this, totalPoints, m_InputValues->Operation, featureIds, neighbors, featureIDataArray));
    taskRunner.wait(); // Redundant while parallelization is disabled, but keeps the "transfer is complete" invariant local.
  }

  return {};
}
