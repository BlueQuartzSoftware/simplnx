#include "AlignSectionsMutualInformation.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

#include <vector>

using namespace nx::core;

// -----------------------------------------------------------------------------
AlignSectionsMutualInformation::AlignSectionsMutualInformation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                               AlignSectionsMutualInformationInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSectionsMutualInformation::~AlignSectionsMutualInformation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> AlignSectionsMutualInformation::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  return execute(gridGeom.getDimensions(), m_InputValues->ImageGeometryPath);
}

// -----------------------------------------------------------------------------
int32 AlignSectionsMutualInformation::formFeaturesForSlice(const float32* quats, const int32* phases, const uint8* mask, std::vector<int32>& featureIds, int64 dimX, int64 dimY,
                                                           float32 misorientationTolerance, bool useMask, const std::vector<ebsdlib::LaueOps::Pointer>& orientationOps,
                                                           const std::vector<uint32>& crystalStructures)
{
  const int64 sliceVoxels = dimX * dimY;
  usize initialVoxelsListSize = 1000;
  std::vector<int64> voxelList(initialVoxelsListSize, -1);
  int64 neighborPoints[4] = {-dimX, -1, 1, dimX};

  int64 currentStartPoint = 0;
  int32 featureCount = 1;
  bool noSeeds = false;
  while(!noSeeds)
  {
    int64 seed = -1;

    for(int64 point = currentStartPoint; point < sliceVoxels; point++)
    {
      if((!useMask || (mask != nullptr && mask[point] != 0)) && featureIds[point] == 0 && phases[point] > 0)
      {
        seed = point;
        currentStartPoint = point;
      }
      if(seed > -1)
      {
        break;
      }
    }

    if(seed == -1)
    {
      noSeeds = true;
    }
    if(seed >= 0)
    {
      std::vector<int64>::size_type size = 0;
      featureIds[seed] = featureCount;
      voxelList[size] = seed;
      size++;
      for(usize j = 0; j < size; ++j)
      {
        int64 currentpoint = voxelList[j];
        int64 col = currentpoint % dimX;
        int64 row = currentpoint / dimX;

        auto q1Idx = currentpoint * 4;
        ebsdlib::QuatD quat1(quats[q1Idx], quats[q1Idx + 1], quats[q1Idx + 2], quats[q1Idx + 3]);
        uint32 laueClass1 = crystalStructures[phases[currentpoint]];
        for(int32 i = 0; i < 4; i++)
        {
          int64 neighbor = currentpoint + neighborPoints[i];
          if((i == 0) && row == 0)
          {
            continue;
          }
          if((i == 3) && row == (dimY - 1))
          {
            continue;
          }
          if((i == 1) && col == 0)
          {
            continue;
          }
          if((i == 2) && col == (dimX - 1))
          {
            continue;
          }
          if(featureIds[neighbor] <= 0 && phases[neighbor] > 0)
          {
            float32 angle = std::numeric_limits<float32>::max();
            auto q2Idx = neighbor * 4;
            ebsdlib::QuatD quat2(quats[q2Idx], quats[q2Idx + 1], quats[q2Idx + 2], quats[q2Idx + 3]);
            uint32 phase2 = crystalStructures[phases[neighbor]];

            if(laueClass1 == phase2)
            {
              ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
              angle = axisAngle[3];
            }
            if(angle < misorientationTolerance)
            {
              featureIds[neighbor] = featureCount;
              voxelList[size] = neighbor;
              size++;
              if(size >= voxelList.size())
              {
                size = voxelList.size();
                voxelList.resize(size + initialVoxelsListSize);
                for(std::vector<int64>::size_type v = size; v < voxelList.size(); ++v)
                {
                  voxelList[v] = -1;
                }
              }
            }
          }
        }
      }
      voxelList.erase(std::remove(voxelList.begin(), voxelList.end(), -1), voxelList.end());
      featureCount++;
      voxelList.assign(initialVoxelsListSize, -1);
    }
  }
  return featureCount;
}

namespace
{
/**
 * @brief Helper to buffer one slice of mask data into a uint8 vector.
 */
void bufferMaskSlice(const AbstractDataStore<uint8>* maskUInt8StorePtr, const AbstractDataStore<bool>* maskBoolStorePtr, int64 sliceOffset, int64 sliceVoxels, std::vector<uint8>& maskBuf)
{
  if(maskUInt8StorePtr != nullptr)
  {
    maskUInt8StorePtr->copyIntoBuffer(sliceOffset, nonstd::span<uint8>(maskBuf.data(), sliceVoxels));
  }
  else if(maskBoolStorePtr != nullptr)
  {
    auto boolBuf = std::make_unique<bool[]>(sliceVoxels);
    maskBoolStorePtr->copyIntoBuffer(sliceOffset, nonstd::span<bool>(boolBuf.get(), sliceVoxels));
    for(int64 idx = 0; idx < sliceVoxels; idx++)
    {
      maskBuf[idx] = boolBuf[idx] ? 1 : 0;
    }
  }
}

} // namespace

// -----------------------------------------------------------------------------
Result<> AlignSectionsMutualInformation::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  const auto& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath);

  SizeVec3 udims = imageGeom.getDimensions();
  int64 dims[3] = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  const int64 sliceVoxels = dims[0] * dims[1];

  // Set up orientation ops and crystal structures
  auto orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const auto& crystalStructuresStore = crystalStructuresArray.getDataStoreRef();
  std::vector<uint32> crystalStructures(crystalStructuresStore.getSize());
  crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()));

  float32 misorientationTolerance = m_InputValues->MisorientationTolerance * nx::core::Constants::k_PiOver180F;

  // Get store refs for bulk reads (copyIntoBuffer works for both in-core and OOC)
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& quatsStore = quats.getDataStoreRef();
  auto& cellPhasesStore = cellPhases.getDataStoreRef();

  // For bulk mask reads
  const AbstractDataStore<uint8>* maskUInt8StorePtr = nullptr;
  const AbstractDataStore<bool>* maskBoolStorePtr = nullptr;
  if(m_InputValues->UseMask)
  {
    const auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    if(maskArray.getDataType() == DataType::uint8)
    {
      maskUInt8StorePtr = &dynamic_cast<const DataArray<uint8>&>(maskArray).getDataStoreRef();
    }
    else if(maskArray.getDataType() == DataType::boolean)
    {
      maskBoolStorePtr = &dynamic_cast<const DataArray<bool>&>(maskArray).getDataStoreRef();
    }
  }

  // Rolling buffers for 2-slice approach
  std::vector<int32> refFeatureIds(sliceVoxels, 0);
  std::vector<int32> curFeatureIds(sliceVoxels, 0);
  int32 refFeatureCount = 0;
  int32 curFeatureCount = 0;

  // Per-slice buffers for bulk reads
  std::vector<float32> quatsBuf(sliceVoxels * 4);
  std::vector<int32> phasesBuf(sliceVoxels);
  std::vector<uint8> maskBuf;
  if(m_InputValues->UseMask)
  {
    maskBuf.resize(sliceVoxels, 1);
  }

  // Lambda to flood-fill a single slice into a feature ID buffer, returning the feature count
  auto floodFillSlice = [&](int64 sliceIndex, std::vector<int32>& featureIds) -> int32 {
    std::fill(featureIds.begin(), featureIds.end(), 0);

    int64 sliceOffset = sliceIndex * sliceVoxels;

    // Bulk-read this slice's data into local buffers
    cellPhasesStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(phasesBuf.data(), sliceVoxels));
    quatsStore.copyIntoBuffer(sliceOffset * 4, nonstd::span<float32>(quatsBuf.data(), sliceVoxels * 4));

    const uint8* sliceMask = nullptr;
    if(m_InputValues->UseMask)
    {
      bufferMaskSlice(maskUInt8StorePtr, maskBoolStorePtr, sliceOffset, sliceVoxels, maskBuf);
      sliceMask = maskBuf.data();
    }

    return formFeaturesForSlice(quatsBuf.data(), phasesBuf.data(), sliceMask, featureIds, dims[0], dims[1], misorientationTolerance, m_InputValues->UseMask, orientationOps, crystalStructures);
  };

  // Pre-flood-fill the topmost slice (slice = dims[2]-1) into refFeatureIds.
  // The iteration goes from iter=1..dims[2]-1, where slice = (dims[2]-1) - iter.
  // The reference slice is slice+1; for iter=1, that's slice+1 = dims[2]-1.
  int64 topSlice = dims[2] - 1;
  m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Identifying Features: Slice {}/{} complete", topSlice, dims[2]));
  refFeatureCount = floodFillSlice(topSlice, refFeatureIds);

  std::vector<std::vector<float32>> mutualInfo12;
  std::vector<float32> mutualInfo1;
  std::vector<float32> mutualInfo2;

  std::vector<std::vector<float32>> misorientations(dims[0]);
  for(int64 i = 0; i < dims[0]; i++)
  {
    misorientations[i].assign(dims[1], 0.0f);
  }

  if(m_InputValues->StoreAlignmentShifts)
  {
    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      int64 slice = (dims[2] - 1) - iter;

      // Flood-fill the current slice
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Identifying Features: Slice {}/{} complete", slice, dims[2]));
      curFeatureCount = floodFillSlice(slice, curFeatureIds);

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Determining Shifts: Slice {}/{} complete", iter, dims[2]));

      int32 featureCount1 = curFeatureCount;
      int32 featureCount2 = refFeatureCount;
      mutualInfo12 = std::vector<std::vector<float32>>(featureCount1, std::vector<float32>(featureCount2, 0.0f));
      mutualInfo1 = std::vector<float32>(featureCount1, 0.0f);
      mutualInfo2 = std::vector<float32>(featureCount2, 0.0f);

      float32 minDisorientation = std::numeric_limits<float32>::max();
      int64 oldXShift = -1;
      int64 oldYShift = -1;
      int64 newXShift = 0;
      int64 newYShift = 0;
      for(int64 i = 0; i < dims[0]; i++)
      {
        for(int64 j = 0; j < dims[1]; j++)
        {
          misorientations[i][j] = 0.0F;
        }
      }
      while(newXShift != oldXShift || newYShift != oldYShift)
      {
        oldXShift = newXShift;
        oldYShift = newYShift;
        for(int32 j = -3; j < 4; j++)
        {
          for(int32 k = -3; k < 4; k++)
          {
            float32 disorientation = 0.0F;
            float32 count = 0.0F;
            if(misorientations[k + oldXShift + dims[0] / 2][j + oldYShift + dims[1] / 2] == 0 && llabs(k + oldXShift) < (dims[0] / 2) && (j + oldYShift) < (dims[1] / 2))
            {
              for(int64 dim1Index = 0; dim1Index < dims[1]; dim1Index = dim1Index + 4)
              {
                for(int64 dim0Index = 0; dim0Index < dims[0]; dim0Index = dim0Index + 4)
                {
                  if((dim1Index + j + oldYShift) >= 0 && (dim1Index + j + oldYShift) < dims[1] && (dim0Index + k + oldXShift) >= 0 && (dim0Index + k + oldXShift) < dims[0])
                  {
                    int64 refLocalIdx = dim1Index * dims[0] + dim0Index;
                    int64 curLocalIdx = (dim1Index + j + oldYShift) * dims[0] + (dim0Index + k + oldXShift);
                    int32 refGNum = refFeatureIds[refLocalIdx];
                    int32 curGNum = curFeatureIds[curLocalIdx];
                    if(curGNum >= 0 && refGNum >= 0)
                    {
                      mutualInfo12[curGNum][refGNum]++;
                      mutualInfo1[curGNum]++;
                      mutualInfo2[refGNum]++;
                      count++;
                    }
                  }
                  else
                  {
                    mutualInfo12[0][0]++;
                    mutualInfo1[0]++;
                    mutualInfo2[0]++;
                  }
                }
              }
              for(int32 featureCount1Index = 0; featureCount1Index < featureCount1; featureCount1Index++)
              {
                mutualInfo1[featureCount1Index] = mutualInfo1[featureCount1Index] / count;
              }
              for(int32 featureCount2Index = 0; featureCount2Index < featureCount2; featureCount2Index++)
              {
                mutualInfo2[featureCount2Index] = mutualInfo2[featureCount2Index] / static_cast<float32>(count);
              }
              for(int32 featureCount1Index = 0; featureCount1Index < featureCount1; featureCount1Index++)
              {
                for(int32 featureCount2Index = 0; featureCount2Index < featureCount2; featureCount2Index++)
                {
                  mutualInfo12[featureCount1Index][featureCount2Index] = mutualInfo12[featureCount1Index][featureCount2Index] / count;

                  float32 value = 0.0f;
                  if(mutualInfo1[featureCount1Index] > 0 && mutualInfo2[featureCount2Index] > 0)
                  {
                    value = (mutualInfo12[featureCount1Index][featureCount2Index] / (mutualInfo1[featureCount1Index] * mutualInfo2[featureCount2Index]));
                  }
                  if(value != 0)
                  {
                    disorientation = disorientation + (mutualInfo12[featureCount1Index][featureCount2Index] * logf(value));
                  }
                }
              }
              for(int32 featureCount1Index = 0; featureCount1Index < featureCount1; featureCount1Index++)
              {
                for(int32 featureCount2Index = 0; featureCount2Index < featureCount2; featureCount2Index++)
                {
                  mutualInfo12[featureCount1Index][featureCount2Index] = 0.0f;
                  mutualInfo1[featureCount1Index] = 0.0f;
                  mutualInfo2[featureCount2Index] = 0.0f;
                }
              }
              disorientation = 1.0f / disorientation;
              misorientations[k + oldXShift + dims[0] / 2][j + oldYShift + dims[1] / 2] = disorientation;
              if(disorientation < minDisorientation)
              {
                newXShift = k + oldXShift;
                newYShift = j + oldYShift;
                minDisorientation = disorientation;
              }
            }
          }
        }
      }
      xShifts[iter] = xShifts[iter - 1] + newXShift;
      yShifts[iter] = yShifts[iter - 1] + newYShift;

      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = slice;
      slicesStore[yIndex] = slice + 1;
      relativeShiftsStore[xIndex] = newXShift;
      relativeShiftsStore[yIndex] = newYShift;
      cumulativeShiftsStore[xIndex] = xShifts[iter];
      cumulativeShiftsStore[yIndex] = yShifts[iter];

      // Roll: current slice becomes reference for the next iteration
      std::swap(refFeatureIds, curFeatureIds);
      refFeatureCount = curFeatureCount;
    }
  }
  else
  {
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      int64 slice = (dims[2] - 1) - iter;

      // Flood-fill the current slice
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Identifying Features: Slice {}/{} complete", slice, dims[2]));
      curFeatureCount = floodFillSlice(slice, curFeatureIds);

      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Determining Shifts: Slice {}/{} complete", iter, dims[2]));

      int32 featureCount1 = curFeatureCount;
      int32 featureCount2 = refFeatureCount;
      mutualInfo12 = std::vector<std::vector<float32>>(featureCount1, std::vector<float32>(featureCount2, 0.0f));
      mutualInfo1 = std::vector<float32>(featureCount1, 0.0f);
      mutualInfo2 = std::vector<float32>(featureCount2, 0.0f);

      float32 minDisorientation = std::numeric_limits<float32>::max();
      int64 oldXShift = -1;
      int64 oldYShift = -1;
      int64 newXShift = 0;
      int64 newYShift = 0;
      for(int64 i = 0; i < dims[0]; i++)
      {
        for(int64 j = 0; j < dims[1]; j++)
        {
          misorientations[i][j] = 0.0F;
        }
      }
      while(newXShift != oldXShift || newYShift != oldYShift)
      {
        oldXShift = newXShift;
        oldYShift = newYShift;
        for(int32 j = -3; j < 4; j++)
        {
          for(int32 k = -3; k < 4; k++)
          {
            float32 disorientation = 0.0F;
            float32 count = 0.0F;
            if(misorientations[k + oldXShift + dims[0] / 2][j + oldYShift + dims[1] / 2] == 0 && llabs(k + oldXShift) < (dims[0] / 2) && (j + oldYShift) < (dims[1] / 2))
            {
              for(int64 dim1Index = 0; dim1Index < dims[1]; dim1Index = dim1Index + 4)
              {
                for(int64 dim0Index = 0; dim0Index < dims[0]; dim0Index = dim0Index + 4)
                {
                  if((dim1Index + j + oldYShift) >= 0 && (dim1Index + j + oldYShift) < dims[1] && (dim0Index + k + oldXShift) >= 0 && (dim0Index + k + oldXShift) < dims[0])
                  {
                    int64 refLocalIdx = dim1Index * dims[0] + dim0Index;
                    int64 curLocalIdx = (dim1Index + j + oldYShift) * dims[0] + (dim0Index + k + oldXShift);
                    int32 refGNum = refFeatureIds[refLocalIdx];
                    int32 curGNum = curFeatureIds[curLocalIdx];
                    if(curGNum >= 0 && refGNum >= 0)
                    {
                      mutualInfo12[curGNum][refGNum]++;
                      mutualInfo1[curGNum]++;
                      mutualInfo2[refGNum]++;
                      count++;
                    }
                  }
                  else
                  {
                    mutualInfo12[0][0]++;
                    mutualInfo1[0]++;
                    mutualInfo2[0]++;
                  }
                }
              }
              for(int32 featureCount1Index = 0; featureCount1Index < featureCount1; featureCount1Index++)
              {
                mutualInfo1[featureCount1Index] = mutualInfo1[featureCount1Index] / count;
              }
              for(int32 featureCount2Index = 0; featureCount2Index < featureCount2; featureCount2Index++)
              {
                mutualInfo2[featureCount2Index] = mutualInfo2[featureCount2Index] / static_cast<float32>(count);
              }
              for(int32 featureCount1Index = 0; featureCount1Index < featureCount1; featureCount1Index++)
              {
                for(int32 featureCount2Index = 0; featureCount2Index < featureCount2; featureCount2Index++)
                {
                  mutualInfo12[featureCount1Index][featureCount2Index] = mutualInfo12[featureCount1Index][featureCount2Index] / count;

                  float32 value = 0.0f;
                  if(mutualInfo1[featureCount1Index] > 0 && mutualInfo2[featureCount2Index] > 0)
                  {
                    value = (mutualInfo12[featureCount1Index][featureCount2Index] / (mutualInfo1[featureCount1Index] * mutualInfo2[featureCount2Index]));
                  }
                  if(value != 0)
                  {
                    disorientation = disorientation + (mutualInfo12[featureCount1Index][featureCount2Index] * logf(value));
                  }
                }
              }
              for(int32 featureCount1Index = 0; featureCount1Index < featureCount1; featureCount1Index++)
              {
                for(int32 featureCount2Index = 0; featureCount2Index < featureCount2; featureCount2Index++)
                {
                  mutualInfo12[featureCount1Index][featureCount2Index] = 0.0f;
                  mutualInfo1[featureCount1Index] = 0.0f;
                  mutualInfo2[featureCount2Index] = 0.0f;
                }
              }
              disorientation = 1.0f / disorientation;
              misorientations[k + oldXShift + dims[0] / 2][j + oldYShift + dims[1] / 2] = disorientation;
              if(disorientation < minDisorientation)
              {
                newXShift = k + oldXShift;
                newYShift = j + oldYShift;
                minDisorientation = disorientation;
              }
            }
          }
        }
      }
      xShifts[iter] = xShifts[iter - 1] + newXShift;
      yShifts[iter] = yShifts[iter - 1] + newYShift;

      // Roll: current slice becomes reference for the next iteration
      std::swap(refFeatureIds, curFeatureIds);
      refFeatureCount = curFeatureCount;
    }
  }

  return {};
}
