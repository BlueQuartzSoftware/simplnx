#include "MergeTwins.hpp"

#include "complex/Common/Numbers.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataGroup.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/Core/EbsdLibConstants.h"
#include "EbsdLib/Core/Orientation.hpp"
#include "EbsdLib/Core/Quaternion.hpp"
#include "EbsdLib/LaueOps/LaueOps.h"

#include <random>

using namespace complex;

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
void MergeTwins::characterize_twins()
{
}

// -----------------------------------------------------------------------------
int MergeTwins::getSeed(int32 newFid)
{
  Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  Int32Array& featureParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName);
  AttributeMatrix& cellFeaturesAttMatrix = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->NewCellFeatureAttributeMatrixName);
  BoolArray& active = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->ActiveArrayName);

  int32 numfeatures = static_cast<int32>(phases.getNumberOfTuples());

  int32 seed = -1;
  int32 randfeature = 0;

  // Precalculate some constants
  int32 totalFMinus1 = numfeatures - 1;

  size_t counter = 0;
  double rangeMin = 0;
  double rangeMax = 1;
  std::random_device randomDevice;           // Will be used to obtain a seed for the random number engine
  std::mt19937_64 generator(randomDevice()); // Standard mersenne_twister_engine seeded with rd()
  generator.seed(m_InputValues->Seed);
  std::uniform_real_distribution<> distribution(rangeMin, rangeMax);
  randfeature = int32(float(distribution(generator)) * float(totalFMinus1));
  while(seed == -1 && counter < numfeatures)
  {
    if(randfeature > totalFMinus1)
    {
      randfeature = randfeature - numfeatures;
    }
    if(featureParentIds[randfeature] == -1)
    {
      seed = randfeature;
    }
    randfeature++;
    counter++;
  }
  if(seed >= 0)
  {
    featureParentIds[seed] = newFid;
    std::vector<size_t> tDims(1, newFid + 1);
    cellFeaturesAttMatrix.resizeTuples(tDims); // this will resize the active array as well
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool MergeTwins::determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid)
{
  Int32Array& phases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeaturePhasesArrayPath);
  Int32Array& featureParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName);
  UInt32Array& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  Float32Array& avgQuats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);
  auto axisToleranceRad = m_InputValues->AxisTolerance * numbers::pi_v<float32> / 180.0f;

  // float w = 0.0f;
  // float n1 = 0.0f, n2 = 0.0f, n3 = 0.0f;
  bool twin = false;
  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  if(featureParentIds[neighborFeature] == -1 && phases[referenceFeature] > 0 && phases[neighborFeature] > 0)
  {
    uint32_t phase1 = crystalStructures[phases[referenceFeature]];

    QuatF q1(avgQuats[referenceFeature * 4], avgQuats[referenceFeature * 4 + 1], avgQuats[referenceFeature * 4 + 2], avgQuats[referenceFeature * 4 + 3]);
    QuatF q2(avgQuats[neighborFeature * 4], avgQuats[neighborFeature * 4 + 1], avgQuats[neighborFeature * 4 + 2], avgQuats[neighborFeature * 4 + 3]);

    uint32_t phase2 = crystalStructures[phases[neighborFeature]];
    if(phase1 == phase2 && (phase1 == EbsdLib::CrystalStructure::Cubic_High))
    {
      OrientationD axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
      double w = axisAngle[3];
      w = w * (180.0f / numbers::pi);
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
  UInt32Array& laueClasses = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  Int32Array& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  Int32Array& cellParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellParentIdsArrayName);
  cellParentIds.fill(-1);
  Int32Array& featureParentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureParentIdsArrayName);
  featureParentIds.fill(-1);
  BoolArray& active = m_DataStructure.getDataRefAs<BoolArray>(m_InputValues->ActiveArrayName);
  active.fill(true);

  for(size_t i = 1; i < laueClasses.getSize(); i++)
  {
    if(laueClasses[i] != EbsdLib::CrystalStructure::Cubic_High)
    {
      std::string msg = fmt::format("Phase '{}' is NOT m3m crystal symmetry. Data from this phase will not be used in this filter.", i);
      result = MakeWarningVoidResult(-23500, msg);
    }
  }

  featureParentIds[0] = 0; // set feature 0 to be parent 0

  GroupFeatures::execute();

  size_t totalFeatures = active.getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return MergeResults(
        result, ConvertResult(MakeErrorResult<OutputActions>(-23501, "The number of grouped Features was 0 or 1 which means no grouped Features were detected. A grouping value may be set too high")));
  }

  int32 numParents = 0;
  size_t totalPoints = featureIds.getNumberOfTuples();
  for(size_t k = 0; k < totalPoints; k++)
  {
    int32_t featurename = featureIds[k];
    cellParentIds[k] = featureParentIds[featurename];
    if(featureParentIds[featurename] > numParents)
    {
      numParents = featureParentIds[featurename];
    }
  }
  numParents += 1;

  m_MessageHandler({IFilter::Message::Type::Info, "Characterizing Twins Starting"});
  characterize_twins();
  m_MessageHandler({IFilter::Message::Type::Info, "Characterizing Twins Complete"});

  if(m_InputValues->RandomizeParentIds)
  {
    m_MessageHandler({IFilter::Message::Type::Info, "Randomizing Parent Ids...."});
    // Generate all the numbers up front
    const int32 rangeMin = 1;
    const int32 rangeMax = numParents - 1;
    std::mt19937_64 generator(std::mt19937_64::default_seed); // Standard mersenne_twister_engine seeded with milliseconds
    std::uniform_int_distribution<int32_t> distribution(rangeMin, rangeMax);

    size_t nParents = static_cast<size_t>(numParents);

    DataStructure tmpStructure;
    Int32Array* rndNumbers = Int32Array::CreateWithStore<Int32DataStore>(tmpStructure, std::string("_INTERNAL_USE_ONLY_NewParentIds"), std::vector<usize>{nParents}, std::vector<usize>{1});
    Int32DataStore* rndStore = dynamic_cast<Int32DataStore*>(rndNumbers->getDataStore());
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
    for(size_t i = 1; i < nParents; i++)
    {
      size_t r = static_cast<size_t>(distribution(generator)); // Random remaining position.
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
    for(size_t i = 0; i < totalPoints; ++i) // TODO : easily parallelized
    {
      cellParentIds[i] = pid[cellParentIds[i]];
      featureParentIds[featureIds[i]] = cellParentIds[i];
    }
  }
  return result;
}
