#include "EBSDSegmentFeatures.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"

#include <algorithm>

using namespace nx::core;

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
  m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();
  m_IsPeriodic = inputValues->IsPeriodic;
}

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::~EBSDSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
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
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-485090, message);
    }
  }
  m_CrystalStructures = m_DataStructure.getDataAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  m_FeatureIdsArray->fill(0); // initialize the output array with zeros

  // Dispatch between DFS (in-core) and CCL (OOC) algorithms
  if(IsOutOfCore(*m_FeatureIdsArray) || ForceOocAlgorithm())
  {
    SizeVec3 udims = gridGeom->getDimensions();
    allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));

    auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
    executeCCL(gridGeom, featureIdsStore);

    deallocateSliceBuffers();
  }
  else
  {
    execute(gridGeom);
  }
  // Sanity check the result.
  if(this->m_FoundFeatures < 1)
  {
    return MakeErrorResult(-87000, fmt::format("The number of Features is '{}' which means no Features were detected. A threshold value may be set incorrectly", this->m_FoundFeatures));
  }

  // Resize the Feature Attribute Matrix
  ShapeType tDims = {static_cast<usize>(this->m_FoundFeatures + 1)};
  auto& cellFeaturesAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
  cellFeaturesAM.resizeTuples(tDims); // This will resize the active array

  // make sure all values are initialized and "re-reserve" index 0
  auto* activeArray = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath);
  activeArray->getDataStore()->fill(1);
  (*activeArray)[0] = 0;

  // Randomize the feature Ids for purely visual clarify. Having random Feature Ids
  // allows users visualizing the data to better discern each grain otherwise the coloring
  // would look like a smooth gradient. This is a user input parameter
  if(m_InputValues->RandomizeFeatureIds)
  {
    randomizeFeatureIds(m_FeatureIdsArray, this->m_FoundFeatures + 1);
  }

  return {};
}

// -----------------------------------------------------------------------------
int64_t EBSDSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  nx::core::DataArray<int32>::store_type* featureIds = m_FeatureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();

  nx::core::AbstractDataStore<int32>* cellPhases = m_CellPhases->getDataStore();

  int64 seed = -1;
  // start with the next voxel after the last seed
  auto randPoint = static_cast<usize>(nextSeed);
  while(seed == -1 && randPoint < totalPoints)
  {
    if(featureIds->getValue(randPoint) == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if((!m_InputValues->UseMask || m_GoodVoxelsArray->isTrue(randPoint)) && cellPhases->getValue(randPoint) > 0)
      {
        seed = static_cast<int64>(randPoint);
      }
      else
      {
        randPoint += 1;
      }
    }
    else
    {
      randPoint += 1;
    }
  }
  if(seed >= 0)
  {
    featureIds->setValue(static_cast<usize>(seed), gnum);
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::determineGrouping(int64 referencePoint, int64 neighborPoint, int32 gnum) const
{
  bool group = false;

  // Get the phases for each voxel
  nx::core::AbstractDataStore<int32>* cellPhases = m_CellPhases->getDataStore();

  int32_t laueClass1 = (*m_CrystalStructures)[(*cellPhases)[referencePoint]];
  int32_t laueClass2 = (*m_CrystalStructures)[(*cellPhases)[neighborPoint]];
  // If either of the phases is 999 then we bail out now.
  if(laueClass1 >= m_OrientationOps.size() || laueClass2 >= m_OrientationOps.size())
  {
    return group;
  }
  Float32Array& currentQuatPtr = *m_QuatsArray;
  Int32Array& featureIds = *m_FeatureIdsArray;

  bool neighborPointIsGood = false;
  if(m_GoodVoxelsArray != nullptr)
  {
    neighborPointIsGood = m_GoodVoxelsArray->isTrue(neighborPoint);
  }

  if(featureIds[neighborPoint] == 0 && (m_GoodVoxelsArray == nullptr || neighborPointIsGood))
  {
    float w = std::numeric_limits<float>::max();
    const ebsdlib::QuatD q1(currentQuatPtr[referencePoint * 4], currentQuatPtr[referencePoint * 4 + 1], currentQuatPtr[referencePoint * 4 + 2], currentQuatPtr[referencePoint * 4 + 3]);
    const ebsdlib::QuatD q2(currentQuatPtr[neighborPoint * 4], currentQuatPtr[neighborPoint * 4 + 1], currentQuatPtr[neighborPoint * 4 + 2], currentQuatPtr[neighborPoint * 4 + 3]);

    if((*cellPhases)[referencePoint] == (*cellPhases)[neighborPoint])
    {
      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass1]->calculateMisorientation(q1, q2);
      w = static_cast<float>(axisAngle[3]);
    }
    if(w < m_InputValues->MisorientationTolerance)
    {
      group = true;
      featureIds[neighborPoint] = gnum;
    }
  }

  return group;
}

// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz = point / m_BufSliceSize;
    const int slot = static_cast<int>(iz % 2);
    if(m_BufferedSliceZ[slot] == iz)
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
  }

  // OOC fallback
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

// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz1 = point1 / m_BufSliceSize;
    const int slot1 = static_cast<int>(iz1 % 2);
    const int64 iz2 = point2 / m_BufSliceSize;
    const int slot2 = static_cast<int>(iz2 % 2);

    if(m_BufferedSliceZ[slot1] == iz1 && m_BufferedSliceZ[slot2] == iz2)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off1 = static_cast<usize>(slot1) * sliceSize + static_cast<usize>(point1 - iz1 * m_BufSliceSize);
      const usize off2 = static_cast<usize>(slot2) * sliceSize + static_cast<usize>(point2 - iz2 * m_BufSliceSize);

      // Check point2 validity
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

      int32 laueClass = (*m_CrystalStructures)[phase1];
      if(static_cast<usize>(laueClass) >= m_OrientationOps.size())
      {
        return false;
      }

      const usize q1Base = static_cast<usize>(slot1) * sliceSize * 4 + static_cast<usize>(point1 - iz1 * m_BufSliceSize) * 4;
      const usize q2Base = static_cast<usize>(slot2) * sliceSize * 4 + static_cast<usize>(point2 - iz2 * m_BufSliceSize) * 4;

      const ebsdlib::QuatD q1(m_QuatBuffer[q1Base], m_QuatBuffer[q1Base + 1], m_QuatBuffer[q1Base + 2], m_QuatBuffer[q1Base + 3]);
      const ebsdlib::QuatD q2(m_QuatBuffer[q2Base], m_QuatBuffer[q2Base + 1], m_QuatBuffer[q2Base + 2], m_QuatBuffer[q2Base + 3]);

      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
      float w = static_cast<float>(axisAngle[3]);

      return w < m_InputValues->MisorientationTolerance;
    }
  }

  // OOC fallback (original code)
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
  const ebsdlib::QuatD q1(quats[point1 * 4], quats[point1 * 4 + 1], quats[point1 * 4 + 2], quats[point1 * 4 + 3]);
  const ebsdlib::QuatD q2(quats[point2 * 4], quats[point2 * 4 + 1], quats[point2 * 4 + 2], quats[point2 * 4 + 3]);

  ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);
  float w = static_cast<float>(axisAngle[3]);

  return w < m_InputValues->MisorientationTolerance;
}

// -----------------------------------------------------------------------------
void EBSDSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  m_QuatBuffer.resize(2 * sliceSize * 4);
  m_PhaseBuffer.resize(2 * sliceSize);
  m_MaskBuffer.resize(2 * sliceSize);
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_UseSliceBuffers = true;
}

// -----------------------------------------------------------------------------
void EBSDSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_QuatBuffer = std::vector<float32>();
  m_PhaseBuffer = std::vector<int32>();
  m_MaskBuffer = std::vector<uint8>();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
}

// -----------------------------------------------------------------------------
void EBSDSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return;
  }

  if(!m_UseSliceBuffers)
  {
    return;
  }

  const int slot = static_cast<int>(iz % 2);
  if(m_BufferedSliceZ[slot] == iz)
  {
    return;
  }

  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  const usize slotOffset = static_cast<usize>(slot) * sliceSize;
  const usize quatSlotOffset = slotOffset * 4;
  const int64 baseIndex = iz * m_BufSliceSize;

  AbstractDataStore<float32>& quatStore = m_QuatsArray->getDataStoreRef();
  for(usize i = 0; i < sliceSize; i++)
  {
    const usize srcBase = static_cast<usize>(baseIndex + static_cast<int64>(i)) * 4;
    const usize dstBase = quatSlotOffset + i * 4;
    m_QuatBuffer[dstBase + 0] = quatStore[srcBase + 0];
    m_QuatBuffer[dstBase + 1] = quatStore[srcBase + 1];
    m_QuatBuffer[dstBase + 2] = quatStore[srcBase + 2];
    m_QuatBuffer[dstBase + 3] = quatStore[srcBase + 3];
  }

  AbstractDataStore<int32>& phaseStore = m_CellPhases->getDataStoreRef();
  for(usize i = 0; i < sliceSize; i++)
  {
    m_PhaseBuffer[slotOffset + i] = phaseStore[static_cast<usize>(baseIndex) + i];
  }

  if(m_InputValues->UseMask && m_GoodVoxelsArray != nullptr)
  {
    for(usize i = 0; i < sliceSize; i++)
    {
      m_MaskBuffer[slotOffset + i] = m_GoodVoxelsArray->isTrue(static_cast<usize>(baseIndex) + i) ? 1 : 0;
    }
  }
  else
  {
    std::fill(m_MaskBuffer.begin() + slotOffset, m_MaskBuffer.begin() + slotOffset + sliceSize, static_cast<uint8>(1));
  }

  m_BufferedSliceZ[slot] = iz;
}
