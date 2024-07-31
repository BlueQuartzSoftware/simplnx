#include "CAxisSegmentFeatures.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"

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
const std::atomic_bool& CAxisSegmentFeatures::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> CAxisSegmentFeatures::operator()()
{
  auto* imageGeometry = m_DataStructure.getDataAs<ImageGeom>(m_InputValues->ImageGeometryPath);
  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxelsArray = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range&)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      return MakeErrorResult(-8362, fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString()));
    }
  }
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  for(usize i = 1; i < crystalStructures.size(); ++i)
  {
    const auto crystalStructureType = crystalStructures[i];
    if(crystalStructureType != EbsdLib::CrystalStructure::Hexagonal_High && crystalStructureType != EbsdLib::CrystalStructure::Hexagonal_Low)
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

  // Generate the random voxel indices that will be used for the seed points to start a new grain growth/agglomeration
  execute(imageGeometry);

  const auto totalFeatures = static_cast<int64>(active->getNumberOfTuples());
  if(totalFeatures < 2)
  {
    return MakeErrorResult(-8362, "The number of Features was 0 or 1 which means no Features were detected. A threshold value may be set too high");
  }

  // By default we randomize grains
  if(m_InputValues->RandomizeFeatureIds)
  {
    randomizeFeatureIds(m_FeatureIdsArray, totalFeatures);
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
    auto& cellFeatureAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixName);
    featureIds[static_cast<usize>(seed)] = gnum;
    const std::vector<usize> tDims = {static_cast<usize>(gnum) + 1};
    cellFeatureAM.resizeTuples(tDims); // This will resize the active array
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool CAxisSegmentFeatures::determineGrouping(int64 referencepoint, int64 neighborpoint, int32 gnum) const
{
  bool group = false;
  float32 w = std::numeric_limits<float32>::max();

  Matrix3fR g1T;
  g1T.fill(0.0f);
  Matrix3fR g2T;
  g2T.fill(0.0f);
  const Eigen::Vector3f cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3f c1{0.0f, 0.0f, 0.0f};
  Eigen::Vector3f c2{0.0f, 0.0f, 0.0f};
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
    const QuatF q1(currentQuat[referencepoint * 4], currentQuat[referencepoint * 4 + 1], currentQuat[referencepoint * 4 + 2], currentQuat[referencepoint * 4 + 3]);
    const QuatF q2(currentQuat[neighborpoint * 4 + 0], currentQuat[neighborpoint * 4 + 1], currentQuat[neighborpoint * 4 + 2], currentQuat[neighborpoint * 4 + 3]);

    if(cellPhases[referencepoint] == cellPhases[neighborpoint])
    {
      const OrientationF oMatrix1 = OrientationTransformation::qu2om<QuatF, Orientation<float32>>(q1);
      const OrientationF oMatrix2 = OrientationTransformation::qu2om<QuatF, Orientation<float32>>(q2);

      // Convert the quaternion matrices to transposed g matrices so when caxis is multiplied by it, it will give the sample direction that the caxis is along
      g1T = OrientationMatrixToGMatrixTranspose(oMatrix1);
      g2T = OrientationMatrixToGMatrixTranspose(oMatrix2);

      c1 = g1T * cAxis;
      c2 = g2T * cAxis;

      // normalize so that the dot product can be taken below without
      // dividing by the magnitudes (they would be 1)
      c1.normalize();
      c2.normalize();

      // Validate value of w falls between [-1, 1] to ensure that acos returns a valid value
      w = std::clamp(((c1[0] * c2[0]) + (c1[1] * c2[1]) + (c1[2] * c2[2])), -1.0F, 1.0F);
      w = acosf(w);
      if(w <= m_InputValues->MisorientationTolerance || (Constants::k_PiD - w) <= m_InputValues->MisorientationTolerance)
      {
        group = true;
        featureIds[neighborpoint] = gnum;
      }
    }
  }
  return group;
}
