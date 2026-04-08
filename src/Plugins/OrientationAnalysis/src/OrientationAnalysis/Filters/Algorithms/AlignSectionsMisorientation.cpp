#include "AlignSectionsMisorientation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
AlignSectionsMisorientation::AlignSectionsMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         AlignSectionsMisorientationInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AlignSectionsMisorientation::~AlignSectionsMisorientation() noexcept = default;

// -----------------------------------------------------------------------------
Result<> AlignSectionsMisorientation::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  return execute(gridGeom.getDimensions(), m_InputValues->ImageGeometryPath);
}

// -----------------------------------------------------------------------------
Result<> AlignSectionsMisorientation::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  {
    const auto& quatsCheck = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->quatsArrayPath);
    const auto& cellPhasesCheck = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
    if(ForceOocAlgorithm() || IsOutOfCore(quatsCheck) || IsOutOfCore(cellPhasesCheck))
    {
      return findShiftsOoc(xShifts, yShifts);
    }
  }

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-53900, message);
    }
  }

  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->quatsArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);

  SizeVec3 udims = gridGeom->getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  // Allocate a 2D Array which will be reused from slice to slice
  std::vector<bool> misorients(dims[0] * dims[1], false);

  const auto halfDim0 = static_cast<int64>(dims[0] * 0.5f);
  const auto halfDim1 = static_cast<int64>(dims[1] * 0.5f);

  float64 deg2Rad = (nx::core::numbers::pi / 180.0);
  ThrottledMessenger throttledMessenger = getMessageHelper().createThrottledMessenger();
  if(m_InputValues->StoreAlignmentShifts)
  {
    auto& slicesStore = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    auto& relativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    auto& cumulativeShiftsStore = m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
    // Loop over the Z Direction
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Shifts || {:.2f}% Complete", CalculatePercentComplete(iter, dims[2])); });
      if(getCancel())
      {
        return {};
      }
      float32 minDisorientation = std::numeric_limits<float32>::max();
      // Work from the largest Slice Value to the lowest Slice Value.
      int64 slice = (dims[2] - 1) - iter;
      int64 oldxshift = -1;
      int64 oldyshift = -1;
      int64 newxshift = 0;
      int64 newyshift = 0;

      // Initialize everything to false
      std::fill(misorients.begin(), misorients.end(), false);

      float32 misorientationTolerance = static_cast<float32>(m_InputValues->misorientationTolerance * deg2Rad);

      while(newxshift != oldxshift || newyshift != oldyshift)
      {
        oldxshift = newxshift;
        oldyshift = newyshift;
        for(int32 j = -3; j < 4; j++)
        {
          for(int32 k = -3; k < 4; k++)
          {
            float32 disorientation = 0.0f;
            float32 count = 0.0f;
            int64 xIdx = k + oldxshift + halfDim0;
            int64 yIdx = j + oldyshift + halfDim1;
            int64 idx = (dims[0] * yIdx) + xIdx;
            if(!misorients[idx] && llabs(k + oldxshift) < halfDim0 && llabs(j + oldyshift) < halfDim1)
            {
              for(int64 l = 0; l < dims[1]; l = l + 4)
              {
                for(int64 n = 0; n < dims[0]; n = n + 4)
                {
                  if((l + j + oldyshift) >= 0 && (l + j + oldyshift) < dims[1] && (n + k + oldxshift) >= 0 && (n + k + oldxshift) < dims[0])
                  {
                    count++;
                    int64 refposition = ((slice + 1) * dims[0] * dims[1]) + (l * dims[0]) + n;
                    int64 curposition = (slice * dims[0] * dims[1]) + ((l + j + oldyshift) * dims[0]) + (n + k + oldxshift);
                    if(!m_InputValues->UseMask || maskCompare->bothTrue(refposition, curposition))
                    {
                      float32 angle = std::numeric_limits<float32>::max();
                      if(cellPhases[refposition] > 0 && cellPhases[curposition] > 0)
                      {
                        ebsdlib::QuatD quat1(quats[refposition * 4], quats[refposition * 4 + 1], quats[refposition * 4 + 2], quats[refposition * 4 + 3]); // Makes a copy into voxQuat!!!!
                        auto laueClass1 = static_cast<int32>(crystalStructures[cellPhases[refposition]]);
                        ebsdlib::QuatD quat2(quats[curposition * 4], quats[curposition * 4 + 1], quats[curposition * 4 + 2], quats[curposition * 4 + 3]); // Makes a copy into voxQuat!!!!
                        auto laueClass2 = static_cast<int32>(crystalStructures[cellPhases[curposition]]);
                        if(laueClass1 == laueClass2 && laueClass1 < static_cast<uint32>(orientationOps.size()))
                        {
                          ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                          angle = axisAngle[3];
                        }
                      }
                      if(angle > misorientationTolerance)
                      {
                        disorientation++;
                      }
                    }
                    if(m_InputValues->UseMask)
                    {
                      if(maskCompare->isTrue(refposition) && !maskCompare->isTrue(curposition))
                      {
                        disorientation++;
                      }
                      if(!maskCompare->isTrue(refposition) && maskCompare->isTrue(curposition))
                      {
                        disorientation++;
                      }
                    }
                  }
                }
              }
              disorientation = disorientation / count;
              xIdx = k + oldxshift + halfDim0;
              yIdx = j + oldyshift + halfDim1;
              idx = (dims[0] * yIdx) + xIdx;
              misorients[idx] = true;
              if(disorientation < minDisorientation || (disorientation == minDisorientation && ((llabs(k + oldxshift) < llabs(newxshift)) || (llabs(j + oldyshift) < llabs(newyshift)))))
              {
                newxshift = k + oldxshift;
                newyshift = j + oldyshift;
                minDisorientation = disorientation;
              }
            }
          }
        }
      }
      xShifts[iter] = xShifts[iter - 1] + newxshift;
      yShifts[iter] = yShifts[iter - 1] + newyshift;
      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      slicesStore[xIndex] = slice;
      slicesStore[yIndex] = slice + 1;
      relativeShiftsStore[xIndex] = newxshift;
      relativeShiftsStore[yIndex] = newyshift;
      cumulativeShiftsStore[xIndex] = xShifts[iter];
      cumulativeShiftsStore[yIndex] = yShifts[iter];
    }
  }
  else
  {
    // Loop over the Z Direction
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Shifts || {:.2f}% Complete", CalculatePercentComplete(iter, dims[2])); });
      if(getCancel())
      {
        return {};
      }
      float32 minDisorientation = std::numeric_limits<float32>::max();
      // Work from the largest Slice Value to the lowest Slice Value.
      int64 slice = (dims[2] - 1) - iter;
      int64 oldxshift = -1;
      int64 oldyshift = -1;
      int64 newxshift = 0;
      int64 newyshift = 0;

      // Initialize everything to false
      std::fill(misorients.begin(), misorients.end(), false);

      float32 misorientationTolerance = static_cast<float32>(m_InputValues->misorientationTolerance * deg2Rad);

      while(newxshift != oldxshift || newyshift != oldyshift)
      {
        oldxshift = newxshift;
        oldyshift = newyshift;
        for(int32 j = -3; j < 4; j++)
        {
          for(int32 k = -3; k < 4; k++)
          {
            float32 disorientation = 0.0f;
            float32 count = 0.0f;
            int64 xIdx = k + oldxshift + halfDim0;
            int64 yIdx = j + oldyshift + halfDim1;
            int64 idx = (dims[0] * yIdx) + xIdx;
            if(!misorients[idx] && llabs(k + oldxshift) < halfDim0 && llabs(j + oldyshift) < halfDim1)
            {
              for(int64 l = 0; l < dims[1]; l = l + 4)
              {
                for(int64 n = 0; n < dims[0]; n = n + 4)
                {
                  if((l + j + oldyshift) >= 0 && (l + j + oldyshift) < dims[1] && (n + k + oldxshift) >= 0 && (n + k + oldxshift) < dims[0])
                  {
                    count++;
                    int64 refposition = ((slice + 1) * dims[0] * dims[1]) + (l * dims[0]) + n;
                    int64 curposition = (slice * dims[0] * dims[1]) + ((l + j + oldyshift) * dims[0]) + (n + k + oldxshift);
                    if(!m_InputValues->UseMask || maskCompare->bothTrue(refposition, curposition))
                    {
                      float32 angle = std::numeric_limits<float32>::max();
                      if(cellPhases[refposition] > 0 && cellPhases[curposition] > 0)
                      {
                        ebsdlib::QuatD quat1(quats[refposition * 4], quats[refposition * 4 + 1], quats[refposition * 4 + 2], quats[refposition * 4 + 3]); // Makes a copy into voxQuat!!!!
                        auto laueClass1 = static_cast<int32>(crystalStructures[cellPhases[refposition]]);
                        ebsdlib::QuatD quat2(quats[curposition * 4], quats[curposition * 4 + 1], quats[curposition * 4 + 2], quats[curposition * 4 + 3]); // Makes a copy into voxQuat!!!!
                        auto laueClass2 = static_cast<int32>(crystalStructures[cellPhases[curposition]]);
                        if(laueClass1 == laueClass2 && laueClass1 < static_cast<uint32>(orientationOps.size()))
                        {
                          ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                          angle = axisAngle[3];
                        }
                      }
                      if(angle > misorientationTolerance)
                      {
                        disorientation++;
                      }
                    }
                    if(m_InputValues->UseMask)
                    {
                      if(maskCompare->isTrue(refposition) && !maskCompare->isTrue(curposition))
                      {
                        disorientation++;
                      }
                      if(!maskCompare->isTrue(refposition) && maskCompare->isTrue(curposition))
                      {
                        disorientation++;
                      }
                    }
                  }
                }
              }
              disorientation = disorientation / count;
              xIdx = k + oldxshift + halfDim0;
              yIdx = j + oldyshift + halfDim1;
              idx = (dims[0] * yIdx) + xIdx;
              misorients[idx] = true;
              if(disorientation < minDisorientation || (disorientation == minDisorientation && ((llabs(k + oldxshift) < llabs(newxshift)) || (llabs(j + oldyshift) < llabs(newyshift)))))
              {
                newxshift = k + oldxshift;
                newyshift = j + oldyshift;
                minDisorientation = disorientation;
              }
            }
          }
        }
      }
      xShifts[iter] = xShifts[iter - 1] + newxshift;
      yShifts[iter] = yShifts[iter - 1] + newyshift;
    }
  }

  return {};
}

// -----------------------------------------------------------------------------
// OOC-optimized findShifts: buffers 2 adjacent Z-slices of quats, cellPhases,
// and mask into local vectors before the convergence loop, eliminating random
// chunk-based DataStore access.
// -----------------------------------------------------------------------------
Result<> AlignSectionsMisorientation::findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  // For OOC mask buffering, get the raw mask store for bulk reads instead of per-element isTrue()
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
    else
    {
      return MakeErrorResult(-53900, fmt::format("Mask Array is not Bool or UInt8: {}", m_InputValues->MaskArrayPath.toString()));
    }
  }

  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->quatsArrayPath);
  const auto& crystalStructuresArray = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
  auto& cellPhasesStore = cellPhases.getDataStoreRef();
  auto& quatsStore = quats.getDataStoreRef();

  // Cache ensemble-level array locally to avoid per-element virtual dispatch in hot loop
  const auto& crystalStructuresStore = crystalStructuresArray.getDataStoreRef();
  std::vector<uint32> crystalStructures(crystalStructuresStore.getSize());
  crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()));

  SizeVec3 udims = gridGeom->getDimensions();

  std::array<int64, 3> dims = {
      static_cast<int64>(udims[0]),
      static_cast<int64>(udims[1]),
      static_cast<int64>(udims[2]),
  };

  std::vector<ebsdlib::LaueOps::Pointer> orientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  std::vector<bool> misorients(dims[0] * dims[1], false);

  const auto halfDim0 = static_cast<int64>(dims[0] * 0.5f);
  const auto halfDim1 = static_cast<int64>(dims[1] * 0.5f);

  float64 deg2Rad = (nx::core::numbers::pi / 180.0);

  const int64 sliceVoxels = dims[0] * dims[1];

  // Buffers for 2 Z-slices: reference (slice+1) and current (slice)
  std::vector<float32> refQuatsBuf(sliceVoxels * 4);
  std::vector<float32> curQuatsBuf(sliceVoxels * 4);
  std::vector<int32> refPhasesBuf(sliceVoxels);
  std::vector<int32> curPhasesBuf(sliceVoxels);
  std::vector<uint8> refMaskBuf;
  std::vector<uint8> curMaskBuf;
  if(m_InputValues->UseMask)
  {
    refMaskBuf.resize(sliceVoxels, 1);
    curMaskBuf.resize(sliceVoxels, 1);
  }

  // Optional output stores
  AbstractDataStore<uint32>* slicesStorePtr = nullptr;
  AbstractDataStore<int64>* relativeShiftsStorePtr = nullptr;
  AbstractDataStore<int64>* cumulativeShiftsStorePtr = nullptr;
  if(m_InputValues->StoreAlignmentShifts)
  {
    slicesStorePtr = &m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    relativeShiftsStorePtr = &m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    cumulativeShiftsStorePtr = &m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
  }

  // Pre-load the first reference slice (the top-most Z-slice) via bulk read
  {
    int64 firstRefOffset = (dims[2] - 1) * sliceVoxels;
    cellPhasesStore.copyIntoBuffer(firstRefOffset, nonstd::span<int32>(refPhasesBuf.data(), sliceVoxels));
    quatsStore.copyIntoBuffer(firstRefOffset * 4, nonstd::span<float32>(refQuatsBuf.data(), sliceVoxels * 4));
    if(m_InputValues->UseMask)
    {
      if(maskUInt8StorePtr != nullptr)
      {
        maskUInt8StorePtr->copyIntoBuffer(firstRefOffset, nonstd::span<uint8>(refMaskBuf.data(), sliceVoxels));
      }
      else if(maskBoolStorePtr != nullptr)
      {
        auto boolBuf = std::make_unique<bool[]>(sliceVoxels);
        maskBoolStorePtr->copyIntoBuffer(firstRefOffset, nonstd::span<bool>(boolBuf.get(), sliceVoxels));
        for(int64 idx = 0; idx < sliceVoxels; idx++)
        {
          refMaskBuf[idx] = boolBuf[idx] ? 1 : 0;
        }
      }
    }
  }

  for(int64 iter = 1; iter < dims[2]; iter++)
  {
    if(getCancel())
    {
      return {};
    }

    int64 slice = (dims[2] - 1) - iter;

    // Bulk-read current slice (reference available from pre-load or previous iteration swap)
    int64 curOffset = slice * sliceVoxels;
    cellPhasesStore.copyIntoBuffer(curOffset, nonstd::span<int32>(curPhasesBuf.data(), sliceVoxels));
    quatsStore.copyIntoBuffer(curOffset * 4, nonstd::span<float32>(curQuatsBuf.data(), sliceVoxels * 4));
    if(m_InputValues->UseMask)
    {
      if(maskUInt8StorePtr != nullptr)
      {
        maskUInt8StorePtr->copyIntoBuffer(curOffset, nonstd::span<uint8>(curMaskBuf.data(), sliceVoxels));
      }
      else if(maskBoolStorePtr != nullptr)
      {
        auto boolBuf = std::make_unique<bool[]>(sliceVoxels);
        maskBoolStorePtr->copyIntoBuffer(curOffset, nonstd::span<bool>(boolBuf.get(), sliceVoxels));
        for(int64 idx = 0; idx < sliceVoxels; idx++)
        {
          curMaskBuf[idx] = boolBuf[idx] ? 1 : 0;
        }
      }
    }

    float32 minDisorientation = std::numeric_limits<float32>::max();
    int64 oldxshift = -1;
    int64 oldyshift = -1;
    int64 newxshift = 0;
    int64 newyshift = 0;

    std::fill(misorients.begin(), misorients.end(), false);

    float32 misorientationTolerance = static_cast<float32>(m_InputValues->misorientationTolerance * deg2Rad);

    while(newxshift != oldxshift || newyshift != oldyshift)
    {
      oldxshift = newxshift;
      oldyshift = newyshift;
      for(int32 j = -3; j < 4; j++)
      {
        for(int32 k = -3; k < 4; k++)
        {
          float32 disorientation = 0.0f;
          float32 count = 0.0f;
          int64 xIdx = k + oldxshift + halfDim0;
          int64 yIdx = j + oldyshift + halfDim1;
          int64 idx = (dims[0] * yIdx) + xIdx;
          if(!misorients[idx] && llabs(k + oldxshift) < halfDim0 && llabs(j + oldyshift) < halfDim1)
          {
            for(int64 l = 0; l < dims[1]; l = l + 4)
            {
              for(int64 n = 0; n < dims[0]; n = n + 4)
              {
                if((l + j + oldyshift) >= 0 && (l + j + oldyshift) < dims[1] && (n + k + oldxshift) >= 0 && (n + k + oldxshift) < dims[0])
                {
                  count++;
                  // Local buffer indices (within-slice)
                  int64 refLocalIdx = l * dims[0] + n;
                  int64 curLocalIdx = (l + j + oldyshift) * dims[0] + (n + k + oldxshift);

                  bool maskOk = !m_InputValues->UseMask || (refMaskBuf[refLocalIdx] != 0 && curMaskBuf[curLocalIdx] != 0);
                  if(maskOk)
                  {
                    float32 angle = std::numeric_limits<float32>::max();
                    if(refPhasesBuf[refLocalIdx] > 0 && curPhasesBuf[curLocalIdx] > 0)
                    {
                      ebsdlib::QuatD quat1(refQuatsBuf[refLocalIdx * 4], refQuatsBuf[refLocalIdx * 4 + 1], refQuatsBuf[refLocalIdx * 4 + 2], refQuatsBuf[refLocalIdx * 4 + 3]);
                      auto laueClass1 = static_cast<int32>(crystalStructures[refPhasesBuf[refLocalIdx]]);
                      ebsdlib::QuatD quat2(curQuatsBuf[curLocalIdx * 4], curQuatsBuf[curLocalIdx * 4 + 1], curQuatsBuf[curLocalIdx * 4 + 2], curQuatsBuf[curLocalIdx * 4 + 3]);
                      auto laueClass2 = static_cast<int32>(crystalStructures[curPhasesBuf[curLocalIdx]]);
                      if(laueClass1 == laueClass2 && laueClass1 < static_cast<uint32>(orientationOps.size()))
                      {
                        ebsdlib::AxisAngleDType axisAngle = orientationOps[laueClass1]->calculateMisorientation(quat1, quat2);
                        angle = axisAngle[3];
                      }
                    }
                    if(angle > misorientationTolerance)
                    {
                      disorientation++;
                    }
                  }
                  if(m_InputValues->UseMask)
                  {
                    if(refMaskBuf[refLocalIdx] != 0 && curMaskBuf[curLocalIdx] == 0)
                    {
                      disorientation++;
                    }
                    if(refMaskBuf[refLocalIdx] == 0 && curMaskBuf[curLocalIdx] != 0)
                    {
                      disorientation++;
                    }
                  }
                }
              }
            }
            disorientation = disorientation / count;
            xIdx = k + oldxshift + halfDim0;
            yIdx = j + oldyshift + halfDim1;
            idx = (dims[0] * yIdx) + xIdx;
            misorients[idx] = true;
            if(disorientation < minDisorientation || (disorientation == minDisorientation && ((llabs(k + oldxshift) < llabs(newxshift)) || (llabs(j + oldyshift) < llabs(newyshift)))))
            {
              newxshift = k + oldxshift;
              newyshift = j + oldyshift;
              minDisorientation = disorientation;
            }
          }
        }
      }
    }
    xShifts[iter] = xShifts[iter - 1] + newxshift;
    yShifts[iter] = yShifts[iter - 1] + newyshift;

    if(m_InputValues->StoreAlignmentShifts)
    {
      usize xIndex = iter * 2;
      usize yIndex = (iter * 2) + 1;
      (*slicesStorePtr)[xIndex] = slice;
      (*slicesStorePtr)[yIndex] = slice + 1;
      (*relativeShiftsStorePtr)[xIndex] = newxshift;
      (*relativeShiftsStorePtr)[yIndex] = newyshift;
      (*cumulativeShiftsStorePtr)[xIndex] = xShifts[iter];
      (*cumulativeShiftsStorePtr)[yIndex] = yShifts[iter];
    }

    // Current slice becomes the reference for the next iteration (O(1) pointer swap)
    std::swap(refQuatsBuf, curQuatsBuf);
    std::swap(refPhasesBuf, curPhasesBuf);
    if(m_InputValues->UseMask)
    {
      std::swap(refMaskBuf, curMaskBuf);
    }
  }

  return {};
}
