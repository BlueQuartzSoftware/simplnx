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
 * @class TSpecificCompareFunctorBool
 * @brief Compares resident Boolean values for exact equality.
 */
class TSpecificCompareFunctorBool : public SegmentFeatures::CompareFunctor
{
public:
  using DataArrayType = BoolArray;
  CX_DEFAULT_CONSTRUCTORS(TSpecificCompareFunctorBool)

  /**
   * @brief Initializes the resident Boolean comparator.
   * @param data Supplies Boolean values.
   * @param length Exclusive upper bound for valid indexes.
   * @pre data identifies a BoolArray and outlives this comparator.
   */
  TSpecificCompareFunctorBool(IDataArray* data, int64 length)
  : m_Length(length)
  , m_Data(dynamic_cast<DataArrayType*>(data))
  {
  }
  TSpecificCompareFunctorBool() = default;
  ~TSpecificCompareFunctorBool() override = default;

  /**
   * @brief Tests two Boolean values for equality.
   * @param index First flat value index.
   * @param neighIndex Neighbor flat value index.
   * @return True when both indexes are in range and values are equal.
   * @pre index and neighIndex are nonnegative.
   */
  bool compare(int64 index, int64 neighIndex) override
  {
    if(index >= m_Length || neighIndex >= m_Length)
    {
      return false;
    }
    return (*m_Data)[neighIndex] == (*m_Data)[index];
  }

private:
  int64 m_Length = 0;
  DataArrayType* m_Data = nullptr;
};

/**
 * @class TSpecificCompareFunctor
 * @brief Compares resident scalar values in their native type.
 * @tparam T Specifies the scalar value type.
 *
 * The buffered CCL normally compares preloaded float64 slice values; this
 * fallback preserves native typed behavior outside that window. Signed
 * subtraction requires a representable absolute difference.
 */
template <class T>
class TSpecificCompareFunctor : public SegmentFeatures::CompareFunctor
{
public:
  CX_DEFAULT_CONSTRUCTORS(TSpecificCompareFunctor)

  using DataArrayType = DataArray<T>;
  using DataStoreType = AbstractDataStore<T>;

  /**
   * @brief Initializes the resident typed comparator.
   * @param data Supplies scalar values.
   * @param length Exclusive upper bound for valid indexes.
   * @param tolerance Maximum native-type difference.
   * @pre data has type T and outlives this comparator.
   */
  TSpecificCompareFunctor(IDataArray* data, int64 length, T tolerance)
  : m_Length(length)
  , m_Tolerance(tolerance)
  , m_Data(data->template getIDataStoreRefAs<DataStoreType>())
  {
  }
  TSpecificCompareFunctor() = default;
  ~TSpecificCompareFunctor() override = default;

  /**
   * @brief Tests the native-type absolute difference.
   * @param index First flat value index.
   * @param neighIndex Neighbor flat value index.
   * @return True when both indexes are in range and their difference is within tolerance.
   * @pre index and neighIndex are nonnegative.
   * @pre Signed differences are representable in T.
   */
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
  int64 m_Length = 0;
  T m_Tolerance = static_cast<T>(0);
  DataStoreType& m_Data;
};

/**
 * @struct FillScalarSliceBufferFunctor
 * @brief Bulk-reads one typed scalar slice and widens it to float64.
 */
struct FillScalarSliceBufferFunctor
{
  /**
   * @brief Reads one scalar slice into the selected LRU slot.
   * @tparam T Specifies the scalar input type.
   * @param dataArray Supplies scalar values.
   * @param baseIndex First flat source value.
   * @param sliceSize Number of values to read.
   * @param buffer Receives widened values.
   * @param bufferOffset First destination-buffer index.
   * @return Source bulk-read result.
   * @pre baseIndex is nonnegative.
   * @pre Source and destination ranges contain sliceSize values.
   */
  template <typename T>
  Result<> operator()(IDataArray* dataArray, int64 baseIndex, usize sliceSize, std::vector<float64>& buffer, usize bufferOffset)
  {
    auto& store = dataArray->template getIDataStoreRefAs<AbstractDataStore<T>>();
    // A raw buffer provides contiguous storage for every dispatched type.
    auto tempBuffer = std::make_unique<T[]>(sliceSize);
    auto readResult = store.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<T>(tempBuffer.get(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < sliceSize; i++)
    {
      buffer[bufferOffset + i] = static_cast<float64>(tempBuffer[i]);
    }
    return {};
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
      // Direct algorithm callers can bypass filter path and type validation.
      std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
      return MakeErrorResult(-54110, message);
    }
  }

  auto* gridGeom = m_DataStructure.getDataAs<IGridGeometry>(m_InputValues->ImageGeometryPath);

  m_FeatureIdsArray = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);

  auto* inputDataArray = m_DataStructure.getDataAs<IDataArray>(m_InputValues->InputDataPath);
  m_InputDataArray = inputDataArray;
  usize inDataPoints = inputDataArray->getNumberOfTuples();
  nx::core::DataType dataType = inputDataArray->getDataType();

  switch(dataType)
  {
  case nx::core::DataType::int8: {
    m_CompareFunctor = std::make_shared<::TSpecificCompareFunctor<int8>>(inputDataArray, inDataPoints, static_cast<int8>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::uint8: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint8>>(inputDataArray, inDataPoints, static_cast<uint8>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::boolean: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctorBool>(inputDataArray, inDataPoints);
    break;
  }
  case nx::core::DataType::int16: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int16>>(inputDataArray, inDataPoints, static_cast<int16>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::uint16: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint16>>(inputDataArray, inDataPoints, static_cast<uint16>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::int32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int32>>(inputDataArray, inDataPoints, static_cast<int32>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::uint32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint32>>(inputDataArray, inDataPoints, static_cast<uint32>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::int64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<int64>>(inputDataArray, inDataPoints, static_cast<int64>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::uint64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<uint64>>(inputDataArray, inDataPoints, static_cast<uint64>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::float32: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<float32>>(inputDataArray, inDataPoints, static_cast<float32>(m_InputValues->ScalarTolerance));
    break;
  }
  case nx::core::DataType::float64: {
    m_CompareFunctor = std::make_shared<TSpecificCompareFunctor<float64>>(inputDataArray, inDataPoints, static_cast<float64>(m_InputValues->ScalarTolerance));
    break;
  }
  default:
    break;
  }
  if(inputDataArray->getNumberOfComponents() != 1)
  {
    m_CompareFunctor = std::make_shared<SegmentFeatures::CompareFunctor>();
  }

  SizeVec3 udims = gridGeom->getDimensions();
  allocateSliceBuffers(static_cast<int64>(udims[0]), static_cast<int64>(udims[1]));

  auto& featureIdsStore = m_FeatureIdsArray->getDataStoreRef();
  const auto* maskArray = m_InputValues->UseMask ? m_DataStructure.getDataAs<IDataArray>(m_InputValues->MaskArrayPath) : nullptr;
  const bool usesOutOfCoreInput = IsOutOfCore(*m_InputDataArray) || (maskArray != nullptr && IsOutOfCore(*maskArray));
  Result<> segmentResult = executeCCL(gridGeom, featureIdsStore, usesOutOfCoreInput);

  deallocateSliceBuffers();

  if(segmentResult.invalid())
  {
    return segmentResult;
  }
  if(m_ShouldCancel)
  {
    return {};
  }

  if(m_FoundFeatures < 1)
  {
    return MakeErrorResult(-87000, "No Features were detected: no Cell was eligible to seed a Feature. Every Cell is excluded by the Mask.");
  }

  // Final feature IDs include positive labels and the reserved zero tuple.
  ShapeType tDims = {static_cast<usize>(m_FoundFeatures + 1)};
  auto& cellFeaturesAM = m_DataStructure.getDataRefAs<AttributeMatrix>(m_InputValues->CellFeatureAttributeMatrixPath);
  cellFeaturesAM.resizeTuples(tDims);

  // Positive features start active. Feature zero remains reserved background.
  auto* activeArray = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->ActiveArrayPath);
  activeArray->getDataStore()->fill(1);
  (*activeArray)[0] = 0;

  // A deterministic permutation improves adjacent-feature color contrast.
  if(m_InputValues->RandomizeFeatureIds)
  {
    randomizeFeatureIds(m_FeatureIdsArray, m_FoundFeatures + 1);
  }

  return {};
}

bool ScalarSegmentFeatures::isValidVoxel(int64 point) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz = point / m_BufSliceSize;
    const int slot = m_BufferedSliceZ[0] == iz ? 0 : (m_BufferedSliceZ[1] == iz ? 1 : -1);
    if(slot >= 0)
    {
      const usize off = static_cast<usize>(slot) * static_cast<usize>(m_BufSliceSize) + static_cast<usize>(point - iz * m_BufSliceSize);
      if(m_InputValues->UseMask && m_MaskBuffer[off] == 0)
      {
        return false;
      }
      return true;
    }
    return false;
  }

  // Direct fallback supports calls outside buffered CCL execution.
  if(m_InputValues->UseMask && !m_GoodVoxels->isTrue(point))
  {
    return false;
  }
  return true;
}

bool ScalarSegmentFeatures::areNeighborsSimilar(int64 point1, int64 point2) const
{
  if(m_UseSliceBuffers)
  {
    const int64 iz1 = point1 / m_BufSliceSize;
    const int slot1 = m_BufferedSliceZ[0] == iz1 ? 0 : (m_BufferedSliceZ[1] == iz1 ? 1 : -1);
    const int64 iz2 = point2 / m_BufSliceSize;
    const int slot2 = m_BufferedSliceZ[0] == iz2 ? 0 : (m_BufferedSliceZ[1] == iz2 ? 1 : -1);

    if(slot1 >= 0 && slot2 >= 0)
    {
      const usize sliceSize = static_cast<usize>(m_BufSliceSize);
      const usize off1 = static_cast<usize>(slot1) * sliceSize + static_cast<usize>(point1 - iz1 * m_BufSliceSize);
      const usize off2 = static_cast<usize>(slot2) * sliceSize + static_cast<usize>(point2 - iz2 * m_BufSliceSize);

      // Similarity also requires the candidate neighbor to pass the mask.
      if(m_InputValues->UseMask && m_MaskBuffer[off2] == 0)
      {
        return false;
      }

      // Widened comparison avoids type dispatch in the CCL neighbor loop.
      float64 val1 = m_ScalarBuffer[off1];
      float64 val2 = m_ScalarBuffer[off2];
      float64 diff = val1 >= val2 ? (val1 - val2) : (val2 - val1);
      return diff <= static_cast<float64>(m_InputValues->ScalarTolerance);
    }
    return false;
  }

  // Direct fallback supports calls outside buffered CCL execution.
  if(!isValidVoxel(point2))
  {
    return false;
  }
  return m_CompareFunctor->compare(point1, point2);
}

void ScalarSegmentFeatures::allocateSliceBuffers(int64 dimX, int64 dimY)
{
  m_BufSliceSize = dimX * dimY;
  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  m_ScalarBuffer.resize(2 * sliceSize);
  m_MaskBuffer.resize(2 * sliceSize);
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_BufferUseSequence = {0, 0};
  m_NextBufferUseSequence = 1;
  m_UseSliceBuffers = true;
}

void ScalarSegmentFeatures::deallocateSliceBuffers()
{
  m_UseSliceBuffers = false;
  m_ScalarBuffer = std::vector<float64>();
  m_MaskBuffer = std::vector<uint8>();
  m_BufferedSliceZ[0] = -1;
  m_BufferedSliceZ[1] = -1;
  m_BufferUseSequence = {0, 0};
  m_NextBufferUseSequence = 1;
}

Result<> ScalarSegmentFeatures::prepareForSlice(int64 iz, int64 dimX, int64 dimY, int64 dimZ)
{
  if(iz < 0)
  {
    m_UseSliceBuffers = false;
    return {};
  }
  if(!m_UseSliceBuffers)
  {
    return {};
  }

  int slot = m_BufferedSliceZ[0] == iz ? 0 : (m_BufferedSliceZ[1] == iz ? 1 : -1);
  if(slot >= 0)
  {
    m_BufferUseSequence[static_cast<usize>(slot)] = m_NextBufferUseSequence++;
    return {};
  }
  if(m_BufferedSliceZ[0] < 0)
  {
    slot = 0;
  }
  else if(m_BufferedSliceZ[1] < 0)
  {
    slot = 1;
  }
  else
  {
    slot = m_BufferUseSequence[0] <= m_BufferUseSequence[1] ? 0 : 1;
  }

  const usize sliceSize = static_cast<usize>(m_BufSliceSize);
  const usize slotOffset = static_cast<usize>(slot) * sliceSize;
  const int64 baseIndex = iz * m_BufSliceSize;

  // Widen one scalar slice into the selected LRU slot.
  DataType dataType = m_InputDataArray->getDataType();
  if(dataType == DataType::boolean)
  {
    auto& store = m_InputDataArray->template getIDataStoreRefAs<AbstractDataStore<bool>>();
    // Bool dispatch uses a raw temporary because vector<bool> is not contiguous.
    auto boolBuf = std::make_unique<bool[]>(sliceSize);
    auto readResult = store.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<bool>(boolBuf.get(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }
    for(usize i = 0; i < sliceSize; i++)
    {
      m_ScalarBuffer[slotOffset + i] = boolBuf[i] ? 1.0 : 0.0;
    }
  }
  else
  {
    auto readResult = ExecuteDataFunctionNoBool(FillScalarSliceBufferFunctor{}, dataType, m_InputDataArray, baseIndex, sliceSize, m_ScalarBuffer, slotOffset);
    if(readResult.invalid())
    {
      return readResult;
    }
  }

  // Cache mask values with the scalar slice.
  if(m_InputValues->UseMask && m_GoodVoxels != nullptr)
  {
    auto& maskArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->MaskArrayPath);
    if(maskArray.getDataType() == DataType::uint8)
    {
      auto& typedStore = maskArray.getIDataStoreRefAs<AbstractDataStore<uint8>>();
      auto readResult = typedStore.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<uint8>(m_MaskBuffer.data() + slotOffset, sliceSize));
      if(readResult.invalid())
      {
        return readResult;
      }
    }
    else if(maskArray.getDataType() == DataType::boolean)
    {
      // Bool mask input needs a contiguous temporary buffer.
      auto& typedStore = maskArray.getIDataStoreRefAs<AbstractDataStore<bool>>();
      auto boolBuf = std::make_unique<bool[]>(sliceSize);
      auto readResult = typedStore.copyIntoBuffer(static_cast<usize>(baseIndex), nonstd::span<bool>(boolBuf.get(), sliceSize));
      if(readResult.invalid())
      {
        return readResult;
      }
      for(usize i = 0; i < sliceSize; i++)
      {
        m_MaskBuffer[slotOffset + i] = boolBuf[i] ? 1 : 0;
      }
    }
    else
    {
      return MakeErrorResult(-54111, "ScalarSegmentFeatures mask storage must be Bool or UInt8.");
    }
  }
  else
  {
    std::fill(m_MaskBuffer.begin() + slotOffset, m_MaskBuffer.begin() + slotOffset + sliceSize, static_cast<uint8>(1));
  }

  m_BufferedSliceZ[slot] = iz;
  m_BufferUseSequence[static_cast<usize>(slot)] = m_NextBufferUseSequence++;
  return {};
}
