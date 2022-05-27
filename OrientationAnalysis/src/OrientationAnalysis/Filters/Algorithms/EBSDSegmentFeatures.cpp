#include "EBSDSegmentFeatures.hpp"

#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/DataStructure/DataStore.hpp"


#include <chrono>

using namespace complex;

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
  complex::AbstractDataStore<bool>* goodVoxels = nullptr;
  if(m_InputValues->useGoodVoxels)
  {
    m_GoodVoxelsArray = m_DataStructure.getDataAs<GoodVoxelsArrayType>(m_InputValues->goodVoxelsArrayPath);
    goodVoxels = m_GoodVoxelsArray->getDataStore();
  }

  auto gridGeom = m_DataStructure.getDataAs<AbstractGeometryGrid>(m_InputValues->gridGeomPath);


  m_QuatsArray = m_DataStructure.getDataAs<Float32Array>(m_InputValues->quatsArrayPath);
  m_CellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->cellPhasesArrayPath);
  m_GoodVoxelsArray = m_DataStructure.getDataAs<BoolArray>(m_InputValues->goodVoxelsArrayPath);
  m_CrystalStructures = m_DataStructure.getDataAs<Int32Array>(m_InputValues->crystalStructuresArrayPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->featureIdsArrayPath);
  m_FeatureIdsArray->fill(0); // initialize the output array with zeros

  // Generate the random voxel indices that will be used for the seed points to start a new grain growth/agglomeration
  auto totalPoints = m_QuatsArray->getNumberOfTuples();

  const int64 rangeMin = 0;
  const int64 rangeMax = static_cast<int64>(totalPoints - 1);
  Int64Distribution distribution;
  initializeVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  execute(gridGeom);

  IDataArray* activeArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->activeArrayPath);
  auto totalFeatures = activeArray->getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-87000, "The number of Features was 0 or 1 which means no Features were detected. A threshold value may be set too high"}})};
  }

  // By default we randomize grains
  if(m_InputValues->shouldRandomizeFeatureIds)
  {
    totalPoints = gridGeom->getNumberOfElements();
    randomizeFeatureIds(m_FeatureIdsArray, totalPoints, totalFeatures, distribution);
  }

  return {};
}

// -----------------------------------------------------------------------------
int64_t EBSDSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  complex::AbstractDataStore<bool>* goodVoxels = nullptr;
  if(m_InputValues->useGoodVoxels)
  {
    goodVoxels = m_GoodVoxelsArray->getDataStore();
  }

  complex::DataArray<int32>::store_type* featureIds = m_FeatureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();

  complex::AbstractDataStore<int32>* cellPhases = m_CellPhases->getDataStore();

  int64 seed = -1;
  // start with the next voxel after the last seed
  usize randpoint = static_cast<usize>(nextSeed);
  while(seed == -1 && randpoint < totalPoints)
  {
    if(featureIds->getValue(randpoint) == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if((!m_InputValues->useGoodVoxels || goodVoxels->getValue(randpoint)) && cellPhases->getValue(randpoint) > 0)
      {
        seed = static_cast<int64>(randpoint);
      }
      else
      {
        randpoint += 1;
      }
    }
    else
    {
      randpoint += 1;
    }
  }
  if(seed >= 0)
  {
    UInt8Array& activeArray = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->activeArrayPath);
    featureIds->setValue(static_cast<usize>(seed), gnum);
    std::vector<usize> tDims = {static_cast<usize>(gnum) + 1};
    activeArray.getDataStore()->reshapeTuples(tDims); // This will resize the actives array
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool EBSDSegmentFeatures::determineGrouping(int64 referencepoint, int64 neighborpoint, int32 gnum) const
{
  bool group = false;

  // Get the phases for each voxel
  complex::AbstractDataStore<int32>* cellPhases = m_CellPhases->getDataStore();

  int32_t phase1 = (*m_CrystalStructures)[(*cellPhases)[referencepoint]];
  int32_t phase2 = (*m_CrystalStructures)[(*cellPhases)[neighborpoint]];
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
    BoolArray& useGoodVoxels = *m_GoodVoxelsArray;
    neighborPointIsGood = useGoodVoxels[neighborpoint];
  }

  if(featureIds[neighborpoint] == 0 && (m_GoodVoxelsArray == nullptr || neighborPointIsGood))
  {
    float w = std::numeric_limits<float>::max();
    QuatF q1(currentQuatPtr[referencepoint * 4], currentQuatPtr[referencepoint * 4 + 1], currentQuatPtr[referencepoint * 4 + 2], currentQuatPtr[referencepoint * 4 + 3]);
    QuatF q2(currentQuatPtr[neighborpoint * 4 + 0], currentQuatPtr[neighborpoint * 4 + 1], currentQuatPtr[neighborpoint * 4 + 2], currentQuatPtr[neighborpoint * 4 + 3]);

    if((*cellPhases)[referencepoint] == (*cellPhases)[neighborpoint])
    {
      OrientationF axisAngle = m_OrientationOps[phase1]->calculateMisorientation(q1, q2);
      w = axisAngle[3];
    }
    if(w < m_InputValues->misorientationTolerance)
    {
      group = true;
      featureIds[neighborpoint] = gnum;
    }
  }

  return group;
}
