#include "ConditionalSetValue.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/Utilities/AlgorithmDispatch.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/StringInterpretationUtilities.hpp"

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>
#include <type_traits>

using namespace nx::core;

namespace
{
// Conditional processing rounds this value down to complete tuples. This avoids
// partial-tuple read-modify-write operations for disk-backed stores.
constexpr usize k_TargetChunkValues = 65536;

template <typename T>
inline constexpr bool k_IsConditionalType = std::is_same_v<T, bool> || std::is_same_v<T, uint8> || std::is_same_v<T, int8>;

/**
 * @brief Creates an unsupported conditional-array type error.
 * @return Error that identifies the accepted conditional types.
 */
Result<> MakeInvalidConditionalTypeError()
{
  return MakeErrorResult<>(-4001, "Mask array was not of type [BOOL | UINT8 | INT8].");
}

/**
 * @struct ReplaceValueInArrayDirectFunctor
 * @brief Replaces matching scalar values with direct array access.
 */
struct ReplaceValueInArrayDirectFunctor
{
  template <typename ScalarType>
  Result<> operator()(IDataArray& workingArray, const std::string& removeValue, const std::string& replaceValue, const std::atomic_bool& shouldCancel) const
  {
    auto& dataStore = workingArray.template getIDataStoreRefAs<AbstractDataStore<ScalarType>>();

    const ScalarType removeVal = StringInterpretationUtilities::Convert<ScalarType>(removeValue).value();
    const ScalarType replaceVal = StringInterpretationUtilities::Convert<ScalarType>(replaceValue).value();
    const usize size = dataStore.getNumberOfTuples() * dataStore.getNumberOfComponents();

    if(auto* contiguousStore = dynamic_cast<DataStore<ScalarType>*>(&dataStore); contiguousStore != nullptr)
    {
      ScalarType* values = contiguousStore->data();
      for(usize offset = 0; offset < size; offset += k_TargetChunkValues)
      {
        if(shouldCancel)
        {
          return {};
        }

        const usize end = std::min(offset + k_TargetChunkValues, size);
        for(usize index = offset; index < end; index++)
        {
          if(values[index] == removeVal)
          {
            values[index] = replaceVal;
          }
        }
      }
      return {};
    }

    for(usize offset = 0; offset < size; offset += k_TargetChunkValues)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize end = std::min(offset + k_TargetChunkValues, size);
      for(usize index = offset; index < end; index++)
      {
        if(dataStore[index] == removeVal)
        {
          dataStore[index] = replaceVal;
        }
      }
    }

    return {};
  }
};

/**
 * @struct ConditionalReplaceValueDirectFunctor
 * @brief Replaces complete target tuples selected by a conditional value.
 * @tparam TargetType Specifies the target scalar type.
 */
template <typename TargetType>
struct ConditionalReplaceValueDirectFunctor
{
  template <typename ConditionalType>
  Result<> operator()(IDataArray& targetArray, const IDataArray& conditionalArray, TargetType replaceValue, bool invertMask, const std::atomic_bool& shouldCancel) const
  {
    if constexpr(!k_IsConditionalType<ConditionalType>)
    {
      return MakeInvalidConditionalTypeError();
    }
    else
    {
      auto& targetDataArray = dynamic_cast<DataArray<TargetType>&>(targetArray);
      const auto& conditionalDataArray = dynamic_cast<const DataArray<ConditionalType>&>(conditionalArray);
      const usize numTuples = targetDataArray.getNumberOfTuples();
      const usize numComps = targetDataArray.getNumberOfComponents();
      const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComps);
      auto& targetStore = targetDataArray.getDataStoreRef();
      const auto& conditionalStore = conditionalDataArray.getDataStoreRef();

      auto* contiguousTargetStore = dynamic_cast<DataStore<TargetType>*>(&targetStore);
      const auto* contiguousConditionalStore = dynamic_cast<const DataStore<ConditionalType>*>(&conditionalStore);
      if(contiguousTargetStore != nullptr && contiguousConditionalStore != nullptr)
      {
        TargetType* targetValues = contiguousTargetStore->data();
        const ConditionalType* conditionalValues = contiguousConditionalStore->data();
        if(numComps == 1)
        {
          for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += chunkTuples)
          {
            if(shouldCancel)
            {
              return {};
            }

            const usize tupleEnd = std::min(tupleOffset + chunkTuples, numTuples);
            for(usize tupleIndex = tupleOffset; tupleIndex < tupleEnd; tupleIndex++)
            {
              if(static_cast<bool>(conditionalValues[tupleIndex]) != invertMask)
              {
                targetValues[tupleIndex] = replaceValue;
              }
            }
          }
          return {};
        }

        for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += chunkTuples)
        {
          if(shouldCancel)
          {
            return {};
          }

          const usize tupleEnd = std::min(tupleOffset + chunkTuples, numTuples);
          for(usize tupleIndex = tupleOffset; tupleIndex < tupleEnd; tupleIndex++)
          {
            if(static_cast<bool>(conditionalValues[tupleIndex]) != invertMask)
            {
              std::fill_n(targetValues + tupleIndex * numComps, numComps, replaceValue);
            }
          }
        }
        return {};
      }

      for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += chunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }

        const usize tupleEnd = std::min(tupleOffset + chunkTuples, numTuples);
        for(usize tupleIndex = tupleOffset; tupleIndex < tupleEnd; tupleIndex++)
        {
          const bool shouldReplace = static_cast<bool>(conditionalDataArray[tupleIndex]) != invertMask;
          if(shouldReplace)
          {
            targetDataArray.initializeTuple(tupleIndex, replaceValue);
          }
        }
      }

      return {};
    }
  }
};

/**
 * @struct ConditionalReplaceValueDirectTargetFunctor
 * @brief Converts a replacement string before direct conditional replacement.
 */
struct ConditionalReplaceValueDirectTargetFunctor
{
  template <typename TargetType>
  Result<> operator()(IDataArray& targetArray, const IDataArray& conditionalArray, const std::string& replaceValue, bool invertMask, const std::atomic_bool& shouldCancel) const
  {
    Result<TargetType> conversionResult = StringInterpretationUtilities::Convert<TargetType>(replaceValue);
    if(conversionResult.invalid())
    {
      return MakeErrorResult<>(-4000, "Input String Value could not be converted to the appropriate numeric type.");
    }

    return ExecuteDataFunction(ConditionalReplaceValueDirectFunctor<TargetType>{}, conditionalArray.getDataType(), targetArray, conditionalArray, conversionResult.value(), invertMask, shouldCancel);
  }
};

/**
 * @struct ReplaceValueInArrayScanlineFunctor
 * @brief Replaces matching scalar values through bounded bulk transfers.
 */
struct ReplaceValueInArrayScanlineFunctor
{
  template <typename ScalarType>
  Result<> operator()(IDataArray& workingArray, const std::string& removeValue, const std::string& replaceValue, const std::atomic_bool& shouldCancel) const
  {
    auto& dataStore = workingArray.template getIDataStoreRefAs<AbstractDataStore<ScalarType>>();
    const ScalarType removeVal = StringInterpretationUtilities::Convert<ScalarType>(removeValue).value();
    const ScalarType replaceVal = StringInterpretationUtilities::Convert<ScalarType>(replaceValue).value();
    const usize numTuples = dataStore.getNumberOfTuples();
    const usize numComps = dataStore.getNumberOfComponents();
    const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComps);
    const usize bufferSize = chunkTuples * numComps;
    auto buffer = std::make_unique<ScalarType[]>(bufferSize);

    for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += chunkTuples)
    {
      if(shouldCancel)
      {
        return {};
      }

      const usize tupleCount = std::min(chunkTuples, numTuples - tupleOffset);
      const usize valueOffset = tupleOffset * numComps;
      const usize valueCount = tupleCount * numComps;
      Result<> result = dataStore.copyIntoBuffer(valueOffset, nonstd::span<ScalarType>(buffer.get(), valueCount));
      if(result.invalid())
      {
        return result;
      }

      bool modified = false;
      for(usize index = 0; index < valueCount; index++)
      {
        if(buffer[index] == removeVal)
        {
          buffer[index] = replaceVal;
          modified = true;
        }
      }

      if(modified)
      {
        result = dataStore.copyFromBuffer(valueOffset, nonstd::span<const ScalarType>(buffer.get(), valueCount));
        if(result.invalid())
        {
          return result;
        }
      }
    }

    return {};
  }
};

/**
 * @struct ConditionalReplaceValueScanlineFunctor
 * @brief Replaces complete target tuples through bounded bulk transfers.
 * @tparam TargetType Specifies the target scalar type.
 */
template <typename TargetType>
struct ConditionalReplaceValueScanlineFunctor
{
  template <typename ConditionalType>
  Result<> operator()(IDataArray& targetArray, const IDataArray& conditionalArray, TargetType replaceValue, bool invertMask, const std::atomic_bool& shouldCancel) const
  {
    if constexpr(!k_IsConditionalType<ConditionalType>)
    {
      return MakeInvalidConditionalTypeError();
    }
    else
    {
      auto& targetStore = targetArray.template getIDataStoreRefAs<AbstractDataStore<TargetType>>();
      const auto& conditionalStore = conditionalArray.template getIDataStoreRefAs<AbstractDataStore<ConditionalType>>();
      const usize numTuples = targetStore.getNumberOfTuples();
      const usize numComps = targetStore.getNumberOfComponents();
      const usize chunkTuples = std::max<usize>(1, k_TargetChunkValues / numComps);
      const usize targetBufferSize = chunkTuples * numComps;
      auto targetBuffer = std::make_unique<TargetType[]>(targetBufferSize);
      auto conditionalBuffer = std::make_unique<ConditionalType[]>(chunkTuples);

      for(usize tupleOffset = 0; tupleOffset < numTuples; tupleOffset += chunkTuples)
      {
        if(shouldCancel)
        {
          return {};
        }

        const usize tupleCount = std::min(chunkTuples, numTuples - tupleOffset);
        const usize valueOffset = tupleOffset * numComps;
        const usize valueCount = tupleCount * numComps;
        Result<> result = conditionalStore.copyIntoBuffer(tupleOffset, nonstd::span<ConditionalType>(conditionalBuffer.get(), tupleCount));
        if(result.invalid())
        {
          return result;
        }
        result = targetStore.copyIntoBuffer(valueOffset, nonstd::span<TargetType>(targetBuffer.get(), valueCount));
        if(result.invalid())
        {
          return result;
        }

        bool modified = false;
        for(usize tupleIndex = 0; tupleIndex < tupleCount; tupleIndex++)
        {
          const bool shouldReplace = static_cast<bool>(conditionalBuffer[tupleIndex]) != invertMask;
          if(shouldReplace)
          {
            std::fill_n(targetBuffer.get() + tupleIndex * numComps, numComps, replaceValue);
            modified = true;
          }
        }

        if(modified)
        {
          result = targetStore.copyFromBuffer(valueOffset, nonstd::span<const TargetType>(targetBuffer.get(), valueCount));
          if(result.invalid())
          {
            return result;
          }
        }
      }

      return {};
    }
  }
};

/**
 * @struct ConditionalReplaceValueScanlineTargetFunctor
 * @brief Converts a replacement string before bulk conditional replacement.
 */
struct ConditionalReplaceValueScanlineTargetFunctor
{
  template <typename TargetType>
  Result<> operator()(IDataArray& targetArray, const IDataArray& conditionalArray, const std::string& replaceValue, bool invertMask, const std::atomic_bool& shouldCancel) const
  {
    Result<TargetType> conversionResult = StringInterpretationUtilities::Convert<TargetType>(replaceValue);
    if(conversionResult.invalid())
    {
      return MakeErrorResult<>(-4000, "Input String Value could not be converted to the appropriate numeric type.");
    }

    return ExecuteDataFunction(ConditionalReplaceValueScanlineFunctor<TargetType>{}, conditionalArray.getDataType(), targetArray, conditionalArray, conversionResult.value(), invertMask, shouldCancel);
  }
};

/**
 * @class ConditionalSetValueDirect
 * @brief Replaces values through in-memory target and condition arrays.
 *
 * Contiguous stores avoid staging copies and virtual per-value access.
 */
class ConditionalSetValueDirect
{
public:
  /**
   * @brief Creates a direct replacement algorithm.
   * @param dataStructure Provides selected arrays.
   * @param shouldCancel Stops later chunks when true.
   * @param inputValues Specifies validated paths and options. It must outlive
   * this algorithm.
   */
  ConditionalSetValueDirect(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const ConditionalSetValueInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Replaces selected target values.
   * @return Error from type conversion, or success after cancellation.
   */
  Result<> operator()()
  {
    auto& targetArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
    if(m_InputValues->UseConditional)
    {
      const auto& conditionalArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ConditionalArrayPath);
      return ExecuteDataFunction(ConditionalReplaceValueDirectTargetFunctor{}, targetArray.getDataType(), targetArray, conditionalArray, m_InputValues->ReplaceValue, m_InputValues->InvertMask,
                                 m_ShouldCancel);
    }

    return ExecuteDataFunction(ReplaceValueInArrayDirectFunctor{}, targetArray.getDataType(), targetArray, m_InputValues->RemoveValue, m_InputValues->ReplaceValue, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const ConditionalSetValueInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};

/**
 * @class ConditionalSetValueScanline
 * @brief Replaces values through bounded target and condition buffers.
 *
 * Bulk transfers eliminate per-cell disk access.
 */
class ConditionalSetValueScanline
{
public:
  /**
   * @brief Creates a bulk-I/O replacement algorithm.
   * @param dataStructure Provides selected arrays.
   * @param shouldCancel Stops later chunks when true.
   * @param inputValues Specifies validated paths and options. It must outlive
   * this algorithm.
   */
  ConditionalSetValueScanline(DataStructure& dataStructure, const std::atomic_bool& shouldCancel, const ConditionalSetValueInputValues* inputValues)
  : m_DataStructure(dataStructure)
  , m_InputValues(inputValues)
  , m_ShouldCancel(shouldCancel)
  {
  }

  /**
   * @brief Replaces selected target values.
   * @return Error from conversion or bulk I/O, or success after cancellation.
   */
  Result<> operator()()
  {
    auto& targetArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
    if(m_InputValues->UseConditional)
    {
      const auto& conditionalArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ConditionalArrayPath);
      return ExecuteDataFunction(ConditionalReplaceValueScanlineTargetFunctor{}, targetArray.getDataType(), targetArray, conditionalArray, m_InputValues->ReplaceValue, m_InputValues->InvertMask,
                                 m_ShouldCancel);
    }

    return ExecuteDataFunction(ReplaceValueInArrayScanlineFunctor{}, targetArray.getDataType(), targetArray, m_InputValues->RemoveValue, m_InputValues->ReplaceValue, m_ShouldCancel);
  }

private:
  DataStructure& m_DataStructure;
  const ConditionalSetValueInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace

ConditionalSetValue::ConditionalSetValue(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConditionalSetValueInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ConditionalSetValue::~ConditionalSetValue() noexcept = default;

Result<> ConditionalSetValue::operator()()
{
  auto& targetArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->SelectedArrayPath);
  if(m_InputValues->UseConditional)
  {
    const auto& conditionalArray = m_DataStructure.getDataRefAs<IDataArray>(m_InputValues->ConditionalArrayPath);
    return DispatchAlgorithm<ConditionalSetValueDirect, ConditionalSetValueScanline>({&targetArray, &conditionalArray}, m_DataStructure, m_ShouldCancel, m_InputValues);
  }

  return DispatchAlgorithm<ConditionalSetValueDirect, ConditionalSetValueScanline>({&targetArray}, m_DataStructure, m_ShouldCancel, m_InputValues);
}
