#include "ScalarSegmentFeatures.hpp"

#include <memory>

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"

using namespace nx::core;

#define CX_DEFAULT_CONSTRUCTORS(className)                                                                                                                                                             \
  className(const className&) = delete;                                                                                                                                                                \
  className(className&&) noexcept = delete;                                                                                                                                                            \
  className& operator=(const className&) = delete;                                                                                                                                                     \
  className& operator=(className&&) noexcept = delete;

namespace
{
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

  TSpecificCompareFunctorBool(IDataArray* data, int64 length, AbstractDataStore<int32>* featureIds)
  : m_Length(length)
  , m_FeatureIdsArray(featureIds)
  , m_Data(dynamic_cast<DataArrayType*>(data))
  {
  }
  TSpecificCompareFunctorBool() = default;
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
  CX_DEFAULT_CONSTRUCTORS(TSpecificCompareFunctor)

  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  TSpecificCompareFunctor(IDataArray* data, int64 length, T tolerance, AbstractDataStore<int32>* featureIds)
  : m_Length(length)
  , m_Tolerance(tolerance)
  , m_FeatureIdsArray(featureIds)
  , m_Data(data->getIDataStoreRefAs<DataStoreType>())
  {
  }
  TSpecificCompareFunctor() = default;
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
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxels = InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
    } catch(const std::out_of_range& exception)
    {
      // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
      // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-54110, message);
    }
  }

  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  m_FeatureIdsArray->fill(0); // initialize the output array with zeros

  auto* inputDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputDataPath);
  size_t inDataPoints = inputDataArray->getNumberOfTuples();
  nx::core::DataType dataType = inputDataArray->getDataType();

  auto featureIds = m_FeatureIdsArray->getDataStore();

  switch(dataType)
  {
  case nx::core::DataType::int8: {
    m_CompareFunctor = std::make_shared<::TSpecificCompareFunctor<int8>>(inputDataArray, inDataPoints, static_cast<int8>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint8: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint8>>(inputDataArray, inDataPoints, static_cast<uint8>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::boolean: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctorBool>(inputDataArray, inDataPoints, featureIds);
    break;
  }
  case nx::core::DataType::int16: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int16>>(inputDataArray, inDataPoints, static_cast<int16>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint16: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint16>>(inputDataArray, inDataPoints, static_cast<uint16>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::int32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int32>>(inputDataArray, inDataPoints, static_cast<int32>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint32>>(inputDataArray, inDataPoints, static_cast<uint32>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::int64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int64>>(inputDataArray, inDataPoints, static_cast<int64>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::uint64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint64>>(inputDataArray, inDataPoints, static_cast<uint64>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::float32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<float32>>(inputDataArray, inDataPoints, static_cast<float32>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  case nx::core::DataType::float64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<float64>>(inputDataArray, inDataPoints, static_cast<float64>(m_InputValues->ScalarTolerance), featureIds);
    break;
  }
  default:
    break;
  }
  if(inputDataArray->getNumberOfComponents() != 1)
  {
    m_CompareFunctor = std::make_shared<SegmentFeatures::CompareFunctor>(); // The default CompareFunctor which ALWAYS returns false for the comparison
  }

  // Run the segmentation algorithm
  execute(gridGeom);
  // Sanity check the result.
  if(this->m_FoundFeatures < 1)
  {
    return {MakeErrorResult(-87000, fmt::format("The number of Features is '{}' which means no Features were detected. A threshold value may be set incorrectly", this->m_FoundFeatures))};
  }

  // Resize the Feature Attribute Matrix
  std::vector<usize> tDims = {static_cast<usize>(this->m_FoundFeatures + 1)};
  auto& cellFeaturesAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
  cellFeaturesAM.resizeTuples(tDims); // This will resize the active array

  // make sure all values are initialized and "re-reserve" index 0
  auto& activeStore = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath)->getDataStoreRef();
  activeStore.fill(1);
  activeStore[0] = 0;

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
int64_t ScalarSegmentFeatures::getSeed(int32 gnum, int64 nextSeed) const
{
  nx::core::DataArray<int32>::store_type* featureIds = m_FeatureIdsArray->getDataStore();
  usize totalPoints = featureIds->getNumberOfTuples();

  int64 seed = -1;
  // start with the next voxel after the last seed
  auto randPoint = static_cast<usize>(nextSeed);
  while(seed == -1 && randPoint < totalPoints)
  {
    if(featureIds->getValue(randPoint) == 0) // If the GrainId of the voxel is ZERO then we can use this as a seed point
    {
      if(!m_InputValues->UseMask || m_GoodVoxels->isTrue(randPoint))
      {
        seed = randPoint;
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
bool ScalarSegmentFeatures::determineGrouping(int64 referencepoint, int64 neighborpoint, int32 gnum) const
{
  auto* featureIds = m_FeatureIdsArray->getDataStore();
  if(featureIds->getValue(neighborpoint) == 0 && (!m_InputValues->UseMask || m_GoodVoxels->isTrue(neighborpoint)))
  {
    CompareFunctor* func = m_CompareFunctor.get();
    return (*func)((usize)(referencepoint), (usize)(neighborpoint), gnum);
  }

  return false;
}
