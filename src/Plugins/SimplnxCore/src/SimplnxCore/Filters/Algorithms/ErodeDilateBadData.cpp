#include "ErodeDilateBadData.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/DataStoreUtilities.hpp"

using namespace nx::core;
namespace
{
constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;

class ErodeDilateBadDataTransferDataImpl
{
public:
  ErodeDilateBadDataTransferDataImpl() = delete;
  ErodeDilateBadDataTransferDataImpl(const ErodeDilateBadDataTransferDataImpl&) = default;

  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadData* filterAlg, usize totalPoints, ChoicesParameter::ValueType operation, const Int32AbstractDataStore& featureIds,
                                     const std::vector<int64>& neighbors, const std::shared_ptr<IDataArray>& dataArrayPtr, MessageHelper& messageHelper)
  : m_FilterAlg(filterAlg)
  , m_TotalPoints(totalPoints)
  , m_Operation(operation)
  , m_Neighbors(neighbors)
  , m_DataArrayPtr(dataArrayPtr)
  , m_FeatureIds(featureIds)
  , m_MessageHelper(messageHelper)
  {
  }
  ErodeDilateBadDataTransferDataImpl(ErodeDilateBadDataTransferDataImpl&&) = default;                // Move Constructor Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(const ErodeDilateBadDataTransferDataImpl&) = delete; // Copy Assignment Not Implemented
  ErodeDilateBadDataTransferDataImpl& operator=(ErodeDilateBadDataTransferDataImpl&&) = delete;      // Move Assignment Not Implemented

  ~ErodeDilateBadDataTransferDataImpl() = default;

  void operator()() const
  {
    ThrottledMessenger throttledMessenger = m_MessageHelper.createThrottledMessenger();
    std::string arrayName = m_DataArrayPtr->getName();
    for(usize i = 0; i < m_TotalPoints; i++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Processing {}: {:.2f}% completed", arrayName, CalculatePercentComplete(i, m_TotalPoints)); });

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
  MessageHelper& m_MessageHelper;
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
bool shouldSkipData(const ErodeDilateBadDataInputValues* inputValues, int32 neighPointIdx, const std::array<int64, 3>& dims, int64 xIndex, int64 yIndex, int64 zIndex)
{
  if(neighPointIdx == 0 && (zIndex == 0 || !inputValues->ZDirOn))
  {
    return true;
  }
  if(neighPointIdx == 5 && (zIndex == (dims[2] - 1) || !inputValues->ZDirOn))
  {
    return true;
  }
  if(neighPointIdx == 1 && (yIndex == 0 || !inputValues->YDirOn))
  {
    return true;
  }
  if(neighPointIdx == 4 && (yIndex == (dims[1] - 1) || !inputValues->YDirOn))
  {
    return true;
  }
  if(neighPointIdx == 2 && (xIndex == 0 || !inputValues->XDirOn))
  {
    return true;
  }
  if(neighPointIdx == 3 && (xIndex == (dims[0] - 1) || !inputValues->XDirOn))
  {
    return true;
  }

  return false;
}

/**
 * @brief Parses over the neighbor indices and sets the feature counts to 0 for existing points.
 * @param featureIds Feature ID data store for determining neighboring feature IDs.
 * @param featureCount Running total of the number of features it neighbors.
 * @param neighpoints Pre-created array for determining neighbor indices.
 * @param dims Geometry dimentions for determining boundaries.
 * @param voxelIndex Array index of the 3D geometry position.
 * @param xIndex X index in the geometry position used for determing neighbor validity.
 * @param yIndex Y index in the geometry position used for determing neighbor validity.
 * @param zIndex Z index in the geometry position used for determing neighbor validity.
 */
inline void ErodeBadDataPostOp(const Int32AbstractDataStore& featureIds, std::vector<int32>& featureCount, const std::array<int64, k_NumFaceNeighbors>& neighpoints,
                               const std::array<FaceNeighborType, k_NumFaceNeighbors>& faceNeighborInternalIndex, const std::array<int64, 3>& dims, const int64 voxelIndex,
                        int64 xIndex, int64 yIndex, int64 zIndex)
{
  // Loop over the 6 face neighbors of the voxel
  const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIndex, yIndex, zIndex, dims);
  for (const auto& faceIndex : faceNeighborInternalIndex)
  {
    if(!isValidFaceNeighbor[faceIndex])
    {
      continue;
    }
    const int64 neighborPoint = voxelIndex + neighpoints[faceIndex];
    const int32 feature = featureIds[neighborPoint];
    featureCount[feature] = 0;
  }
}

/**
 * @brief
 * @param inputValues Algorithm input values
 * @param featureIds Feature ID data store.
 * @param featureCount Running total of the number of features it neighbors.
 * @param neighbors
 * @param neighpoints Pre-created array for determining neighbor indices.
 * @param dims Geometry dimentions for determining boundaries.
 * @param voxelIndex Array index of the 3D geometry position.
 * @param xIndex X index in the geometry position used for determing neighbor validity.
 * @param yIndex Y index in the geometry position used for determing neighbor validity.
 * @param zIndes Z index in the geometry position used for determing neighbor validity.
 */
inline void erodeDilateBadDataVoxel(const ErodeDilateBadDataInputValues* inputValues, const Int32AbstractDataStore& featureIds, std::vector<int32>& featureCount, std::vector<int64>& neighbors,
                                    const std::array<int64, k_NumFaceNeighbors>& neighpoints, const std::array<FaceNeighborType, k_NumFaceNeighbors>& faceNeighborInternalIndex,
                                    const std::array<int64, 3>& dims, int64 voxelIndex, int64 xIndex,
                                    int64 yIndex, int64 zIndex)
{
  const int32 featureName = featureIds[voxelIndex];
  if(featureName == 0)
  {
    int32 most = 0;
    // Loop over the 6 face neighbors of the voxel
    const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIndex, yIndex, zIndex, dims);
    for (const auto& faceIndex : faceNeighborInternalIndex)
    {
      if(!isValidFaceNeighbor[faceIndex])
      {
        continue;
      }
      const int64 neighborPoint = voxelIndex + neighpoints[faceIndex];
      const int32 feature = featureIds[neighborPoint];
      if(inputValues->Operation == detail::k_DilateIndex && feature > 0)
      {
        neighbors[neighborPoint] = voxelIndex;
      }
      if(feature > 0 && inputValues->Operation == detail::k_ErodeIndex)
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

    // Erode operation
    if(inputValues->Operation == detail::k_ErodeIndex)
    {
      ErodeBadDataPostOp(featureIds, featureCount, neighpoints, faceNeighborInternalIndex, dims, voxelIndex, xIndex, yIndex, zIndex);
    }
  }
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateBadData::operator()()
{
  const auto& featureIds = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  const usize totalPoints = featureIds.getNumberOfTuples();

  // Update for OOC data sizes
  std::vector<int64> neighbors(totalPoints, -1);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  usize numFeatures = std::max(0, *(std::max_element(featureIds.begin(), featureIds.end())));

  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<int32> featureCount(numFeatures + 1, 0);

  // Iterate over the geometry to handle every voxel
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
          erodeDilateBadDataVoxel(m_InputValues, featureIds, featureCount, neighbors, neighborVoxelIndexOffsets, faceNeighborInternalIdx, dims, voxelIndex, xIndex, yIndex, zIndex);
        }
      }
    }

    // Build up a list of the DataArrays that we are going to operate on.
    const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

    MessageHelper messageHelper(m_MessageHandler);

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

      taskRunner.execute(ErodeDilateBadDataTransferDataImpl(this, totalPoints, m_InputValues->Operation, featureIds, neighbors, voxelArray, messageHelper));
    }
    taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

    // Now update the feature Ids
    auto featureIDataArray = m_DataStructure.getSharedDataAs<IDataArray>(m_InputValues->FeatureIdsArrayPath);
    taskRunner.setParallelizationEnabled(false); // Do this to make the next call synchronous
    taskRunner.execute(ErodeDilateBadDataTransferDataImpl(this, totalPoints, m_InputValues->Operation, featureIds, neighbors, featureIDataArray, messageHelper));
  }

  return {};
}
