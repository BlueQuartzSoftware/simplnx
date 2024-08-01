#include "EBSDSegmentFeatures.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"

#include <chrono>

using namespace nx::core;

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::EBSDSegmentFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, EBSDSegmentFeaturesInputValues* inputValues)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
  m_OrientationOps = LaueOps::GetAllOrientationOps();
}

// -----------------------------------------------------------------------------
EBSDSegmentFeatures::~EBSDSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> EBSDSegmentFeatures::operator()()
{
  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->QuatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxelsArray = std::move(InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath));
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

  // Run the segmentation algorithm
  execute(gridGeom);
  // Sanity check the result.
  if(this->m_FoundFeatures < 1)
  {
    return {MakeErrorResult(-87000, fmt::format("The number of Features is '{}' which means no Features were detected. A threshold value may be set incorrectly", this->m_FoundFeatures))};
  }

  // Resize the Feature Attribute Matrix
  std::vector<usize> tDims = {static_cast<usize>(this->m_FoundFeatures)};
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

  int32_t phase1 = (*m_CrystalStructures)[(*cellPhases)[referencePoint]];
  int32_t phase2 = (*m_CrystalStructures)[(*cellPhases)[neighborPoint]];
  // If either of the phases is 999 then we bail out now.
  if(phase1 >= m_OrientationOps.size() || phase2 >= m_OrientationOps.size())
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
    QuatF q1(currentQuatPtr[referencePoint * 4], currentQuatPtr[referencePoint * 4 + 1], currentQuatPtr[referencePoint * 4 + 2], currentQuatPtr[referencePoint * 4 + 3]);
    QuatF q2(currentQuatPtr[neighborPoint * 4 + 0], currentQuatPtr[neighborPoint * 4 + 1], currentQuatPtr[neighborPoint * 4 + 2], currentQuatPtr[neighborPoint * 4 + 3]);

    if((*cellPhases)[referencePoint] == (*cellPhases)[neighborPoint])
    {
      OrientationF axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
      w = axisAngle[3];
    }
    if(w < m_InputValues->MisorientationTolerance)
    {
      group = true;
      featureIds[neighborPoint] = gnum;
    }
  }

  return group;
}
