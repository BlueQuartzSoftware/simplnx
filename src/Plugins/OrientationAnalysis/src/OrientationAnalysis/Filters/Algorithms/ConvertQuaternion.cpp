#include "ConvertQuaternion.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <algorithm>
#include <memory>
#include <nonstd/span.hpp>

using namespace nx::core;

namespace
{

constexpr ChoicesParameter::ValueType k_ToVectorScalar = 1;
constexpr usize k_QuaternionComponents = 4;
constexpr usize k_ChunkTuples = 65536;

/**
 * @brief Converts one quaternion component order.
 * @tparam T Specifies the floating-point value type.
 * @tparam ToVectorScalar Selects scalar-last output order.
 * @param input Provides four input components.
 * @param output Receives four output components.
 */
template <typename T, bool ToVectorScalar>
void ConvertTuple(const T* input, T* output)
{
  // Preserve the legacy Float64 behavior, which narrows each component through float32.
  const float32 first = static_cast<float32>(input[0]);
  const float32 second = static_cast<float32>(input[1]);
  const float32 third = static_cast<float32>(input[2]);
  const float32 fourth = static_cast<float32>(input[3]);

  if constexpr(ToVectorScalar)
  {
    output[0] = static_cast<T>(second);
    output[1] = static_cast<T>(third);
    output[2] = static_cast<T>(fourth);
    output[3] = static_cast<T>(first);
  }
  else
  {
    output[0] = static_cast<T>(fourth);
    output[1] = static_cast<T>(first);
    output[2] = static_cast<T>(second);
    output[3] = static_cast<T>(third);
  }
}

template <typename T, bool ToVectorScalar>
void ConvertContiguousRange(const T* input, T* output, usize start, usize end, const std::atomic_bool& shouldCancel)
{
  for(usize blockStart = start; blockStart < end; blockStart += k_ChunkTuples)
  {
    if(shouldCancel)
    {
      return;
    }

    const usize blockEnd = std::min(blockStart + k_ChunkTuples, end);
    for(usize tupleIndex = blockStart; tupleIndex < blockEnd; tupleIndex++)
    {
      const usize componentOffset = tupleIndex * k_QuaternionComponents;
      ConvertTuple<T, ToVectorScalar>(input + componentOffset, output + componentOffset);
    }
  }
}

/**
 * @class ConvertQuaternionContiguousImpl
 * @brief Converts contiguous quaternion ranges through raw pointers.
 * @tparam T Specifies the floating-point value type.
 */
template <typename T>
class ConvertQuaternionContiguousImpl
{
public:
  ConvertQuaternionContiguousImpl(const T* input, T* output, ChoicesParameter::ValueType conversionType, const std::atomic_bool& shouldCancel)
  : m_Input(input)
  , m_Output(output)
  , m_ConversionType(conversionType)
  , m_ShouldCancel(shouldCancel)
  {
  }

  void operator()(const Range& range) const
  {
    if(m_ConversionType == k_ToVectorScalar)
    {
      ConvertContiguousRange<T, true>(m_Input, m_Output, range.min(), range.max(), m_ShouldCancel);
    }
    else
    {
      ConvertContiguousRange<T, false>(m_Input, m_Output, range.min(), range.max(), m_ShouldCancel);
    }
  }

private:
  const T* m_Input = nullptr;
  T* m_Output = nullptr;
  ChoicesParameter::ValueType m_ConversionType = 0;
  const std::atomic_bool& m_ShouldCancel;
};

template <typename T, bool ToVectorScalar>
void ConvertBuffer(T* buffer, usize tupleCount)
{
  for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
  {
    const usize componentOffset = tupleIndex * k_QuaternionComponents;
    ConvertTuple<T, ToVectorScalar>(buffer + componentOffset, buffer + componentOffset);
  }
}

/**
 * @class ConvertQuaternionScanlineType
 * @brief Converts bounded bulk-I/O quaternion blocks.
 * @tparam T Specifies the floating-point value type.
 */
template <typename T>
class ConvertQuaternionScanlineType
{
public:
  ConvertQuaternionScanlineType(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ConvertQuaternionInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  Result<> operator()()
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_InputValues->QuaternionDataArrayPath);
    auto& outputArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_InputValues->OutputDataArrayPath);
    const auto& inputStoreRef = inputArray.getDataStoreRef();
    auto& outputStoreRef = outputArray.getDataStoreRef();
    const usize totalTuples = inputArray.getNumberOfTuples();

    auto buffer = std::make_unique<T[]>(k_ChunkTuples * k_QuaternionComponents);
    for(usize tupleOffset = 0; tupleOffset < totalTuples; tupleOffset += k_ChunkTuples)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(k_ChunkTuples, totalTuples - tupleOffset);
      const usize valueCount = tupleCount * k_QuaternionComponents;
      const usize valueOffset = tupleOffset * k_QuaternionComponents;
      if(Result<> result = inputStoreRef.copyIntoBuffer(valueOffset, nonstd::span<T>(buffer.get(), valueCount)); result.invalid())
      {
        return result;
      }

      if(m_InputValues->ConversionType == k_ToVectorScalar)
      {
        ConvertBuffer<T, true>(buffer.get(), tupleCount);
      }
      else
      {
        ConvertBuffer<T, false>(buffer.get(), tupleCount);
      }

      if(Result<> result = outputStoreRef.copyFromBuffer(valueOffset, nonstd::span<const T>(buffer.get(), valueCount)); result.invalid())
      {
        return result;
      }
    }

    return {};
  }

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ConvertQuaternionInputValues* m_InputValues = nullptr;
};

/**
 * @class ConvertQuaternionDirectType
 * @brief Converts direct quaternion arrays.
 * @tparam T Specifies the floating-point value type.
 */
template <typename T>
class ConvertQuaternionDirectType
{
public:
  ConvertQuaternionDirectType(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ConvertQuaternionInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  Result<> operator()()
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_InputValues->QuaternionDataArrayPath);
    auto& outputArray = m_DataStructure.getDataRefAs<DataArray<T>>(m_InputValues->OutputDataArrayPath);
    const auto* inputStorePtr = dynamic_cast<const DataStore<T>*>(&inputArray.getDataStoreRef());
    auto* outputStorePtr = dynamic_cast<DataStore<T>*>(&outputArray.getDataStoreRef());

    // A forced Direct path can still receive non-contiguous stores; retain bounded behavior.
    if(inputStorePtr == nullptr || outputStorePtr == nullptr)
    {
      return ConvertQuaternionScanlineType<T>(m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues)();
    }

    // Workers use disjoint raw-pointer ranges and never access non-thread-safe DataStore APIs.
    ParallelDataAlgorithm dataAlgorithm;
    dataAlgorithm.setRange(0, inputArray.getNumberOfTuples());
    dataAlgorithm.execute(ConvertQuaternionContiguousImpl<T>(inputStorePtr->data(), outputStorePtr->data(), m_InputValues->ConversionType, m_ShouldCancel));
    return {};
  }

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ConvertQuaternionInputValues* m_InputValues = nullptr;
};

/**
 * @class ConvertQuaternionDirect
 * @brief Dispatches a direct typed converter.
 */
class ConvertQuaternionDirect
{
public:
  ConvertQuaternionDirect(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ConvertQuaternionInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  Result<> operator()()
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->QuaternionDataArrayPath);
    return RunTemplateClass<ConvertQuaternionDirectType, ArrayUseFloatingTypes>(inputArray.getDataType(), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
  }

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ConvertQuaternionInputValues* m_InputValues = nullptr;
};

/**
 * @class ConvertQuaternionScanline
 * @brief Dispatches a scanline typed converter.
 */
class ConvertQuaternionScanline
{
public:
  ConvertQuaternionScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ConvertQuaternionInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_MessageHandler(messageHandler)
  , m_ShouldCancel(shouldCancel)
  , m_InputValues(inputValues)
  {
  }

  Result<> operator()()
  {
    const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->QuaternionDataArrayPath);
    return RunTemplateClass<ConvertQuaternionScanlineType, ArrayUseFloatingTypes>(inputArray.getDataType(), m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
  }

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ConvertQuaternionInputValues* m_InputValues = nullptr;
};

} // namespace

ConvertQuaternion::ConvertQuaternion(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ConvertQuaternionInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(messageHandler)
{
}

ConvertQuaternion::~ConvertQuaternion() noexcept = default;

const std::atomic_bool& ConvertQuaternion::getCancel()
{
  return m_ShouldCancel;
}

Result<> ConvertQuaternion::operator()()
{
  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->QuaternionDataArrayPath);
  if(inputArray.getDataType() != DataType::float32 && inputArray.getDataType() != DataType::float64)
  {
    return MakeErrorResult(-74836, fmt::format("The input quaternion array at path '{}' has data type '{}', but must be either Float32 or Float64.", m_InputValues->QuaternionDataArrayPath.toString(),
                                               DataTypeToString(inputArray.getDataType())));
  }

  const auto& outputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->OutputDataArrayPath);
  return DispatchAlgorithm<ConvertQuaternionDirect, ConvertQuaternionScanline>({&inputArray, &outputArray}, m_DataStructure, m_MessageHandler, m_ShouldCancel, m_InputValues);
}
