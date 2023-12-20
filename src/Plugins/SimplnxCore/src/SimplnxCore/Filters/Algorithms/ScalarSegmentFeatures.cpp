#include "ScalarSegmentFeatures.hpp"

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

#define CX_DEFAULT_CONSTRUCTORS(className)                                                                                                                                                             \
  className(const className&) = delete;                                                                                                                                                                \
  className(className&&) noexcept = delete;                                                                                                                                                            \
  className& operator=(const className&) = delete;                                                                                                                                                     \
  className& operator=(className&&) noexcept = delete;

namespace
{

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
  , m_FeatureIdsArray(featureIds)
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
      m_FeatureIdsArray->setValue(neighborPoint, gnum);
      return true;
    }
    return false;
  }

protected:
  TSpecificCompareFunctorBool() = default;

private:
  int64 m_Length = 0;                                    // Length of the Data Array
  AbstractDataStore<int32>* m_FeatureIdsArray = nullptr; // The Feature Ids
  DataArrayType* m_Data = nullptr;                       // The data that is being compared
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
  using DataStoreType = AbstractDataStore<T>;

  TSpecificCompareFunctor(IDataArray* data, int64 length, T tolerance, AbstractDataStore<int32>* featureIds)
  : m_Length(length)
  , m_Tolerance(tolerance)
  , m_FeatureIdsArray(featureIds)
  , m_Data(dynamic_cast<DataArrayType*>(data)->getDataStoreRef())
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

    if(m_Data[referencePoint] >= m_Data[neighborPoint])
    {
      if((m_Data[referencePoint] - m_Data[neighborPoint]) <= m_Tolerance)
      {
        m_FeatureIdsArray->setValue(neighborPoint, gnum);
        return true;
      }
    }
    else
    {
      if((m_Data[neighborPoint] - m_Data[referencePoint]) <= m_Tolerance)
      {
        m_FeatureIdsArray->setValue(neighborPoint, gnum);
        return true;
      }
    }
    return false;
  }

protected:
  TSpecificCompareFunctor() = default;

private:
  int64 m_Length = 0;                                    // Length of the Data Array
  T m_Tolerance = static_cast<T>(0);                     // The tolerance of the comparison
  AbstractDataStore<int32>* m_FeatureIdsArray = nullptr; // The Feature Ids
  DataStoreType& m_Data;                                 // The data that is being compared
};
} // namespace

ScalarSegmentFeatures::ScalarSegmentFeatures(DataStructure& dataStructure, ScalarSegmentFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel,
                                             const IFilter::MessageHandler& mesgHandler)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
}

ScalarSegmentFeatures::~ScalarSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ScalarSegmentFeatures::operator()()
{
  nx::core::AbstractDataStore<bool>* goodVoxels = nullptr;
  if(m_InputValues->pUseGoodVoxels)
  {
    m_GoodVoxelsArray = m_DataStructure.getDataAs<GoodVoxelsArrayType>(m_InputValues->pGoodVoxelsPath);
    goodVoxels = m_GoodVoxelsArray->getDataStore();
  }

  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->pGridGeomPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->pFeatureIdsPath);
  m_FeatureIdsArray->fill(0); // initialize the output array with zeros
  IDataArray* inputDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->pInputDataPath);
  size_t inDataPoints = inputDataArray->getNumberOfTuples();
  nx::core::DataType dataType = inputDataArray->getDataType();

  auto featureIds = m_FeatureIdsArray->getDataStore();

  switch(dataType)
  {
  case nx::core::DataType::int8: {
    m_CompareFunctor = std::make_shared<::TSpecificCompareFunctor<int8>>(inputDataArray, inDataPoints, static_cast<int8>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint8: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint8>>(inputDataArray, inDataPoints, static_cast<uint8>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::boolean: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctorBool>(inputDataArray, inDataPoints, static_cast<bool>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::int16: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int16>>(inputDataArray, inDataPoints, static_cast<int16>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint16: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint16>>(inputDataArray, inDataPoints, static_cast<uint16>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::int32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int32>>(inputDataArray, inDataPoints, static_cast<int32>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint32>>(inputDataArray, inDataPoints, static_cast<uint32>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::int64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int64>>(inputDataArray, inDataPoints, static_cast<int64>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint64>>(inputDataArray, inDataPoints, static_cast<uint64>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::float32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<float32>>(inputDataArray, inDataPoints, static_cast<float32>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::float64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<float64>>(inputDataArray, inDataPoints, static_cast<float64>(m_InputValues->pScalarTolerance), featureIds);
    break;
  }
  default:
    break;
  }
  if(inputDataArray->getNumberOfComponents() != 1)
  {
    m_CompareFunctor = std::shared_ptr<SegmentFeatures::CompareFunctor>(new SegmentFeatures::CompareFunctor()); // The default CompareFunctor which ALWAYS returns false for the comparison
  }

  // Generate the random voxel indices that will be used for the seed points to start a new grain growth/agglomeration
  auto totalPoints = inputDataArray->getNumberOfTuples();

  //  // Add compare function to arguments
  //  Arguments newArgs = args;
  //  newArgs.insert(k_CompareFunctKey, compare.get());

  execute(gridGeom);

  auto* activeArray = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->pActiveArrayPath);
  auto totalFeatures = activeArray->getNumberOfTuples();
  if(totalFeatures < 2)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-87000, "The number of Features was 0 or 1 which means no Features were detected. A threshold value may be set too high"}})};
  }

  // make sure all values are initialized and "re-reserve" index 0
  activeArray->getDataStore()->fill(1);
  (*activeArray)[0] = 0;

  // Randomize the feature Ids for purely visual clarify. Having random Feature Ids
  // allows users visualizing the data to better discern each grain otherwise the coloring
  // would look like a smooth gradient. This is a user input parameter
  if(m_InputValues->pShouldRandomizeFeatureIds)
  {
    const int64 rangeMin = 0;
    const int64 rangeMax = totalPoints - 1;
    Int64Distribution distribution;
    initializeStaticVoxelSeedGenerator(distribution, rangeMin, rangeMax);
    totalPoints = gridGeom->getNumberOfCells();
    randomizeFeatureIds(m_FeatureIdsArray, totalPoints, totalFeatures, distribution);
  }

  return {};
}

// -----------------------------------------------------------------------------
int64_t ScalarSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  nx::core::AbstractDataStore<bool>* goodVoxels = nullptr;
  if(m_InputValues->pUseGoodVoxels)
  {
    goodVoxels = m_GoodVoxelsArray->getDataStore();
  }

  nx::core::DataArray<int32>::store_type* featureIds = m_FeatureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();

  int64 seed = -1;
  // start with the next voxel after the last seed
  usize randpoint = static_cast<usize>(nextSeed);
  while(seed == -1 && randpoint < totalPoints)
  {
    if(featureIds->getValue(randpoint) == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if(!m_InputValues->pUseGoodVoxels || goodVoxels->getValue(randpoint))
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
    UInt8Array& activeArray = m_DataStructure.getDataRefAs<UInt8Array>(m_InputValues->pActiveArrayPath);
    featureIds->setValue(static_cast<usize>(seed), gnum);
    std::vector<usize> tDims = {static_cast<usize>(gnum) + 1};
    auto& cellFeaturesAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->pCellFeaturesPath);
    cellFeaturesAM.resizeTuples(tDims); // This will resize the active array
  }
  return seed;
}

// -----------------------------------------------------------------------------
bool ScalarSegmentFeatures::determineGrouping(int64 referencepoint, int64 neighborpoint, int32 gnum) const
{

  auto featureIds = m_FeatureIdsArray->getDataStore();

  const nx::core::AbstractDataStore<bool>* goodVoxels = nullptr;
  if(m_InputValues->pUseGoodVoxels)
  {
    goodVoxels = m_GoodVoxelsArray->getDataStore();
  }

  if(featureIds->getValue(neighborpoint) == 0 && (!m_InputValues->pUseGoodVoxels || goodVoxels->getValue(neighborpoint)))
  {
    CompareFunctor* func = m_CompareFunctor.get();
    return (*func)((usize)(referencepoint), (usize)(neighborpoint), gnum);
    //     | Functor  ||calling the operator() method of the CompareFunctor Class |
  }

  return false;
}
