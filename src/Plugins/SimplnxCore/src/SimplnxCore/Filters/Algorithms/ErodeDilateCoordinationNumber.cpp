/**
 * @file ErodeDilateCoordinationNumber.cpp
 * @brief Smooths good/bad boundaries with slice-bounded coordination state.
 */

#include "ErodeDilateCoordinationNumber.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataGroupUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/NeighborUtilities.hpp"
#include "simplnx/Utilities/SliceBufferedTransfer.hpp"

#include <algorithm>
#include <array>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @struct OrderedCoordinationSliceTransfer
 * @brief Replays one slice's tuple replacements in voxel order.
 *
 * Same-slice copies read the mutable destination buffer so later copies see earlier replacements.
 * Adjacent source slices load only when a mark needs them.
 */
struct OrderedCoordinationSliceTransfer
{
  /**
   * @brief Transfers one selected store through bounded, contiguous buffers.
   * @tparam T Store element type, including bool.
   * @param dataArray Represents the selected store.
   * @param marks Maps current-slice destinations to global source tuples, or -1.
   * @param sliceSize Number of tuples per XY slice.
   * @param destZ Current destination slice.
   * @param dimZ Number of image slices.
   * @pre Each marked source is a valid face neighbor of its destination.
   * @pre Previous slices are committed, and future slices are unchanged in this pass.
   * @pre Tuple/component products fit usize and agree with the store shape.
   * @return First invalid bulk Result, or success after the selected slice write.
   */
  template <typename T>
  Result<> operator()(IDataArray& dataArray, const std::vector<int64>& marks, usize sliceSize, usize destZ, usize dimZ) const
  {
    using BufferType = SliceBufferedTransferFunctor::BufferType<T>;
    auto& storeRef = dynamic_cast<DataArray<T>&>(dataArray).getDataStoreRef();
    const usize componentCount = storeRef.getNumberOfComponents();
    const usize sliceValueCount = sliceSize * componentCount;
    auto destinationBuffer = SliceBufferedTransferFunctor::makeBuf<T>(sliceValueCount);
    auto* destinationPtr = SliceBufferedTransferFunctor::bufPtr(destinationBuffer);
    auto readResult = storeRef.copyIntoBuffer(destZ * sliceValueCount, nonstd::span<T>(destinationPtr, sliceValueCount));
    if(readResult.invalid())
    {
      return readResult;
    }

    std::array<BufferType, 2> adjacentBuffers;
    std::array<bool, 2> isAdjacentLoaded = {false, false};
    bool isModified = false;
    for(usize inSlice = 0; inSlice < sliceSize; inSlice++)
    {
      const int64 sourceIndex = marks[inSlice];
      if(sourceIndex < 0)
      {
        continue;
      }

      const usize sourceZ = static_cast<usize>(sourceIndex) / sliceSize;
      const usize sourceInSlice = static_cast<usize>(sourceIndex) % sliceSize;
      const T* sourcePtr = destinationPtr;
      if(sourceZ != destZ)
      {
        const usize sourceSlot = sourceZ < destZ ? 0 : 1;
        if(!isAdjacentLoaded[sourceSlot] && sourceZ < dimZ)
        {
          adjacentBuffers[sourceSlot] = SliceBufferedTransferFunctor::makeBuf<T>(sliceValueCount);
          readResult = storeRef.copyIntoBuffer(sourceZ * sliceValueCount, nonstd::span<T>(SliceBufferedTransferFunctor::bufPtr(adjacentBuffers[sourceSlot]), sliceValueCount));
          if(readResult.invalid())
          {
            return readResult;
          }
          isAdjacentLoaded[sourceSlot] = true;
        }
        sourcePtr = SliceBufferedTransferFunctor::bufPtr(adjacentBuffers[sourceSlot]);
      }

      for(usize componentIdx = 0; componentIdx < componentCount; componentIdx++)
      {
        destinationPtr[inSlice * componentCount + componentIdx] = sourcePtr[sourceInSlice * componentCount + componentIdx];
      }
      isModified = true;
    }

    if(isModified)
    {
      return storeRef.copyFromBuffer(destZ * sliceValueCount, nonstd::span<const T>(destinationPtr, sliceValueCount));
    }
    return {};
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
const std::atomic_bool& ErodeDilateCoordinationNumber::getCancel() const
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ErodeDilateCoordinationNumber::operator()()
{
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  const auto& selectedImageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->InputImageGeometry);
  SizeVec3 udims = selectedImageGeom.getDimensions();
  std::array<int64, 3> dims = {static_cast<int64>(udims[0]), static_cast<int64>(udims[1]), static_cast<int64>(udims[2])};

  // Face order determines which source wins when neighbor tallies compete.
  constexpr FaceNeighborType k_NumFaceNeighbors = VoxelNeighbors<Image3D>::k_FaceNeighborCount;
  const std::array<int64, k_NumFaceNeighbors> neighborVoxelIndexOffsets = initializeFaceNeighborOffsets(dims);
  constexpr std::array<FaceNeighborType, k_NumFaceNeighbors> faceNeighborInternalIdx = initializeFaceNeighborInternalIdx();

  std::vector<std::shared_ptr<IDataArray>> voxelArrays = nx::core::GenerateDataArrayList(m_DataStructure, m_InputValues->FeatureIdsArrayPath, m_InputValues->IgnoredDataArrayPaths);

  // Aliases repeat an idempotent tuple copy within one voxel, but must not replay a complete slice twice.
  usize uniqueStoreCount = 0;
  for(usize arrayIdx = 0; arrayIdx < voxelArrays.size(); arrayIdx++)
  {
    const IDataStore* storePtr = voxelArrays[arrayIdx]->getIDataStore();
    bool isAlreadySelected = false;
    for(usize selectedIdx = 0; selectedIdx < uniqueStoreCount; selectedIdx++)
    {
      if(voxelArrays[selectedIdx]->getIDataStore() == storePtr)
      {
        isAlreadySelected = true;
        break;
      }
    }
    if(!isAlreadySelected)
    {
      voxelArrays[uniqueStoreCount++] = voxelArrays[arrayIdx];
    }
  }
  voxelArrays.resize(uniqueStoreCount);

  // An ignored FeatureIds path can still change through a selected store alias.
  const bool transfersFeatureIds = std::any_of(voxelArrays.cbegin(), voxelArrays.cend(), [&featureIds](const auto& array) { return array->getIDataStore() == featureIds.getIDataStore(); });

  const usize sliceSize = static_cast<usize>(dims[0]) * static_cast<usize>(dims[1]);
  const usize dimZ = static_cast<usize>(dims[2]);

  // Sequential bulk reads avoid OOC chunk thrashing.
  const auto& featureIdsStore = featureIds.getDataStoreRef();
  usize numFeatures = 0;
  {
    std::vector<int32> sliceBuf(sliceSize);
    for(int64 z = 0; z < dims[2]; z++)
    {
      Result<> readResult = featureIdsStore.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(sliceBuf.data(), sliceSize));
      if(readResult.invalid())
      {
        return readResult;
      }
      for(usize i = 0; i < sliceSize; i++)
      {
        if(sliceBuf[i] > static_cast<int32>(numFeatures))
        {
          numFeatures = sliceBuf[i];
        }
      }
    }
  }

  // Positive neighbor tallies reset per voxel. The zero tally accumulates across voxels and affects source selection.
  std::vector<int32> featureCount(numFeatures + 1, 0);
  bool keepGoing = true;
  int32 counter = 1;

  // Slots hold the completed previous slice, the mutable current slice, and the untouched next slice.
  std::array<std::vector<int32>, 3> featureIdSlices;
  for(auto& fis : featureIdSlices)
  {
    fis.resize(sliceSize);
  }

  auto readFeatureIdSlice = [&](int64 z, usize slot) { return featureIdsStore.copyIntoBuffer(static_cast<usize>(z) * sliceSize, nonstd::span<int32>(featureIdSlices[slot].data(), sliceSize)); };

  // The -Z and +Z neighbors use adjacent slots; the four XY neighbors use the current slot.
  constexpr std::array<usize, 6> k_NeighborSlot = {0, 1, 1, 1, 1, 2};

  // One slice of ordered source marks lets each sibling replay the same immediate tuple copies.
  std::vector<int64> sliceNeighbors(sliceSize, -1);

  // A qualifying voxel contributes to the counter even when its Feature ID store is ignored.
  while(counter > 0 && keepGoing)
  {
    counter = 0;
    if(!m_InputValues->Loop)
    {
      keepGoing = false;
    }

    // Each new pass starts from the previous pass's committed stores.
    Result<> ioResult = readFeatureIdSlice(0, 1);
    if(ioResult.invalid())
    {
      return ioResult;
    }
    if(dims[2] > 1)
    {
      ioResult = readFeatureIdSlice(1, 2);
      if(ioResult.invalid())
      {
        return ioResult;
      }
    }

    for(int64 zIdx = 0; zIdx < dims[2]; zIdx++)
    {
      // The completed current slice becomes the previous slice after all sibling stores commit.
      if(zIdx > 0)
      {
        std::swap(featureIdSlices[0], featureIdSlices[1]);
        std::swap(featureIdSlices[1], featureIdSlices[2]);
        if(zIdx + 1 < dims[2])
        {
          ioResult = readFeatureIdSlice(zIdx + 1, 2);
          if(ioResult.invalid())
          {
            return ioResult;
          }
        }
      }

      std::fill(sliceNeighbors.begin(), sliceNeighbors.end(), -1);
      for(int64 yIdx = 0; yIdx < dims[1]; yIdx++)
      {
        for(int64 xIdx = 0; xIdx < dims[0]; xIdx++)
        {
          const int64 voxelIndex = dims[0] * dims[1] * zIdx + dims[0] * yIdx + xIdx;
          const usize inSlice = static_cast<usize>(yIdx * dims[0] + xIdx);
          const int32 featureName = featureIdSlices[1][inSlice];
          int32 coordination = 0;
          int32 most = 0;
          int64 selectedNeighbor = -1;
          int32 selectedFeature = featureName;

          const std::array<bool, k_NumFaceNeighbors> isValidFaceNeighbor = computeValidFaceNeighbors(xIdx, yIdx, zIdx, dims);

          // The validity mask prevents access to out-of-image neighbor positions.
          const std::array<usize, 6> neighborInSlice = {
              inSlice,                                         // -Z
              static_cast<usize>((yIdx - 1) * dims[0] + xIdx), // -Y
              static_cast<usize>(yIdx * dims[0] + (xIdx - 1)), // -X
              static_cast<usize>(yIdx * dims[0] + (xIdx + 1)), // +X
              static_cast<usize>((yIdx + 1) * dims[0] + xIdx), // +Y
              inSlice                                          // +Z
          };

          for(const auto& faceIndex : faceNeighborInternalIdx)
          {
            if(!isValidFaceNeighbor[faceIndex])
            {
              continue;
            }

            const int64 neighborPoint = voxelIndex + neighborVoxelIndexOffsets[faceIndex];
            const int32 feature = featureIdSlices[k_NeighborSlot[faceIndex]][neighborInSlice[faceIndex]];

            // A voxel is on the boundary if it and its neighbor have opposite
            // good/bad status (one is 0, the other is > 0).
            if((featureName > 0 && feature == 0) || (featureName == 0 && feature > 0))
            {
              coordination = coordination + 1;
              featureCount[feature]++;
              const int32 current = featureCount[feature];
              if(current > most)
              {
                most = current;
                selectedNeighbor = neighborPoint;
                selectedFeature = feature;
              }
            }
          }
          if(coordination >= m_InputValues->CoordinationNumber && coordination > 0)
          {
            sliceNeighbors[inSlice] = selectedNeighbor;
            counter++;
            if(transfersFeatureIds)
            {
              // Later decisions see this replacement before sibling stores replay the completed slice.
              featureIdSlices[1][inSlice] = selectedFeature;
            }
          }

          // The center update does not change any face-neighbor value used by this reset.
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

      // Replay from the uncommitted store slice. Prewriting resident Feature IDs would apply the marks twice.
      for(const auto& voxelArray : voxelArrays)
      {
        ioResult = ExecuteDataFunction(OrderedCoordinationSliceTransfer{}, voxelArray->getDataType(), *voxelArray, sliceNeighbors, sliceSize, static_cast<usize>(zIdx), dimZ);
        if(ioResult.invalid())
        {
          return ioResult;
        }
      }
    }
  }

  return {};
}
