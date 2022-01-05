#include "ScalarSegmentFeatures.hpp"

#include <chrono>

#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/AbstractGeometryGrid.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

#define CX_DEFAULT_CONSTRUCTORS(className)                                                                                                                                                             \
  className(const className&) = delete;                                                                                                                                                                \
  className(className&&) noexcept = delete;                                                                                                                                                            \
  className& operator=(const className&) = delete;                                                                                                                                                     \
  className& operator=(className&&) noexcept = delete;

namespace
{
using FeatureIdsArrayType = Int32Array;
using GoodVoxelsArrayType = UInt8Array;

constexpr StringLiteral k_CompareFunctKey = "Compare Function";

constexpr int64 k_IncorrectInputArray = -600;
constexpr int64 k_MissingInputArray = -601;
constexpr int64 k_MissingOrIncorrectGoodVoxelsArray = -602;

/**
 * @brief The TSpecificCompareFunctorBool class extends @see CompareFunctor to compare boolean data
 */
class TSpecificCompareFunctorBool : public SegmentFeatures::CompareFunctor
{
public:
  using DataArrayType = BoolArray;
  CX_DEFAULT_CONSTRUCTORS(TSpecificCompareFunctorBool)

  TSpecificCompareFunctorBool(IDataArray* data, int64 length, bool tolerance, AbstractDataStore<int32>* featureIds)
  : m_Length(length)
  , featureIdsArray(featureIds)
  , m_Data(dynamic_cast<DataArrayType*>(data))
  {
  }
  ~TSpecificCompareFunctorBool() override = default;

  bool operator()(int64 referencePoint, int64 neighborPoint, int32 gnum) override
  {
    // Sanity check the indices that are being passed in.
    if(referencePoint >= m_Length || neighborPoint >= m_Length)
    {
      return false;
    }

    if((*m_Data)[neighborPoint] == (*m_Data)[referencePoint])
    {
      featureIdsArray->setValue(neighborPoint, gnum);
      return true;
    }
    return false;
  }

protected:
  TSpecificCompareFunctorBool() = default;

private:
  int64 m_Length = 0;                                  // Length of the Data Array
  AbstractDataStore<int32>* featureIdsArray = nullptr; // The Feature Ids
  DataArrayType* m_Data = nullptr;                     // The data that is being compared
};

/**
 * @brief The TSpecificCompareFunctor class extens @see CompareFunctor to compare templated data
 */
template <class T>
class TSpecificCompareFunctor : public SegmentFeatures::CompareFunctor
{
public:
  CX_DEFAULT_CONSTRUCTORS(TSpecificCompareFunctor<T>)

  using DataArrayType = DataArray<T>;
  TSpecificCompareFunctor(IDataArray* data, int64 length, T tolerance, AbstractDataStore<int32>* featureIds)
  : m_Length(length)
  , m_Tolerance(tolerance)
  , featureIdsArray(featureIds)
  , m_Data(dynamic_cast<DataArrayType*>(data))
  {
  }
  ~TSpecificCompareFunctor() override = default;

  bool operator()(int64 referencePoint, int64 neighborPoint, int32 gnum) override
  {
    // Sanity check the indices that are being passed in.
    if(referencePoint >= m_Length || neighborPoint >= m_Length)
    {
      return false;
    }

    if((*m_Data)[referencePoint] >= (*m_Data)[neighborPoint])
    {
      if(((*m_Data)[referencePoint] - (*m_Data)[neighborPoint]) <= m_Tolerance)
      {
        featureIdsArray->setValue(neighborPoint, gnum);
        return true;
      }
    }
    else
    {
      if(((*m_Data)[neighborPoint] - (*m_Data)[referencePoint]) <= m_Tolerance)
      {
        featureIdsArray->setValue(neighborPoint, gnum);
        return true;
      }
    }
    return false;
  }

protected:
  TSpecificCompareFunctor() = default;

private:
  int64 m_Length = 0;                                  // Length of the Data Array
  T m_Tolerance = static_cast<T>(0);                   // The tolerance of the comparison
  AbstractDataStore<int32>* featureIdsArray = nullptr; // The Feature Ids
  DataArrayType* m_Data = nullptr;                     // The data that is being compared
};
} // namespace

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

//------------------------------------------------------------------------------
std::vector<std::string> ScalarSegmentFeatures::defaultTags() const
{
  return {"#Reconstruction", "#Segmentation"};
}

Parameters ScalarSegmentFeatures::parameters() const
{
  Parameters params = SegmentFeatures::parameters();

  params.insert(std::make_unique<NumberParameter<int>>(k_ScalarToleranceKey, "Scalar Tolerance", "Tolerance for segmenting input Cell Data", 1));
  params.insert(std::make_unique<BoolParameter>(k_UseGoodVoxelsKey, "Use Mask Array", "Determines if a mask array is used for segmenting", false));

  params.insert(std::make_unique<ArraySelectionParameter>(k_InputArrayPathKey, "Scalar Array to Segment", "Path to the DataArray to segment", DataPath()));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsPathKey, "Mask", "Path to the DataArray Mask", DataPath(), true));

  params.insert(std::make_unique<ArrayCreationParameter>(k_FeatureIdsPathKey, "Feature IDs", "Path to the created Feature IDs path", DataPath()));

  // params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellFeaturePathKey, "Cell Feature Group", "Path to the parent Feature group", DataPath()));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ActiveArrayPathKey, "Active", "Created array", DataPath()));

  params.insert(std::make_unique<BoolParameter>(k_RandomizeFeaturesKey, "Randomize Feature IDs", "Specifies if feature IDs should be randomized during calculations", false));
  return params;
}

IFilter::UniquePointer ScalarSegmentFeatures::clone() const
{
  return std::make_unique<ScalarSegmentFeatures>();
}

IFilter::PreflightResult ScalarSegmentFeatures::preflightImpl(const DataStructure& data, const Arguments& args, const MessageHandler& messageHandler) const
{
  SegmentFeatures::preflightImpl(data, args, messageHandler);
  auto inputDataPath = args.value<DataPath>(k_InputArrayPathKey);
  int scalarTolerance = args.value<int>(k_ScalarToleranceKey);
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);
  // auto cellFeaturesPath = args.value<DataPath>(k_CellFeaturePathKey);
  auto activeArrayPath = args.value<DataPath>(k_ActiveArrayPathKey);

  bool useGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);
  DataPath goodVoxelsPath;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
  }

  // auto activePath = args.value<DataPath>();

  std::vector<DataPath> dataPaths;

  usize numTuples = 0;
  // Input Array
  if(auto inputDataArray = data.getDataAs<IDataArray>(inputDataPath))
  {
    numTuples = inputDataArray->getNumberOfTuples();
    if(inputDataArray->getNumberOfComponents() == 1)
    {
      dataPaths.push_back(inputDataPath);
    }
    else
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Input Array must be a scalar array"}})};
    }
  }
  else
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingInputArray, "Input Array must be specified"}})};
  }

  // Good Voxels
  if(useGoodVoxels)
  {
    const complex::IDataArray* goodVoxelsArray = data.getDataAs<IDataArray>(goodVoxelsPath);
    if(nullptr == goodVoxelsArray)
    {
      return {nonstd::make_unexpected(std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array is not located at path: '{}'", goodVoxelsPath.toString())}})};
    }
    const GoodVoxelsArrayType* maskArray = data.getDataAs<GoodVoxelsArrayType>(goodVoxelsPath);
    if(maskArray == nullptr)
    {
      return {nonstd::make_unexpected(
          std::vector<Error>{Error{k_MissingOrIncorrectGoodVoxelsArray, fmt::format("Mask array at path '{}' is not of the correct type. It must be UInt8.", goodVoxelsPath.toString())}})};
    }
    dataPaths.push_back(goodVoxelsPath);
  }

  data.validateNumberOfTuples(dataPaths);

  // Create Active array
  auto createActiveAction = std::make_unique<CreateArrayAction>(NumericType::uint8, std::vector<usize>{numTuples}, std::vector<usize>{1}, activeArrayPath);
  auto createFeatureIdsAction = std::make_unique<CreateArrayAction>(NumericType::int32, std::vector<usize>{numTuples}, std::vector<usize>{1}, featureIdsPath);

  OutputActions actions;
  actions.actions.push_back(std::move(createActiveAction));
  actions.actions.push_back(std::move(createFeatureIdsAction));

  return {std::move(actions)};
}

Result<> ScalarSegmentFeatures::executeImpl(DataStructure& data, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto inputDataPath = args.value<DataPath>(k_InputArrayPathKey);
  int scalarTolerance = args.value<int>(k_ScalarToleranceKey);
  auto shouldRandomizeFeatureIds = args.value<bool>(k_RandomizeFeaturesKey);
  auto activeArrayPath = args.value<DataPath>(k_ActiveArrayPathKey);
  auto activeArray = data.getDataAs<IDataArray>(activeArrayPath);

  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);

  bool useGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);
  DataPath goodVoxelsPath;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
  }

  auto gridGeomPath = args.value<DataPath>(k_GridGeomPath_Key);
  auto gridGeom = data.getDataAs<AbstractGeometryGrid>(gridGeomPath);

  // updateFeatureInstancePointers();
  auto featureIdsArray = data.getDataAs<Int32Array>(featureIdsPath);
  featureIdsArray->fill(0); // initialize the output array with zeros
  auto inputDataArray = data.getDataAs<IDataArray>(inputDataPath);
  auto inDataPoints = inputDataArray->getNumberOfTuples();
  complex::DataType dataType = inputDataArray->getDataType();

  auto featureIds = featureIdsArray->getDataStore();

  std::shared_ptr<CompareFunctor> compare = nullptr;

  switch(dataType)
  {
  case complex::DataType::int8: {
    compare = std::make_shared<TSpecificCompareFunctor<int8>>(inputDataArray, inDataPoints, static_cast<int8>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::uint8: {
    compare = std::make_shared<TSpecificCompareFunctor<uint8>>(inputDataArray, inDataPoints, static_cast<uint8>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::boolean: {
    compare = std::make_shared<TSpecificCompareFunctorBool>(inputDataArray, inDataPoints, static_cast<bool>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::int16: {
    compare = std::make_shared<TSpecificCompareFunctor<int16>>(inputDataArray, inDataPoints, static_cast<int16>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::uint16: {
    compare = std::make_shared<TSpecificCompareFunctor<uint16>>(inputDataArray, inDataPoints, static_cast<uint16>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::int32: {
    compare = std::make_shared<TSpecificCompareFunctor<int32>>(inputDataArray, inDataPoints, static_cast<int32>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::uint32: {
    compare = std::make_shared<TSpecificCompareFunctor<uint32>>(inputDataArray, inDataPoints, static_cast<uint32>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::int64: {
    compare = std::make_shared<TSpecificCompareFunctor<int64>>(inputDataArray, inDataPoints, static_cast<int64>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::uint64: {
    compare = std::make_shared<TSpecificCompareFunctor<uint64>>(inputDataArray, inDataPoints, static_cast<uint64>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::float32: {
    compare = std::make_shared<TSpecificCompareFunctor<float32>>(inputDataArray, inDataPoints, static_cast<float32>(scalarTolerance), featureIds);
    break;
  }
  case complex::DataType::float64: {
    compare = std::make_shared<TSpecificCompareFunctor<float64>>(inputDataArray, inDataPoints, static_cast<float64>(scalarTolerance), featureIds);
    break;
  }
  default:
    break;
  }
  if(inputDataArray->getNumberOfComponents() != 1)
  {
    compare = std::shared_ptr<CompareFunctor>(new CompareFunctor()); // The default CompareFunctor which ALWAYS returns false for the comparison
  }

  // Generate the random voxel indices that will be used for the seed points to start a new grain growth/agglomeration
  auto totalPoints = inputDataArray->getNumberOfTuples();

  const int64 rangeMin = 0;
  const int64 rangeMax = totalPoints - 1;
  Int64Distribution distribution;
  initializeVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  // Add compare function to arguments
  Arguments newArgs = args;
  newArgs.insert(k_CompareFunctKey, compare.get());

  SegmentFeatures::executeImpl(data, newArgs, pipelineNode, messageHandler);

  // auto activeArrayPath = args.value<DataPath>(k_ActiveArrayPathKey);

  auto totalFeatures = activeArray->getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-87000, "The number of Features was 0 or 1 which means no Features were detected. A threshold value may be set too high"}})};
  }

  // By default we randomize grains
  if(shouldRandomizeFeatureIds)
  {
    totalPoints = gridGeom->getNumberOfElements();
    randomizeFeatureIds(featureIdsArray, totalPoints, totalFeatures, distribution);
  }

  return {};
}

void ScalarSegmentFeatures::randomizeFeatureIds(Int32Array* featureIds, uint64 totalPoints, uint64 totalFeatures, Int64Distribution& distribution) const
{
  // notifyStatusMessage("Randomizing Feature Ids");
  // Generate an even distribution of numbers between the min and max range
  const int64 rangeMin = 1;
  const int64 rangeMax = totalFeatures - 1;
  auto generator = initializeVoxelSeedGenerator(distribution, rangeMin, rangeMax);

  DataStructure tmpStructure;
  auto rndNumbers = Int64Array::CreateWithStore<DataStore<int64>>(tmpStructure, std::string("_INTERNAL_USE_ONLY_NewFeatureIds"), std::vector<usize>{totalFeatures}, std::vector<usize>{1});
  auto rndStore = rndNumbers->getDataStore();

  for(int64 i = 0; i < totalFeatures; ++i)
  {
    rndStore->setValue(i, i);
  }

  int64 r = 0;
  int64 temp = 0;

  //--- Shuffle elements by randomly exchanging each with one other.
  for(int64 i = 1; i < totalFeatures; i++)
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
  for(int64 i = 0; i < totalPoints; ++i)
  {
    featureIdsStore->setValue(i, rndStore->getValue(featureIdsStore->getValue(i)));
  }
}

int64 ScalarSegmentFeatures::getSeed(DataStructure& data, const Arguments& args, int32 gnum, int64 nextSeed) const
{
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);
  auto useGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);

  DataPath goodVoxelsPath;
  GoodVoxelsArrayType* goodVoxelsArray = nullptr;
  complex::AbstractDataStore<uint8_t>* goodVoxels = nullptr;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
    goodVoxelsArray = data.getDataAs<GoodVoxelsArrayType>(goodVoxelsPath);
    goodVoxels = goodVoxelsArray->getDataStore();
  }

  auto featureIdsArray = data.getDataAs<FeatureIdsArrayType>(featureIdsPath);

  auto featureIds = featureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();

  int64 seed = -1;
  // start with the next voxel after the last seed
  usize randpoint = static_cast<usize>(nextSeed);
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
    auto activeArrayPath = args.value<DataPath>(k_ActiveArrayPathKey);
    UInt8Array& activeArray = data.getDataRefAs<UInt8Array>(activeArrayPath);
    featureIds->setValue(static_cast<usize>(seed), gnum);
    std::vector<usize> tDims = {static_cast<usize>(gnum) + 1};
    activeArray.getDataStore()->reshapeTuples(tDims); // This will resize the actives array
  }
  return seed;
}

bool ScalarSegmentFeatures::determineGrouping(const DataStructure& data, const Arguments& args, int64 referencepoint, int64 neighborpoint, int32 gnum) const
{
  auto featureIdsPath = args.value<DataPath>(k_FeatureIdsPathKey);

  auto featureIdsArray = data.getDataAs<FeatureIdsArrayType>(featureIdsPath);

  auto featureIds = featureIdsArray->getDataStore();

  auto useGoodVoxels = args.value<bool>(k_UseGoodVoxelsKey);
  DataPath goodVoxelsPath;
  const GoodVoxelsArrayType* goodVoxelsArray = nullptr;
  const complex::AbstractDataStore<uint8_t>* goodVoxels = nullptr;
  if(useGoodVoxels)
  {
    goodVoxelsPath = args.value<DataPath>(k_GoodVoxelsPathKey);
    goodVoxelsArray = data.getDataAs<GoodVoxelsArrayType>(goodVoxelsPath);
    goodVoxels = goodVoxelsArray->getDataStore();
  }

  if(featureIds->getValue(neighborpoint) == 0 && (!useGoodVoxels || goodVoxels->getValue(neighborpoint)))
  {
    auto compare = args.value<CompareFunctor*>(k_CompareFunctKey);
    CompareFunctor* func = compare;
    return (*func)((usize)(referencepoint), (usize)(neighborpoint), gnum);
    //     | Functor  ||calling the operator() method of the CompareFunctor Class |
  }

  return false;
}

ScalarSegmentFeatures::SeedGenerator ScalarSegmentFeatures::initializeVoxelSeedGenerator(Int64Distribution& distribution, const int64 rangeMin, const int64 rangeMax) const
{
  auto seed = static_cast<SeedGenerator::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  SeedGenerator generator;
  generator.seed(seed);
  distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);
  distribution = std::uniform_int_distribution<int64>(rangeMin, rangeMax);

  return generator;
}
} // namespace complex
