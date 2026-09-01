#include "EBSDSegmentFeatures.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <algorithm>
#include <memory>

using namespace nx::core;

EBSDSegmentFeatures::EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
  m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  m_IsPeriodic = inputValues->IsPeriodic;
}

EBSDSegmentFeatures::~EBSDSegmentFeatures() noexcept = default;

Result<> EBSDSegmentFeatures::operator()()
{
  this->m_NeighborScheme = m_InputValues->NeighborScheme;
  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxelsArray = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This fallback supports callers that bypass parameter preflight.
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-485090, message);
    }
  }
  m_CrystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  m_FeatureIdsArray->fill(0);

  SizeVec3 udims = gridGeom->getDimensions();
  auto allocateResult = allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));
  if(allocateResult.invalid())
  {
    return allocateResult;
  }

  auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
  const auto* maskArray = m_InputValues->UseMask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath) : nullptr;
  const bool usesOutOfCoreInput = IsOutOfCore(*m_QuatsArray) || IsOutOfCore(*m_CellPhases) || IsOutOfCore(*m_CrystalStructures) || (maskArray != nullptr && IsOutOfCore(*maskArray));
  Result<> segmentResult = executeCCL(gridGeom, featureIdsStore, usesOutOfCoreInput);

  deallocateSliceBuffers();

  if(segmentResult.invalid())
  {
    return segmentResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  if(m_FoundFeatures < 1)
  {
    return MakeErrorResult(-87000, "No Features were detected: no Cell was eligible to seed a Feature. Every Cell is either excluded by the Mask or has a Phase value of 0 (unindexed).");
  }

  // Feature tuple zero remains reserved for background cells.
  ShapeType tDims = {static_cast<usize>(m_FoundFeatures + 1)};
  auto& cellFeaturesAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
  cellFeaturesAM.resizeTuples(tDims);

  auto* activeArray = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath);
  activeArray->getDataStore()->fill(1);
  (*activeArray)[0] = 0;

  // Random IDs improve visual distinction between adjacent features.
  if(m_InputValues->RandomizeFeatureIds)
  {
    randomizeFeatureIds(m_FeatureIdsArray, m_FoundFeatures + 1);
  }

  return {};
}

bool EBSDSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz = point / m_BufSliceSize;
    const int slot = m_BufferedSliceZ[0] == iz ? 0 : (m_BufferedSliceZ[1] == iz ? 1 : -1);
    if(slot >= 0)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off = static_cast<usize>(slot) * sliceSize + static_cast<usize>(point - iz * m_BufSliceSize);
      if(m_InputValues->UseMask && m_MaskBuffer[off] == 0)
      {
        return false;
      }
      if(m_PhaseBuffer[off] <= 0)
      {
        return false;
      }
      return true;
    }
    return false;
  }

  if(m_InputValues->UseMask && !m_GoodVoxelsArray->isTrue(point))
  {
    return false;
  }
  AbstractDataStore<int32>& cellPhases = m_CellPhases->getDataStoreRef();
  if(cellPhases[point] <= 0)
  {
    return false;
  }
  return true;
}

bool EBSDSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz1 = point1 / m_BufSliceSize;
    const int slot1 = m_BufferedSliceZ[0] == iz1 ? 0 : (m_BufferedSliceZ[1] == iz1 ? 1 : -1);
    const int64 iz2 = point2 / m_BufSliceSize;
    const int slot2 = m_BufferedSliceZ[0] == iz2 ? 0 : (m_BufferedSliceZ[1] == iz2 ? 1 : -1);

    if(slot1 >= 0 && slot2 >= 0)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off1 = static_cast<usize>(slot1) * sliceSize + static_cast<usize>(point1 - iz1 * m_BufSliceSize);
      const usize off2 = static_cast<usize>(slot2) * sliceSize + static_cast<usize>(point2 - iz2 * m_BufSliceSize);

      if(m_InputValues->UseMask && m_MaskBuffer[off2] == 0)
      {
        return false;
      }
      const int32 phase1 = m_PhaseBuffer[off1];
      const int32 phase2 = m_PhaseBuffer[off2];
      if(phase2 <= 0)
      {
        return false;
      }
      if(phase1 != phase2)
      {
        return false;
      }

      int32 laueClass = static_cast<int32>(m_CrystalStructuresCache[static_cast<usize>(phase1)]);
      if(static_cast<usize>(laueClass) >= m_OrientationOps.size())
      {
        return false;
      }

      const usize q1Base = static_cast<usize>(slot1) * sliceSize * 4 + static_cast<usize>(point1 - iz1 * m_BufSliceSize) * 4;
      const usize q2Base = static_cast<usize>(slot2) * sliceSize * 4 + static_cast<usize>(point2 - iz2 * m_BufSliceSize) * 4;

      // Identical quaternions have exactly zero misorientation. This is the
      // overwhelmingly common case inside grains and avoids the substantially
      // more expensive symmetry-operator calculation without changing the
      // tolerance semantics.
      if(m_QuatBuffer[q1Base] == m_QuatBuffer[q2Base] && m_QuatBuffer[q1Base + 1] == m_QuatBuffer[q2Base + 1] && m_QuatBuffer[q1Base + 2] == m_QuatBuffer[q2Base + 2] &&
         m_QuatBuffer[q1Base + 3] == m_QuatBuffer[q2Base + 3])
      {
        return 0.0F < m_InputValues->MisorientationTolerance;
      }

      const ebsdlib::QuatD q1(m_QuatBuffer[q1Base], m_QuatBuffer[q1Base + 1], m_QuatBuffer[q1Base + 2], m_QuatBuffer[q1Base + 3]);
      const ebsdlib::QuatD q2(m_QuatBuffer[q2Base], m_QuatBuffer[q2Base + 1], m_QuatBuffer[q2Base + 2], m_QuatBuffer[q2Base + 3]);

      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
      float32 w = static_cast<float32>(axisAngle[3]);

      return w < m_InputValues->MisorientationTolerance;
    }
    return false;
  }

  if(!isValidVoxel(point2))
  {
    return false;
  }

  AbstractDataStore<int32>& cellPhases = m_CellPhases->getDataStoreRef();

  if(cellPhases[point1] != cellPhases[point2])
  {
    return false;
  }

  int32 laueClass = (*m_CrystalStructures)[cellPhases[point1]];
  if(static_cast<usize>(laueClass) >= m_OrientationOps.size())
  {
    return false;
  }

  Float32Array& quats = *m_QuatsArray;
  if(quats[point1 * 4] == quats[point2 * 4] && quats[point1 * 4 + 1] == quats[point2 * 4 + 1] && quats[point1 * 4 + 2] == quats[point2 * 4 + 2] && quats[point1 * 4 + 3] == quats[point2 * 4 + 3])
  {
    return 0.0F < m_InputValues->MisorientationTolerance;
  }

  const ebsdlib::QuatD q1(quats[point1 * 4], quats[point1 * 4 + 1], quats[point1 * 4 + 2], quats[point1 * 4 + 3]);
  const ebsdlib::QuatD q2(quats[point2 * 4], quats[point2 * 4 + 1], quats[point2 * 4 + 2], quats[point2 * 4 + 3]);

  ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
  float32 w = static_cast<float32>(axisAngle[3]);

  return w < m_InputValues->MisorientationTolerance;
}

Result<> EBSDSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  m_QuatBuffer.resize(2 * sliceSize * 4);
  m_PhaseBuffer.resize(2 * sliceSize);
  m_MaskBuffer.resize(2 * sliceSize);
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_BufferUseSequence = {0, 0};
  m_NextBufferUseSequence = 1;
  m_UseSliceBuffers = true;

  // The local crystal cache avoids inner-loop OOC access.
  const usize numPhases = m_CrystalStructures->getNumberOfTuples();
  m_CrystalStructuresCache.resize(numPhases);
  auto readResult = m_CrystalStructures->getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(m_CrystalStructuresCache.data(), numPhases));
  if(readResult.invalid())
  {
    return readResult;
  }
  return {};
}

void EBSDSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_QuatBuffer = std::vector<float32>();
  m_PhaseBuffer = std::vector<int32>();
  m_MaskBuffer = std::vector<uint8>();
  m_CrystalStructuresCache = std::vector<uint32>();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_BufferUseSequence = {0, 0};
  m_NextBufferUseSequence = 1;
}

Result<> EBSDSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return {};
  }

  if(!m_UseSliceBuffers)
  {
    return {};
  }

  int slot = m_BufferedSliceZ[0] == iz ? 0 : (m_BufferedSliceZ[1] == iz ? 1 : -1);
  if(slot >= 0)
  {
    m_BufferUseSequence[static_cast<usize>(slot)] = m_NextBufferUseSequence++;
    return {};
  }
  if(m_BufferedSliceZ[0] < 0)
  {
    slot = 0;
  }
  else if(m_BufferedSliceZ[1] < 0)
  {
    slot = 1;
  }
  else
  {
    slot = m_BufferUseSequence[0] <= m_BufferUseSequence[1] ? 0 : 1;
  }

  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  const usize slotOffset = static_cast<usize>(slot) * sliceSize;
  const usize quatSlotOffset = slotOffset * 4;
  const int64 baseIndex = iz * m_BufSliceSize;

  AbstractDataStore<float32>& quatStore = m_QuatsArray->getDataStoreRef();
  auto quatReadResult = quatStore.copyIntoBuffer(static_cast<usize>(baseIndex) * 4, nonstd::span<float32>(m_QuatBuffer.data() + quatSlotOffset, sliceSize * 4));
  if(quatReadResult.invalid())
  {
    return quatReadResult;
  }

  AbstractDataStore<int32>& phaseStore = m_CellPhases->getDataStoreRef();
  auto phaseReadResult = phaseStore.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<int32>(m_PhaseBuffer.data() + slotOffset, sliceSize));
  if(phaseReadResult.invalid())
  {
    return phaseReadResult;
  }

  if(m_InputValues->UseMask && m_GoodVoxelsArray != nullptr)
  {
    auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    if(maskArray.getDataType() == DataType::uint8)
    {
      auto& typedStore = maskArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
      auto maskReadResult = typedStore.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<uint8>(m_MaskBuffer.data() + slotOffset, sliceSize));
      if(maskReadResult.invalid())
      {
        return maskReadResult;
      }
    }
    else if(maskArray.getDataType() == DataType::boolean)
    {
      auto& typedStore = maskArray.getIDataStoreRefAs<AbstractDataStore<bool>>();
      auto boolBuf = std::make_unique<bool[]>(sliceSize);
      auto maskReadResult = typedStore.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<bool>(boolBuf.get(), sliceSize));
      if(maskReadResult.invalid())
      {
        return maskReadResult;
      }
      for(usize i = 0; i < sliceSize; i++)
      {
        m_MaskBuffer[slotOffset + i] = boolBuf[i] ? 1 : 0;
      }
    }
    else
    {
      return MakeErrorResult(-485091, "EBSDSegmentFeatures mask storage must be Bool or UInt8.");
    }
  }
  else
  {
    std::fill(m_MaskBuffer.begin() + slotOffset, m_MaskBuffer.begin() + slotOffset + sliceSize, static_cast<uint8>(1));
  }

  m_BufferedSliceZ[slot] = iz;
  m_BufferUseSequence[static_cast<usize>(slot)] = m_NextBufferUseSequence++;
  return {};
}
