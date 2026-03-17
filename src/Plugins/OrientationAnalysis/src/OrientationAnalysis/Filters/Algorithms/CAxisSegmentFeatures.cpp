#include "CAxisSegmentFeatures.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"

#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <algorithm>
#include <cmath>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
CAxisSegmentFeatures::CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
CAxisSegmentFeatures::~CAxisSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> CAxisSegmentFeatures::operator()()
{
  this->m_NeighborScheme = m_InputValues->NeighborScheme;
  auto* imageGeometry = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxelsArray = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range&)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      return MakeErrorResult(-8362, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }

  // Loop through all the "Phase" cell values and validate that any phase found is
  // a hexagonal phase. This guards against there being multiple phases defined in
  // and EBSD file but the non-hexagonal phases were actually never found
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  usize numCells = m_CellPhases->getNumberOfTuples();
  for(usize cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    int32 currentPhaseIdx = m_CellPhases->getValue(cellIdx);
    const auto crystalStructureType = crystalStructures[currentPhaseIdx];
    if(crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_Low)
    {
      return MakeErrorResult(-8363, fmt::format("Input data is using {} type crystal structures but segmenting features via c-axis mis orientation requires all phases to be either Hexagonal-Low 6/m "
                                                "or Hexagonal-High 6/mmm type crystal structures.",
                                                CrystalStructureEnumToString(crystalStructureType)));
    }
  }

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  m_FeatureIdsArray->fill(0);
  auto* active = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath);
  active->fill(1);

  // Dispatch between DFS (in-core) and CCL (OOC) algorithms
  if(IsOutOfCore(*m_FeatureIdsArray) || ForceOocAlgorithm())
  {
    SizeVec3 udims = imageGeometry->getDimensions();
    allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));

    auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
    executeCCL(imageGeometry, featureIdsStore);

    deallocateSliceBuffers();
  }
  else
  {
    execute(imageGeometry);
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
    ClusterUtilities::RandomizeFeatureIds(m_FeatureIdsArray->getDataStoreRef(), this->m_FoundFeatures + 1);
  }

  return {};
}

// -----------------------------------------------------------------------------
int64 CAxisSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  DataArray<int32>::store_type& featureIds = m_FeatureIdsArray->getDataStoreRef();
  const usize totalPoints = featureIds.getNumberOfTuples();
  AbstractDataStore<int32>& cellPhases = m_CellPhases->getDataStoreRef();

  // start with the next voxel after the last seed
  auto randPoint = static_cast<usize>(nextSeed);
  int64 seed = -1;
  while(seed == -1 && randPoint < totalPoints)
  {
    if(featureIds[randPoint] == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if((!m_InputValues->UseMask || m_GoodVoxelsArray->isTrue(randPoint)) && cellPhases[randPoint] > 0)
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
    featureIds[static_cast<usize>(seed)] = gnum;
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::determineGrouping(int64 referencepoint, int64 neighborpoint, int32 gnum) const
{
  bool group = false;

  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Float32Array& currentQuat = *m_QuatsArray;
  Int32Array& featureIds = *m_FeatureIdsArray;
  Int32Array& cellPhases = *m_CellPhases;

  bool neighborPointIsGood = false;
  if(m_GoodVoxelsArray != nullptr)
  {
    neighborPointIsGood = m_GoodVoxelsArray->isTrue(neighborpoint);
  }

  if(featureIds[neighborpoint] == 0 && (!m_InputValues->UseMask || neighborPointIsGood))
  {
    if(cellPhases[referencepoint] == cellPhases[neighborpoint])
    {
      const ebsdlib::QuatF q1(currentQuat[referencepoint * 4], currentQuat[referencepoint * 4 + 1], currentQuat[referencepoint * 4 + 2], currentQuat[referencepoint * 4 + 3]);
      const ebsdlib::QuatF q2(currentQuat[neighborpoint * 4 + 0], currentQuat[neighborpoint * 4 + 1], currentQuat[neighborpoint * 4 + 2], currentQuat[neighborpoint * 4 + 3]);

      const ebsdlib::OrientationMatrixFType oMatrix1 = q1.toOrientationMatrix();
      const ebsdlib::OrientationMatrixFType oMatrix2 = q2.toOrientationMatrix();

      // Convert the quaternion matrices to transposed g matrices so when caxis is multiplied by it, it will give the sample direction that the caxis is along
      Eigen::Vector3f c1 = oMatrix1.transpose() * cAxis;
      Eigen::Vector3f c2 = oMatrix2.transpose() * cAxis;

      // normalize so that the dot product can be taken below without
      // dividing by the magnitudes (they would be 1)
      c1.normalize();
      c2.normalize();

      // Validate value of w falls between [-1, 1] to ensure that acos returns a valid value
      float32 w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
      w = std::acos(w);
      if(w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance)
      {
        group = true;
        featureIds[neighborpoint] = gnum;
      }
    }
  }
  return group;
}

// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    int64 sliceZ = point / m_BufSliceSize;
    if(sliceZ == m_BufferedSliceZ[0] || sliceZ == m_BufferedSliceZ[1])
    {
      int64 slot = (sliceZ == m_BufferedSliceZ[0]) ? 0 : 1;
      int64 offset = point - sliceZ * m_BufSliceSize;
      int64 bufIdx = slot * m_BufSliceSize + offset;
      // Check mask
      if(m_InputValues->UseMask && m_MaskBuffer[bufIdx] == 0)
      {
        return false;
      }
      // Check phase
      if(m_PhaseBuffer[bufIdx] <= 0)
      {
        return false;
      }
      return true;
    }
  }

  // Fallback: direct array access
  if(m_InputValues->UseMask && !m_GoodVoxelsArray->isTrue(point))
  {
    return false;
  }
  Int32Array& cellPhases = *m_CellPhases;
  if(cellPhases[point] <= 0)
  {
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    int64 sliceZ1 = point1 / m_BufSliceSize;
    int64 sliceZ2 = point2 / m_BufSliceSize;
    bool buf1 = (sliceZ1 == m_BufferedSliceZ[0] || sliceZ1 == m_BufferedSliceZ[1]);
    bool buf2 = (sliceZ2 == m_BufferedSliceZ[0] || sliceZ2 == m_BufferedSliceZ[1]);

    if(buf1 && buf2)
    {
      int64 slot1 = (sliceZ1 == m_BufferedSliceZ[0]) ? 0 : 1;
      int64 slot2 = (sliceZ2 == m_BufferedSliceZ[0]) ? 0 : 1;
      int64 off1 = point1 - sliceZ1 * m_BufSliceSize;
      int64 off2 = point2 - sliceZ2 * m_BufSliceSize;
      int64 bufIdx1 = slot1 * m_BufSliceSize + off1;
      int64 bufIdx2 = slot2 * m_BufSliceSize + off2;

      // Check point2 validity (mask + phase)
      if(m_InputValues->UseMask && m_MaskBuffer[bufIdx2] == 0)
      {
        return false;
      }
      if(m_PhaseBuffer[bufIdx2] <= 0)
      {
        return false;
      }

      // Must be same phase
      if(m_PhaseBuffer[bufIdx1] != m_PhaseBuffer[bufIdx2])
      {
        return false;
      }

      // Read quaternions from buffer
      int64 qIdx1 = bufIdx1 * 4;
      int64 qIdx2 = bufIdx2 * 4;
      const ebsdlib::QuatF q1(m_QuatBuffer[qIdx1], m_QuatBuffer[qIdx1 + 1], m_QuatBuffer[qIdx1 + 2], m_QuatBuffer[qIdx1 + 3]);
      const ebsdlib::QuatF q2(m_QuatBuffer[qIdx2], m_QuatBuffer[qIdx2 + 1], m_QuatBuffer[qIdx2 + 2], m_QuatBuffer[qIdx2 + 3]);

      const ebsdlib::OrientationMatrixFType oMatrix1 = q1.toOrientationMatrix();
      const ebsdlib::OrientationMatrixFType oMatrix2 = q2.toOrientationMatrix();

      const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
      Eigen::Vector3f c1 = oMatrix1.transpose() * cAxis;
      Eigen::Vector3f c2 = oMatrix2.transpose() * cAxis;

      c1.normalize();
      c2.normalize();

      float32 w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
      w = std::acos(w);

      return w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance;
    }
  }

  // Fallback: direct array access
  if(!isValidVoxel(point2))
  {
    return false;
  }

  Int32Array& cellPhases = *m_CellPhases;

  // Must be same phase
  if(cellPhases[point1] != cellPhases[point2])
  {
    return false;
  }

  // Calculate c-axis misalignment
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Float32Array& quats = *m_QuatsArray;

  const ebsdlib::QuatF q1(quats[point1 * 4], quats[point1 * 4 + 1], quats[point1 * 4 + 2], quats[point1 * 4 + 3]);
  const ebsdlib::QuatF q2(quats[point2 * 4], quats[point2 * 4 + 1], quats[point2 * 4 + 2], quats[point2 * 4 + 3]);

  const ebsdlib::OrientationMatrixFType oMatrix1 = q1.toOrientationMatrix();
  const ebsdlib::OrientationMatrixFType oMatrix2 = q2.toOrientationMatrix();

  Eigen::Vector3f c1 = oMatrix1.transpose() * cAxis;
  Eigen::Vector3f c2 = oMatrix2.transpose() * cAxis;

  c1.normalize();
  c2.normalize();

  float32 w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
  w = std::acos(w);

  return w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance;
}

// -----------------------------------------------------------------------------
void CAxisSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  int64 totalSlots = 2 * m_BufSliceSize;
  m_QuatBuffer.resize(static_cast<usize>(totalSlots * 4));
  m_PhaseBuffer.resize(static_cast<usize>(totalSlots));
  m_MaskBuffer.resize(static_cast<usize>(totalSlots));
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_UseSliceBuffers = true;
}

// -----------------------------------------------------------------------------
void CAxisSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_QuatBuffer.clear();
  m_QuatBuffer.shrink_to_fit();
  m_PhaseBuffer.clear();
  m_PhaseBuffer.shrink_to_fit();
  m_MaskBuffer.clear();
  m_MaskBuffer.shrink_to_fit();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_BufSliceSize = 0;
}

// -----------------------------------------------------------------------------
void CAxisSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return;
  }

  int64 slot = iz % 2;
  m_BufferedSliceZ[slot] = iz;

  int64 sliceStart = iz * m_BufSliceSize;
  int64 bufOffset = slot * m_BufSliceSize;

  // Fill quaternion buffer (4 components per voxel)
  for(int64 i = 0; i < m_BufSliceSize; i++)
  {
    int64 srcIdx = (sliceStart + i) * 4;
    int64 dstIdx = (bufOffset + i) * 4;
    m_QuatBuffer[dstIdx] = m_QuatsArray->getValue(srcIdx);
    m_QuatBuffer[dstIdx + 1] = m_QuatsArray->getValue(srcIdx + 1);
    m_QuatBuffer[dstIdx + 2] = m_QuatsArray->getValue(srcIdx + 2);
    m_QuatBuffer[dstIdx + 3] = m_QuatsArray->getValue(srcIdx + 3);
  }

  // Fill phase buffer
  for(int64 i = 0; i < m_BufSliceSize; i++)
  {
    m_PhaseBuffer[bufOffset + i] = m_CellPhases->getValue(sliceStart + i);
  }

  // Fill mask buffer
  if(m_InputValues->UseMask && m_GoodVoxelsArray != nullptr)
  {
    for(int64 i = 0; i < m_BufSliceSize; i++)
    {
      m_MaskBuffer[bufOffset + i] = m_GoodVoxelsArray->isTrue(sliceStart + i) ? 1 : 0;
    }
  }
  else
  {
    // If no mask, mark everything as valid
    std::fill(m_MaskBuffer.begin() + bufOffset, m_MaskBuffer.begin() + bufOffset + m_BufSliceSize, 1);
  }
}
