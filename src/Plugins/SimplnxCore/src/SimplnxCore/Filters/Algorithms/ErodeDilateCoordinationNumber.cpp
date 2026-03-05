#include "ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"

using namespace nx::core;
namespace
{
struct DataArrayCopyTupleFunctor
{
  template <typename T>
  void operator()(IDataArray& outputIDataArray, size_t sourceIndex, size_t targetIndex)
  {
    using DataArrayType = DataArray<T>;
    DataArrayType& outputArray = dynamic_cast<DataArrayType&>(outputIDataArray);
    outputArray.copyTuple(sourceIndex, targetIndex);
  }
};
} // namespace

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::ErodeDilateCoordinationNumber(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                             ErodeDilateCoordinationNumberInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ErodeDilateCoordinationNumber::~ErodeDilateCoordinationNumber() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ErodeDilateCoordinationNumber::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumber::operator()()
{

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const size_t totalPoints = featureIds.getNumberOfTuples();

  std::vector<int64> neighbors(totalPoints, -1);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);

  SizeVec3 udims = selectedImageGeom.getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  size_t numFeatures = 0;

  for(size_t i = 0; i < totalPoints; i++)
  {
    const int32 featureName = featureIds[i];
    if(featureName > numFeatures)
    {
      numFeatures = featureName;
    }
  }

  std::array<int64, 6> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  std::array<FaceNeighborType, 6> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  const std::string attrMatName = m_InputValues->FeatureIdsArrayPath.getTargetName();
  const std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

  std::vector<int32> featureCount(numFeatures + 1, 0);
  std::vector<int32> coordinationNumber(totalPoints, 0);
  bool keepGoing = true;
  int32 counter = 1;

  // Z-slice buffering: maintain rolling window of 3 adjacent Z-slices for FeatureIds
  // to avoid random OOC chunk access during neighbor lookups.
  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);

  // Rolling window: slot 0 = z-1, slot 1 = z (current), slot 2 = z+1
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) {
    const usize zOffset = static_cast<usize>(z) * sliceSize;
    for(usize i = 0; i < sliceSize; i++)
    {
      featureIdSlices[slot][i] = featureIds[zOffset + i];
    }
  };

  // Face neighbor ordering: 0=-Z, 1=-Y, 2=-X, 3=+X, 4=+Y, 5=+Z
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  while(counter > 0 && keepGoing)
  {
    counter = 0;
    if(!m_InputValues->Loop)
    {
      keepGoing = false;
    }

    // Initialize rolling window: load z=0 into slot 1, z=1 into slot 2
    readFeatureIdSlice(0, 1);
    if(dims[2] > 1)
    {
      readFeatureIdSlice(1, 2);
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // Advance rolling window for z > 0
      if(zIdx > 0)
      {
        std::swap(featureIdSlices[0], featureIdSlices[1]);
        std::swap(featureIdSlices[1], featureIdSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          readFeatureIdSlice(zIdx + 1, 2);
        }
      }

      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = dims[0] * dims[1] * zIdx + dims[0] * yIdx + xIdx;
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];
          int32 coordination = 0;
          int32 most = 0;

          std::array<bool, 6> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

          const std::array<usize, 6> neighborInSlice = {
              inSlice,                                          // -Z
              static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
              static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
              static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
              static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
              inSlice                                           // +Z
          };

          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
            const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];

            if((featureName > 0 && feature == 0) || (featureName == 0 && feature > 0))
            {
              coordination = coordination + 1;
              featureCount[feature]++;
              const int32 current = featureCount[feature];
              if(current > most)
              {
                most = current;
                neighbors[voxelIndex] = neighborPoint;
              }
            }
          }
          coordinationNumber[voxelIndex] = coordination;
          const int64 neighbor = neighbors[voxelIndex];
          if(coordinationNumber[voxelIndex] >= m_InputValues->CoordinationNumber && coordinationNumber[voxelIndex] > 0)
          {
            // TODO: update to use IDataArray->copyTuple() function
            /******************************************************************
             * If this section is slow it is because we are having to use the
             * ExecuteDataFunction<T>() in order to call "copyTuple()" because
             * "copyTuple()" isn't in the IArray API set. Oh well.
             */
            for(const auto& voxelArray : voxelArrays)
            {
              ExecuteDataFunction(DataArrayCopyTupleFunctor{}, voxelArray->getDataType(), *voxelArray, neighbor, voxelIndex);
            }
            // Update the buffer to reflect the in-place modification
            // Since featureIds is in voxelArrays, the copyTuple above updated
            // the backing store. Re-read the modified voxel into our buffer.
            featureIdSlices[1][inSlice] = featureIds[voxelIndex];
          }

          // Reset featureCount for neighbors
          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];
            if(feature > 0)
            {
              featureCount[feature] = 0;
            }
          }
        }
      }
    }
    for(int64 zIndex = 0; zIndex < dims[2]; zIndex++)
    {
      const auto zStride = static_cast<int64>(dims[0] * dims[1] * zIndex);
      for(int64 yIndex = 0; yIndex < dims[1]; yIndex++)
      {
        const auto yStride = static_cast<int64>(dims[0] * yIndex);
        for(int64 xIndex = 0; xIndex < dims[0]; xIndex++)
        {
          const int64 voxelIndex = zStride + yStride + xIndex;
          if(coordinationNumber[voxelIndex] >= m_InputValues->CoordinationNumber)
          {
            counter++;
          }
        }
      }
    }
  }

  return {};
}
