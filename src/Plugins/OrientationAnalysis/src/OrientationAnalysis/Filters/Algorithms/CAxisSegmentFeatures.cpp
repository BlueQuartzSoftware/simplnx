#include "CAxisSegmentFeatures.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ClusteringUtilities.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"

#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <cmath>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
CAxisSegmentFeatures::CAxisSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CAxisSegmentFeaturesInputValues* inputValues)
: AbstractSegmentFeatures(dataStructure, shouldCancel, mesgHandler)
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

  // Run the segmentation algorithm
  execute(imageGeometry);
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
    auto& cellFeatureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
    featureIds[static_cast<usize>(seed)] = gnum;
    const ShapeType tDims = {static_cast<usize>(gnum) + 1};
    cellFeatureAM.resizeTuples(tDims); // This will resize the active array
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
