#include "ConvertData.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelDataAlgorithm.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <type_traits>

using namespace nx::core;

namespace Detail
{
// Each bulk input and output buffer contains at most 65,536 values.
constexpr usize k_ConvertChunkSize = 65'536;

/**
 * @class ConvertDataDirectValues
 * @brief Converts values in one direct parallel range.
 * @tparam InputType Specifies the source scalar type.
 * @tparam OutputType Specifies the destination scalar type.
 *
 * This worker accesses DataStore instances in parallel. In-memory residency does
 * not give generic DataStore thread-safety guarantees.
 */
template <typename InputType, typename OutputType>
class ConvertDataDirectValues
{
public:
  using InputStoreType = AbstractDataStore<InputType>;
  using OutputStoreType = AbstractDataStore<OutputType>;

  ConvertDataDirectValues(const InputStoreType& inputStore, OutputStoreType& outputStore, const std::atomic_bool& shouldCancel)
  : m_InputStore(inputStore)
  , m_OutputStore(outputStore)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Converts a half-open value range.
   * @param start Specifies the first value index.
   * @param end Specifies the exclusive value index.
   */
  void convert(usize start, usize end) const
  {
    for(usize index = start; index < end; index++)
    {
      if constexpr(std::is_same_v<InputType, OutputType>)
      {
        m_OutputStore.setValue(index, m_InputStore.getValue(index));
      }
      else if constexpr(std::is_same_v<InputType, bool>)
      {
        m_OutputStore.setValue(index, m_InputStore.getValue(index) ? 1 : 0);
      }
      else if constexpr(std::is_same_v<OutputType, bool>)
      {
        m_OutputStore.setValue(index, m_InputStore.getValue(index) != 0);
      }
      else
      {
        m_OutputStore.setValue(index, static_cast<OutputType>(m_InputStore.getValue(index)));
      }
    }
  }

  /**
   * @brief Converts one parallel range.
   * @param range Specifies the half-open value range.
   *
   * The worker checks cancellation before, but not during, its range.
   */
  void operator()(const Range& range) const
  {
    if(m_ShouldCancel)
    {
      return;
    }
    convert(range.min(), range.max());
  }

private:
  const InputStoreType& m_InputStore;
  OutputStoreType& m_OutputStore;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ConvertDataDirect
 * @brief Dispatches direct parallel value conversion.
 * @tparam InputType Specifies the source scalar type.
 * @tparam OutputType Specifies the destination scalar type.
 */
template <typename InputType, typename OutputType>
class ConvertDataDirect
{
public:
  ConvertDataDirect(const DataArray<InputType>& inputArray, DataArray<OutputType>& outputArray, const std::atomic_bool& shouldCancel)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Converts all values through in-memory arrays.
   * @return Success after conversion or cancellation.
   */
  Result<> operator()() const
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    typename IParallelAlgorithm::AlgorithmArrays algArrays;
    algArrays.push_back(&m_InputArray);
    algArrays.push_back(&m_OutputArray);

    ParallelDataAlgorithm dataAlg;
    dataAlg.setRange(0, m_InputArray.size());
    dataAlg.requireArraysInMemory(algArrays);
    dataAlg.execute(ConvertDataDirectValues<InputType, OutputType>(m_InputArray.getDataStoreRef(), m_OutputArray.getDataStoreRef(), m_ShouldCancel));
    return {};
  }

private:
  const DataArray<InputType>& m_InputArray;
  DataArray<OutputType>& m_OutputArray;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ConvertDataBulk
 * @brief Converts values through bounded bulk buffers.
 * @tparam InputType Specifies the source scalar type.
 * @tparam OutputType Specifies the destination scalar type.
 *
 * Each completed chunk remains written after cancellation.
 */
template <typename InputType, typename OutputType>
class ConvertDataBulk
{
public:
  ConvertDataBulk(const DataArray<InputType>& inputArray, DataArray<OutputType>& outputArray, const std::atomic_bool& shouldCancel)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Converts all values through bulk I/O.
   * @return Error from bulk I/O, or success after cancellation.
   */
  Result<> operator()() const
  {
    const auto& inputStore = m_InputArray.getDataStoreRef();
    auto& outputStore = m_OutputArray.getDataStoreRef();
    const usize numValues = m_InputArray.size();
    auto inputBuffer = std::make_unique<InputType[]>(k_ConvertChunkSize);
    auto outputBuffer = std::make_unique<OutputType[]>(k_ConvertChunkSize);

    for(usize offset = 0; offset < numValues; offset += k_ConvertChunkSize)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      const usize count = std::min(k_ConvertChunkSize, numValues - offset);
      Result<> inputResult = inputStore.copyIntoBuffer(offset, nonstd::span<InputType>(inputBuffer.get(), count));
      if(inputResult.invalid())
      {
        return inputResult;
      }

      for(usize index = 0; index < count; index++)
      {
        if constexpr(std::is_same_v<InputType, OutputType>)
        {
          outputBuffer[index] = inputBuffer[index];
        }
        else if constexpr(std::is_same_v<InputType, bool>)
        {
          outputBuffer[index] = inputBuffer[index] ? 1 : 0;
        }
        else if constexpr(std::is_same_v<OutputType, bool>)
        {
          outputBuffer[index] = inputBuffer[index] != 0;
        }
        else
        {
          outputBuffer[index] = static_cast<OutputType>(inputBuffer[index]);
        }
      }

      Result<> outputResult = outputStore.copyFromBuffer(offset, nonstd::span<const OutputType>(outputBuffer.get(), count));
      if(outputResult.invalid())
      {
        return outputResult;
      }
    }

    return {};
  }

private:
  const DataArray<InputType>& m_InputArray;
  DataArray<OutputType>& m_OutputArray;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @brief Selects direct or bulk conversion for a source and target type.
 * @tparam InputType Specifies the source scalar type.
 * @tparam OutputType Specifies the destination scalar type.
 * @param dataStructure Provides selected arrays.
 * @param inputValues Specifies validated conversion settings.
 * @param shouldCancel Stops later work when true.
 * @return Error from the selected implementation.
 */
template <typename InputType, typename OutputType>
Result<> ConvertData(DataStructure& dataStructure, const ConvertDataInputValues* inputValues, const std::atomic_bool& shouldCancel)
{
  const auto& inputArray = dataStructure.getDataRefAs<DataArray<InputType>>(inputValues->SelectedArrayPath);
  auto& outputArray = dataStructure.getDataRefAs<DataArray<OutputType>>(inputValues->OutputArrayName);
  return DispatchAlgorithm<ConvertDataDirect<InputType, OutputType>, ConvertDataBulk<InputType, OutputType>>({&inputArray, &outputArray}, inputArray, outputArray, shouldCancel);
}

/**
 * @brief Selects the target scalar conversion for a source type.
 * @tparam InputType Specifies the source scalar type.
 * @param dataStructure Provides selected arrays.
 * @param inputValues Specifies validated conversion settings.
 * @param shouldCancel Stops later work when true.
 * @return Error for an unsupported target type, or the selected conversion result.
 */
template <typename InputType>
Result<> ConvertData(DataStructure& dataStructure, const ConvertDataInputValues* inputValues, const std::atomic_bool& shouldCancel)
{
  const auto& inputArray = dataStructure.getDataRefAs<DataArray<InputType>>(inputValues->SelectedArrayPath);
  switch(inputValues->ScalarType)
  {
  case DataType::int8:
    return ConvertData<InputType, int8>(dataStructure, inputValues, shouldCancel);
  case DataType::uint8:
    return ConvertData<InputType, uint8>(dataStructure, inputValues, shouldCancel);
  case DataType::int16:
    return ConvertData<InputType, int16>(dataStructure, inputValues, shouldCancel);
  case DataType::uint16:
    return ConvertData<InputType, uint16>(dataStructure, inputValues, shouldCancel);
  case DataType::int32:
    return ConvertData<InputType, int32>(dataStructure, inputValues, shouldCancel);
  case DataType::uint32:
    return ConvertData<InputType, uint32>(dataStructure, inputValues, shouldCancel);
  case DataType::int64:
    return ConvertData<InputType, int64>(dataStructure, inputValues, shouldCancel);
  case DataType::uint64:
    return ConvertData<InputType, uint64>(dataStructure, inputValues, shouldCancel);
  case DataType::float32:
    return ConvertData<InputType, float32>(dataStructure, inputValues, shouldCancel);
  case DataType::float64:
    return ConvertData<InputType, float64>(dataStructure, inputValues, shouldCancel);
  case DataType::boolean:
    return ConvertData<InputType, bool>(dataStructure, inputValues, shouldCancel);
  default:
    return MakeErrorResult(
        -399, fmt::format("Error Converting DataArray '{}' from type '{}' to type '{}'", inputArray.getName(), DataTypeToString(inputArray.getDataType()), DataTypeToString(inputValues->ScalarType)));
  }
}

/**
 * @struct ConvertDataFunctor
 * @brief Adapts runtime source types to typed conversion.
 */
struct ConvertDataFunctor
{
  template <typename InputType>
  Result<> operator()(DataStructure& dataStructure, const ConvertDataInputValues* inputValues, const std::atomic_bool& shouldCancel) const
  {
    return ConvertData<InputType>(dataStructure, inputValues, shouldCancel);
  }
};
} // namespace Detail

ConvertData::ConvertData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertDataInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ConvertData::~ConvertData() noexcept = default;

const std::atomic_bool& ConvertData::getCancel()
{
  return m_ShouldCancel;
}

Result<> ConvertData::operator()()
{
  const auto& inputArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
  return ExecuteDataFunction(Detail::ConvertDataFunctor{}, inputArray.getDataType(), m_DataStructure, m_InputValues, m_ShouldCancel);
}
