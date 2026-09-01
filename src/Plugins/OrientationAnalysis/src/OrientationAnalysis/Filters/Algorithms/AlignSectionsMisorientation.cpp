#include "AlignSectionsMisorientation.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

AlignSectionsMisorientation::AlignSectionsMisorientation(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                         AlignSectionsMisorientationInputValues* inputValues)
: AlignSections(dataStructure, shouldCancel, mesgHandler)
, m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

AlignSectionsMisorientation::~AlignSectionsMisorientation() noexcept = default;

Result<> AlignSectionsMisorientation::operator()()
{
  if(m_ShouldCancel)
  {
    return {};
  }
  const auto& gridGeom = m_DataStructure.getDataRefAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  return execute(gridGeom.getDimensions(), m_InputValues->ImageGeometryPath);
}

Result<> AlignSectionsMisorientation::findShifts(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  bool usesOutOfCoreStore = false;
  // OOC dispatch must cover every array touched by shift discovery or the
  // subsequent ImageGeom cell-array shift.
  {
    const auto& quatsCheck = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->quatsArrayPath);
    const auto& cellPhasesCheck = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
    const auto& crystalStructuresCheck = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->crystalStructuresArrayPath);
    usesOutOfCoreStore = IsOutOfCore(quatsCheck) || IsOutOfCore(cellPhasesCheck) || IsOutOfCore(crystalStructuresCheck);
    if(m_InputValues->UseMask)
    {
      usesOutOfCoreStore = usesOutOfCoreStore || IsOutOfCore(m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath));
    }
    for(const auto& path : getSelectedDataPaths(m_InputValues->ImageGeometryPath))
    {
      const auto* array = m_DataStructure.getDataAs<IDataArray>(path);
      usesOutOfCoreStore = usesOutOfCoreStore || (array != nullptr && IsOutOfCore(*array));
    }
    if(!ForceInCoreAlgorithm() && (ForceOocAlgorithm() || usesOutOfCoreStore))
    {
      RecordAlgorithmPathExecution(AlgorithmPath::OutOfCore, usesOutOfCoreStore);
      return findShiftsOoc(xShifts, yShifts);
    }
  }

  RecordAlgorithmPathExecution(AlgorithmPath::InCore, usesOutOfCoreStore);

  std::unique_ptr<MaskCompareUtilities::MaskCompare> maskCompare = nullptr;
  if(m_InputValues->UseMask)
  {
    try
    {
      maskCompare = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This fallback supports callers that bypass parameter preflight.
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

  // Reuse candidate flags for each adjacent slice pair.
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
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Shifts || {:.2f}% Complete", CalculatePercentComplete(iter, dims[2])); });
      if(getCancel())
      {
        return {};
      }
      float32 minDisorientation = std::numeric_limits<float32>::max();
      // Traverse from the top slice toward the bottom slice.
      int64 slice = (dims[2] - 1) - iter;
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
    for(int64 iter = 1; iter < dims[2]; iter++)
    {
      throttledMessenger.sendThrottledMessage([&]() { return fmt::format("Determining Shifts || {:.2f}% Complete", CalculatePercentComplete(iter, dims[2])); });
      if(getCancel())
      {
        return {};
      }
      float32 minDisorientation = std::numeric_limits<float32>::max();
      // Traverse from the top slice toward the bottom slice.
      int64 slice = (dims[2] - 1) - iter;
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

Result<> AlignSectionsMisorientation::findShiftsOoc(std::vector<int64>& xShifts, std::vector<int64>& yShifts)
{
  // Bulk reads avoid per-element OOC mask access.
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

  // The local ensemble cache avoids hot-loop store access.
  const auto& crystalStructuresStore = crystalStructuresArray.getDataStoreRef();
  std::vector<uint32> crystalStructures(crystalStructuresStore.getSize());
  auto crystalReadResult = crystalStructuresStore.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()));
  if(crystalReadResult.invalid())
  {
    return crystalReadResult;
  }

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

  std::vector<float32> refQuatsBuf(sliceVoxels * 4);
  std::vector<float32> curQuatsBuf(sliceVoxels * 4);
  std::vector<int32> refPhasesBuf(sliceVoxels);
  std::vector<int32> curPhasesBuf(sliceVoxels);
  std::vector<uint8> refMaskBuf;
  std::vector<uint8> curMaskBuf;
  std::unique_ptr<bool[]> boolMaskBuffer;
  if(m_InputValues->UseMask)
  {
    refMaskBuf.resize(sliceVoxels, 1);
    curMaskBuf.resize(sliceVoxels, 1);
    if(maskBoolStorePtr != nullptr)
    {
      boolMaskBuffer = std::make_unique<bool[]>(sliceVoxels);
    }
  }

  AbstractDataStore<uint32>* slicesStorePtr = nullptr;
  AbstractDataStore<int64>* relativeShiftsStorePtr = nullptr;
  AbstractDataStore<int64>* cumulativeShiftsStorePtr = nullptr;
  if(m_InputValues->StoreAlignmentShifts)
  {
    slicesStorePtr = &m_DataStructure.getDataAs<UInt32Array>(m_InputValues->SlicesArrayPath)->getDataStoreRef();
    relativeShiftsStorePtr = &m_DataStructure.getDataAs<Int64Array>(m_InputValues->RelativeShiftsArrayPath)->getDataStoreRef();
    cumulativeShiftsStorePtr = &m_DataStructure.getDataAs<Int64Array>(m_InputValues->CumulativeShiftsArrayPath)->getDataStoreRef();
  }

  // The first pair uses the top slice as its reference.
  {
    int64 firstRefOffset = (dims[2] - 1) * sliceVoxels;
    auto phaseReadResult = cellPhasesStore.copyIntoBuffer(firstRefOffset, nonstd::span<int32>(refPhasesBuf.data(), sliceVoxels));
    if(phaseReadResult.invalid())
      return phaseReadResult;
    auto quatReadResult = quatsStore.copyIntoBuffer(firstRefOffset * 4, nonstd::span<float32>(refQuatsBuf.data(), sliceVoxels * 4));
    if(quatReadResult.invalid())
      return quatReadResult;
    if(m_InputValues->UseMask)
    {
      if(maskUInt8StorePtr != nullptr)
      {
        auto maskReadResult = maskUInt8StorePtr->copyIntoBuffer(firstRefOffset, nonstd::span<uint8>(refMaskBuf.data(), sliceVoxels));
        if(maskReadResult.invalid())
          return maskReadResult;
      }
      else if(maskBoolStorePtr != nullptr)
      {
        auto maskReadResult = maskBoolStorePtr->copyIntoBuffer(firstRefOffset, nonstd::span<bool>(boolMaskBuffer.get(), sliceVoxels));
        if(maskReadResult.invalid())
          return maskReadResult;
        for(int64 idx = 0; idx < sliceVoxels; idx++)
        {
          refMaskBuf[idx] = boolMaskBuffer[idx] ? 1 : 0;
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

    // The reference buffer holds the prior slice after the first iteration.
    int64 curOffset = slice * sliceVoxels;
    auto phaseReadResult = cellPhasesStore.copyIntoBuffer(curOffset, nonstd::span<int32>(curPhasesBuf.data(), sliceVoxels));
    if(phaseReadResult.invalid())
      return phaseReadResult;
    auto quatReadResult = quatsStore.copyIntoBuffer(curOffset * 4, nonstd::span<float32>(curQuatsBuf.data(), sliceVoxels * 4));
    if(quatReadResult.invalid())
      return quatReadResult;
    if(m_InputValues->UseMask)
    {
      if(maskUInt8StorePtr != nullptr)
      {
        auto maskReadResult = maskUInt8StorePtr->copyIntoBuffer(curOffset, nonstd::span<uint8>(curMaskBuf.data(), sliceVoxels));
        if(maskReadResult.invalid())
          return maskReadResult;
      }
      else if(maskBoolStorePtr != nullptr)
      {
        auto maskReadResult = maskBoolStorePtr->copyIntoBuffer(curOffset, nonstd::span<bool>(boolMaskBuffer.get(), sliceVoxels));
        if(maskReadResult.invalid())
          return maskReadResult;
        for(int64 idx = 0; idx < sliceVoxels; idx++)
        {
          curMaskBuf[idx] = boolMaskBuffer[idx] ? 1 : 0;
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

    // Reuse the current slice as the next reference without another read.
    std::swap(refQuatsBuf, curQuatsBuf);
    std::swap(refPhasesBuf, curPhasesBuf);
    if(m_InputValues->UseMask)
    {
      std::swap(refMaskBuf, curMaskBuf);
    }
  }

  return {};
}
