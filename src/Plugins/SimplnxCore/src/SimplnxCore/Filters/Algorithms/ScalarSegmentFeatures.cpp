#include "ScalarSegmentFeatures.hpp"

#include <algorithm>
#include <memory>

#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/IGridGeometry.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

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

  bool compare(int64 index, int64 neighIndex) override
  {
    if(index >= m_Length || neighIndex >= m_Length)
    {
      return false;
    }
    return (*m_Data)[neighIndex] == (*m_Data)[index];
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
  , m_Data(data->template getIDataStoreRefAs<DataStoreType>())
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

  bool compare(int64 index, int64 neighIndex) override
  {
    if(index >= m_Length || neighIndex >= m_Length)
    {
      return false;
    }

    if(m_Data[index] >= m_Data[neighIndex])
    {
      return (m_Data[index] - m_Data[neighIndex]) <= m_Tolerance;
    }
    return (m_Data[neighIndex] - m_Data[index]) <= m_Tolerance;
  }

private:
  int64 m_Length = 0;                                    // Length of the Data Array
  T m_Tolerance = static_cast<T>(0);                     // The tolerance of the comparison
  AbstractDataStore<int32>* m_FeatureIdsArray = nullptr; // The Feature Ids
  DataStoreType& m_Data;                                 // The data that is being compared
};

/**
 * @brief Functor for type-dispatched filling of a scalar slice buffer.
 * Converts typed data store values to float64 for uniform comparison.
 */
struct FillScalarSliceBufferFunctor
{
  template <typename T>
  void operator()(IDataArray* dataArray, int64 baseIndex, usize sliceSize, std::vector<float64>& buffer, usize bufferOffset)
  {
    auto& store = dataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    for(usize i = 0; i < sliceSize; i++)
    {
      buffer[bufferOffset + i] = static_cast<float64>(store[static_cast<usize>(baseIndex) + i]);
    }
  }
};
} // namespace

ScalarSegmentFeatures::ScalarSegmentFeatures(DataStructure& dataStructure, ScalarSegmentFeaturesInputValues* inputValues, const std::atomic_bool& shouldCancel,
                                             const IFilter::MessageHandler& mesgHandler)
: SegmentFeatures(dataStructure, shouldCancel, mesgHandler)
, m_InputValues(inputValues)
{
  m_IsPeriodic = inputValues->IsPeriodic;
}

ScalarSegmentFeatures::~ScalarSegmentFeatures() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ScalarSegmentFeatures::operator()()
{
  this->m_NeighborScheme = m_InputValues->NeighborScheme;
  if(m_InputValues->UseMask)
  {
    try
    {
      m_GoodVoxels = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
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

  auto* inputDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputDataPath);
  m_InputDataArray = inputDataArray;
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

  // Dispatch between DFS (in-core) and CCL (OOC) algorithms
  if(IsOutOfCore(*m_FeatureIdsArray) || ForceOocAlgorithm())
  {
    SizeVec3 udims = gridGeom->getDimensions();
    allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));

    auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
    executeCCL(gridGeom, featureIdsStore);

    deallocateSliceBuffers();
  }
  else
  {
    execute(gridGeom);
  }
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

// -----------------------------------------------------------------------------
bool ScalarSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz = point / m_BufSliceSize;
    const int slot = static_cast<int>(iz % 2);
    if(m_BufferedSliceZ[slot] == iz)
    {
      const usize off = static_cast<usize>(slot) * static_cast<usize>(m_BufSliceSize) + static_cast<usize>(point - iz * m_BufSliceSize);
      if(m_InputValues->UseMask && m_MaskBuffer[off] == 0)
      {
        return false;
      }
      return true;
    }
  }

  // Fallback: direct OOC access
  if(m_InputValues->UseMask && !m_GoodVoxels->isTrue(point))
  {
    return false;
  }
  return true;
}

// -----------------------------------------------------------------------------
bool ScalarSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz1 = point1 / m_BufSliceSize;
    const int slot1 = static_cast<int>(iz1 % 2);
    const int64 iz2 = point2 / m_BufSliceSize;
    const int slot2 = static_cast<int>(iz2 % 2);

    if(m_BufferedSliceZ[slot1] == iz1 && m_BufferedSliceZ[slot2] == iz2)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off1 = static_cast<usize>(slot1) * sliceSize + static_cast<usize>(point1 - iz1 * m_BufSliceSize);
      const usize off2 = static_cast<usize>(slot2) * sliceSize + static_cast<usize>(point2 - iz2 * m_BufSliceSize);

      // Check point2 validity
      if(m_InputValues->UseMask && m_MaskBuffer[off2] == 0)
      {
        return false;
      }

      // Compare scalar values from the pre-loaded buffer
      float64 val1 = m_ScalarBuffer[off1];
      float64 val2 = m_ScalarBuffer[off2];
      float64 diff = val1 >= val2 ? (val1 - val2) : (val2 - val1);
      return diff <= static_cast<float64>(m_InputValues->ScalarTolerance);
    }
  }

  // Fallback: direct OOC access
  if(!isValidVoxel(point2))
  {
    return false;
  }
  return m_CompareFunctor->compare(point1, point2);
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  m_ScalarBuffer.resize(2 * sliceSize);
  m_MaskBuffer.resize(2 * sliceSize);
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_UseSliceBuffers = true;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_ScalarBuffer = std::vector<float64>();
  m_MaskBuffer = std::vector<uint8>();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
}

// -----------------------------------------------------------------------------
void ScalarSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return;
  }
  if(!m_UseSliceBuffers)
  {
    return;
  }

  const int slot = static_cast<int>(iz % 2);
  if(m_BufferedSliceZ[slot] == iz)
  {
    return;
  }

  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  const usize slotOffset = static_cast<usize>(slot) * sliceSize;
  const int64 baseIndex = iz * m_BufSliceSize;

  // Fill scalar data buffer using type dispatch
  DataType dataType = m_InputDataArray->getDataType();
  if(dataType == DataType::boolean)
  {
    auto& store = m_InputDataArray->template getIDataStoreRefAs<AbstractDataStore<bool>>();
    for(usize i = 0; i < sliceSize; i++)
    {
      m_ScalarBuffer[slotOffset + i] = store[static_cast<usize>(baseIndex) + i] ? 1.0 : 0.0;
    }
  }
  else
  {
    ExecuteDataFunctionNoBool(FillScalarSliceBufferFunctor{}, dataType, m_InputDataArray, baseIndex, sliceSize, m_ScalarBuffer, slotOffset);
  }

  // Fill mask buffer
  if(m_InputValues->UseMask && m_GoodVoxels != nullptr)
  {
    for(usize i = 0; i < sliceSize; i++)
    {
      m_MaskBuffer[slotOffset + i] = m_GoodVoxels->isTrue(static_cast<usize>(baseIndex) + i) ? 1 : 0;
    }
  }
  else
  {
    std::fill(m_MaskBuffer.begin() + slotOffset, m_MaskBuffer.begin() + slotOffset + sliceSize, static_cast<uint8>(1));
  }

  m_BufferedSliceZ[slot] = iz;
}
