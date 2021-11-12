#include "ScalarSegmentFeatures.hpp"

#include <chrono>

#include "complex/Common/StringLiteral.hpp"
#include "complex/DataStructure/DataArray.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/DataPathSelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace
{
constexpr StringLiteral k_ScalarToleranceKey = "scalar tolerance";
constexpr StringLiteral k_InputArrayPathKey = "input array path";
constexpr StringLiteral k_UseGoodVoxelsKey = "use mask";
constexpr StringLiteral k_GoodVoxelsPathKey = "mask path";
constexpr StringLiteral k_FeatureIdsPathKey = "feature ids path";
constexpr StringLiteral k_CellFeaturePathKey = "cell feature group path";
constexpr StringLiteral k_ActiveArrayPathKey = "active array path";

/* from http://www.newty.de/fpt/functor.html */
/**
 * @brief The CompareFunctor class serves as a functor superclass for specific implementations
 * of performing scalar comparisons
 */
class CompareFunctor
{
public:
  virtual ~CompareFunctor() = default;

  virtual bool operator()(int64_t index, int64_t neighIndex, int32_t gnum) // call using () operator
  {
    return false;
  }
};

/**
 * @brief The TSpecificCompareFunctorBool class extends @see CompareFunctor to compare boolean data
 */
class TSpecificCompareFunctorBool : public CompareFunctor
{
public:
  TSpecificCompareFunctorBool(void* data, int64_t length, bool tolerance, int32_t* featureIds)
  : m_Length(length)
  , m_FeatureIds(featureIds)
  {
    m_Data = reinterpret_cast<bool*>(data);
  }
  virtual ~TSpecificCompareFunctorBool() = default;

  bool operator()(int64_t referencepoint, int64_t neighborpoint, int32_t gnum) override
  {
    // Sanity check the indices that are being passed in.
    if(referencepoint >= m_Length || neighborpoint >= m_Length)
    {
      return false;
    }

    if(m_Data[neighborpoint] == m_Data[referencepoint])
    {
      m_FeatureIds[neighborpoint] = gnum;
      return true;
    }
    return false;
  }

protected:
  TSpecificCompareFunctorBool() = default;

private:
  bool* m_Data = nullptr;          // The data that is being compared
  int64_t m_Length = 0;            // Length of the Data Array
  int32_t* m_FeatureIds = nullptr; // The Feature Ids
};

/**
 * @brief The TSpecificCompareFunctor class extens @see CompareFunctor to compare templated data
 */
template <class T>
class TSpecificCompareFunctor : public CompareFunctor
{
public:
  TSpecificCompareFunctor(void* data, int64_t length, T tolerance, int32_t* featureIds)
  : m_Length(length)
  , m_Tolerance(tolerance)
  , m_FeatureIds(featureIds)
  {
    m_Data = reinterpret_cast<T*>(data);
  }
  virtual ~TSpecificCompareFunctor() = default;

  bool operator()(int64_t referencepoint, int64_t neighborpoint, int32_t gnum) override
  {
    // Sanity check the indices that are being passed in.
    if(referencepoint >= m_Length || neighborpoint >= m_Length)
    {
      return false;
    }

    if(m_Data[referencepoint] >= m_Data[neighborpoint])
    {
      if((m_Data[referencepoint] - m_Data[neighborpoint]) <= m_Tolerance)
      {
        m_FeatureIds[neighborpoint] = gnum;
        return true;
      }
    }
    else
    {
      if((m_Data[neighborpoint] - m_Data[referencepoint]) <= m_Tolerance)
      {
        m_FeatureIds[neighborpoint] = gnum;
        return true;
      }
    }
    return false;
  }

protected:
  TSpecificCompareFunctor() = default;

private:
  T* m_Data = nullptr;               // The data that is being compared
  int64_t m_Length = 0;              // Length of the Data Array
  T m_Tolerance = static_cast<T>(0); // The tolerance of the comparison
  int32_t* m_FeatureIds = nullptr;   // The Feature Ids
};
}

namespace complex
{
namespace
{
} // namespace

std::string ScalarSegmentFeatures::name() const
{
  return FilterTraits<ScalarSegmentFeatures>::name;
}

std::string ScalarSegmentFeatures::className() const
{
  return FilterTraits<ScalarSegmentFeatures>::className;
}

Uuid ScalarSegmentFeatures::uuid() const
{
  return FilterTraits<ScalarSegmentFeatures>::uuid;
}

std::string ScalarSegmentFeatures::humanName() const
{
  return "Segment Features (Scalar)";
}

Parameters ScalarSegmentFeatures::parameters() const
{
  Parameters params;
  params.insert(std::make_unique<NumberParameter<float>>(k_ScalarToleranceKey, "Scalar Tolerance", "Tolerance for segmenting input Cell Data", 1.0f));
  params.insert(std::make_unique<BoolParameter>(k_UseGoodVoxelsKey, "Use Mask Array", "Determines if a mask array is used for segmenting", false));

  params.insert(std::make_unique<DataPathSelectionParameter>(k_InputArrayPathKey, "Scalar Array to Segment", "Path to the DataArray to segment", DataPath()));
  params.insert(std::make_unique<DataPathSelectionParameter>(k_GoodVoxelsPathKey, "Mask", "Path to the DataArray Mask", DataPath()));

  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsPathKey, "Feature IDs", "Path to the created Feature IDs path", DataPath()));
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeaturePathKey, "Cell Feature Group", "Path to the parent Feature group", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayPathKey, "Active", "Created array", DataPath()));
  return params;
}

IFilter::UniquePointer ScalarSegmentFeatures::clone() const
{
  return std::make_unique<ScalarSegmentFeatures>();
}

IFilter::PreflightResult ScalarSegmentFeatures::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);
  auto inputDataPath = args.value<DataPath>(k_InputArrayPathKey);

  bool useGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);
  if(useGoodVoxels)
  {
    auto goodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
  }

  auto activePath = args.value<DataPath>();

  validateNmberOfTuples(data, dataArrayPaths);

  //
  auto action = std::make_unique<ScalarSegmentFeaturesAction>(dataArrayPath);

  OutputActions actions;
  actions.actions.push_back(std::move(action));

  return {std::move(actions)};
}

Result<> ScalarSegmentFeatures::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  return {};
}

void ScalarSegmentFeatures::randomizeFeatureIds(Int32Array* featureIds, int64_t totalPoints, int64_t totalFeatures, Int64Distribution& distribution)
{
  // notifyStatusMessage("Randomizing Feature Ids");
  // Generate an even distribution of numbers between the min and max range
  const int64_t rangeMin = 1;
  const int64_t rangeMax = totalFeatures - 1;
  auto generator = initializeVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  auto rndNumbers = std::make_shared<Int64Array>(totalFeatures, std::string("_INTERNAL_USE_ONLY_NewFeatureIds"), true);
  auto rndStore = rndNumbers->getDataStore();
  rndStore->setValue(0, 0);

  for(int64_t i = 1; i < totalFeatures; ++i)
  {
    rndStore->setValue(i, i);
  }

  int64_t r = 0;
  int64_t temp = 0;

  //--- Shuffle elements by randomly exchanging each with one other.
  for(int64_t i = 1; i < totalFeatures; i++)
  {
    r = distribution(generator); // Random remaining position.
    if(r >= totalFeatures)
    {
      continue;
    }
    temp = rndStore->getValue(i);
    rndStore->setValue(i, rndStore->getValue(r));
    rndStore->setValue(r, temp);
  }

  // Now adjust all the Grain Id values for each Voxel
  auto featureIdsStore = featureIds->getDataStore();
  for(int64_t i = 0; i < totalPoints; ++i)
  {
    featureIdsStore->setValue(i, rndStore->getValue(featureIdsStore->getValue(i)));
  }
}

int64 ScalarSegmentFeatures::getSeed(Int32Array* featureIdsArray, int32 gnum, int64 nextSeed, bool useGoodVoxels, const UInt8Array* goodVoxelsArray)
{
  auto featureIds = featureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();
  auto goodVoxels = goodVoxelsArray->getDataStore();
  int64 seed = -1;
  // start with the next voxel after the last seed
  usize randpoint = static_cast<size_t>(nextSeed);
  while(seed == -1 && randpoint < totalPoints)
  {
    if(featureIds->getValue(randpoint) == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if(!useGoodVoxels || goodVoxels->getValue(randpoint))
      {
        seed = randpoint;
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
    featureIds->setValue(seed, gnum);
    //std::vector<size_t> tDims(1, gnum + 1);
    //m->getAttributeMatrix(getCellFeatureAttributeMatrixName())->resizeAttributeArrays(tDims);
  }
  return seed;
}

bool ScalarSegmentFeatures::determineGrouping(const Int32Array* featureIdsArray, bool useGoodVoxels, const UInt8Array* goodVoxelsArray, int64 referencepoint, int64 neighborpoint, int32 gnum)
{
  auto feastureIds = featureIdsArray->getDataStore();
  auto goodVoxels = goodVoxelsArray->getDataStore();
  if(feastureIds->getValue(neighborpoint) == 0 && (!useGoodVoxels || goodVoxels->getValue(neighborpoint)))
  {
    CompareFunctor* func = m_Compare.get();
    return (*func)((size_t)(referencepoint), (size_t)(neighborpoint), gnum);
    //     | Functor  ||calling the operator() method of the CompareFunctor Class |
  }

  return false;
}

ScalarSegmentFeatures::SeedGenerator ScalarSegmentFeatures::initializeVoxelSeedGenerator(Int64Distribution& distribution, const int64 rangeMin, const int64 rangeMax)
{
  auto seed = static_cast<SeedGenerator::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  SeedGenerator generator;
  generator.seed(seed);
  auto distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);
  distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);

  return generator;
}

bool ScalarSegmentFeatures::validateNmberOfTuples(const DataStructure& dataStructure, const std::vector<DataPath>& dataArrayPaths)
{

}
} // namespace complex
