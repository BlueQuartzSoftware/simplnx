#include "MergeTwins.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include <random>

using namespace nx::core;

// -----------------------------------------------------------------------------
MergeTwins::MergeTwins(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MergeTwinsInputValues* inputValues,
                       GroupFeaturesInputValues* groupInputValues)
: GroupFeatures(dataStructure, mesgHandler, shouldCancel, groupInputValues)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
MergeTwins::~MergeTwins() noexcept = default;

// -----------------------------------------------------------------------------
int MergeTwins::getSeed(int32 newFid)
{
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& featureParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName);
  auto& cellFeaturesAttMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->NewCellFeatureAttributeMatrixName);
  auto& active = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->ActiveArrayName);

  auto numfeatures = static_cast<int32>(phases.getNumberOfTuples());

  int32 seed = -1;

  // Precalculate some constants
  int32 totalFMinus1 = numfeatures - 1;

  usize counter = 0;
  std::mt19937_64 generator(m_InputValues->Seed); // Standard mersenne_twister_engine seeded
  std::uniform_real_distribution<> distribution(0, 1);
  auto randFeature = static_cast<int32>(float(distribution(generator)) * float(totalFMinus1));
  while(seed == -1 && counter < numfeatures)
  {
    if(randFeature > totalFMinus1)
    {
      randFeature = randFeature - numfeatures;
    }
    if(featureParentIds[randFeature] == -1)
    {
      seed = randFeature;
    }
    randFeature++;
    counter++;
  }
  if(seed >= 0)
  {
    featureParentIds[seed] = newFid;
    std::vector<usize> tDims(1, newFid + 1);
    cellFeaturesAttMatrix.resizeTuples(tDims); // this will resize the active array as well
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool MergeTwins::determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid)
{
  auto& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  auto& featureParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName);
  auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  auto axisToleranceRad = m_InputValues->AxisTolerance * numbers::pi_v<float32> / 180.0f;

  // float w = 0.0f;
  // float n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;
  bool twin = false;
  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  if(featureParentIds[neighborFeature] == -1 && phases[referenceFeature] > 0 && phases[neighborFeature] > 0)
  {
    uint32 phase1 = crystalStructures[phases[referenceFeature]];

    QuatF q1(avgQuats[referenceFeature * 4], avgQuats[referenceFeature * 4 + 1], avgQuats[referenceFeature * 4 + 2], avgQuats[referenceFeature * 4 + 3]);
    QuatF q2(avgQuats[neighborFeature * 4], avgQuats[neighborFeature * 4 + 1], avgQuats[neighborFeature * 4 + 2], avgQuats[neighborFeature * 4 + 3]);

    uint32 phase2 = crystalStructures[phases[neighborFeature]];
    if(phase1 == phase2 && (phase1 == EbsdLib::CrystalStructure::Cubic_High))
    {
      OrientationD axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
      double w = axisAngle[3];
      w *= (180.0f / numbers::pi);
      double axisdiff111 = acosf(fabs(axisAngle[0]) * 0.57735f + fabs(axisAngle[1]) * 0.57735f + fabs(axisAngle[2]) * 0.57735f);
      double angdiff60 = fabs(w - 60.0f);
      if(axisdiff111 < axisToleranceRad && angdiff60 < m_InputValues->AngleTolerance)
      {
        twin = true;
      }
      if(twin)
      {
        featureParentIds[neighborFeature] = newFid;
        return true;
      }
    }
  }
  return false;
}

// -----------------------------------------------------------------------------
Result<> MergeTwins::operator()()
{
  Result result = {};
  /* Sanity check that each phase is Cubic High (m3m) Laue class. If not then warn the user.
   * There is code later on to ensure that only m3m Laue class is used.
   */
  auto& laueClasses = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  auto& cellParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellParentIdsArrayName);
  cellParentIds.fill(-1);
  auto& featureParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName);
  featureParentIds.fill(-1);
  auto& active = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->ActiveArrayName);
  active.fill(true);

  for(usize i = 1; i < laueClasses.getSize(); i++)
  {
    if(laueClasses[i] != EbsdLib::CrystalStructure::Cubic_High)
    {
      std::string msg = fmt::format("Phase '{}' is NOT m3m crystal symmetry. Data from this phase will not be used in this filter.", i);
      result = MakeWarningVoidResult(-23500, msg);
    }
  }

  featureParentIds[0] = 0; // set feature 0 to be parent 0

  GroupFeatures::execute();

  usize totalFeatures = active.getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return MergeResults(
        result, ConvertResult(MakeErrorResult<OutputActions>(-23501, "The number of grouped Features was 0 or 1 which means no grouped Features were detected. A grouping value may be set too high")));
  }

  int32 numParents = 0;
  usize totalPoints = featureIds.getNumberOfTuples();
  for(usize k = 0; k < totalPoints; k++)
  {
    int32 featureName = featureIds[k];
    cellParentIds[k] = featureParentIds[featureName];
    if(featureParentIds[featureName] > numParents)
    {
      numParents = featureParentIds[featureName];
    }
  }
  numParents += 1;

  if(m_InputValues->RandomizeParentIds)
  {
    m_MessageHandler({IFilter::Message::Type::Info, "Randomizing Parent Ids...."});
    // Generate all the numbers up front
    const int32 rangeMin = 1;
    const int32 rangeMax = numParents - 1;
    std::mt19937_64 generator(std::mt19937_64::default_seed); // Standard mersenne_twister_engine seeded with milliseconds
    std::uniform_int_distribution<int32> distribution(rangeMin, rangeMax);

    auto nParents = static_cast<usize>(numParents);

    DataStructure tmpStructure;
    Int32Array* rndNumbers = Int32Array::CreateWithStore<Int32DataStore>(tmpStructure, std::string("_INTERNAL_USE_ONLY_NewParentIds"), std::vector<usize>{nParents}, std::vector<usize>{1});
    auto* rndStore = dynamic_cast<Int32DataStore*>(rndNumbers->getDataStore());
    int32* pid = rndStore->data();
    pid[0] = 0;
    std::set<int32> parentIdSet;
    parentIdSet.insert(0);
    for(int32 i = 1; i < numParents; ++i) // TODO : easily parallelized
    {
      pid[i] = i; // numberGenerator();
      parentIdSet.insert(rndStore->getValue(i));
    }

    m_MessageHandler({IFilter::Message::Type::Info, "Shuffle elements ...."});
    //--- Shuffle elements by randomly exchanging each with one other.
    for(usize i = 1; i < nParents; i++)
    {
      auto r = static_cast<usize>(distribution(generator)); // Random remaining position.
      int32 pid_i = rndStore->getValue(i);
      if(r >= numParents)
      {
        continue;
      }
      int32 pid_r = rndStore->getValue(r);
      rndStore->setValue(i, pid_r);
      rndStore->setValue(r, pid_i);
    }

    m_MessageHandler({IFilter::Message::Type::Info, "Adjusting Feature Ids Array...."});
    // Now adjust all the Feature Id values for each Voxel
    for(usize i = 0; i < totalPoints; ++i) // TODO : easily parallelized
    {
      cellParentIds[i] = pid[cellParentIds[i]];
      featureParentIds[featureIds[i]] = cellParentIds[i];
    }
  }
  return result;
}
