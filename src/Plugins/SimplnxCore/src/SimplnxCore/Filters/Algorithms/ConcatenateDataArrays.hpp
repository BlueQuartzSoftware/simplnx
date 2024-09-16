#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

namespace nx::core
{
template <typename T>
struct is_allowed_array_type : std::false_type
{
};

template <typename T>
struct is_allowed_array_type<DataArray<T>> : std::true_type
{
};

template <>
struct is_allowed_array_type<StringArray> : std::true_type
{
};

template <typename ArrayType>
typename std::enable_if<is_allowed_array_type<ArrayType>::value, Result<>>::type ConcatenateArraysImpl(DataStructure& dataStructure, const std::vector<DataPath>& inputArrayPaths,
                                                                                                       const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler,
                                                                                                       const std::atomic_bool& shouldCancel)
{
  auto& outputDataArray = dataStructure.getDataRefAs<ArrayType>(outputArrayPath);
  usize destTupleOffset = 0;
  for(const auto& inputArrayPath : inputArrayPaths)
  {
    if(shouldCancel)
    {
      return {};
    }

    messageHandler({IFilter::Message::Type::Info, fmt::format("Concatenating array '{}'...", inputArrayPath.toString())});
    const auto& inputDataArray = dataStructure.getDataRefAs<ArrayType>(inputArrayPath);
    auto result = CopyFromArray::CopyData(inputDataArray, outputDataArray, destTupleOffset, 0, inputDataArray.getNumberOfTuples());
    if(result.invalid())
    {
      return result;
    }
    destTupleOffset += inputDataArray.getNumberOfTuples();
  }

  return {};
}

template <typename T>
Result<> ConcatenateNeighborListsImpl(DataStructure& dataStructure, const std::vector<DataPath>& inputArrayPaths, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler,
                                      const std::atomic_bool& shouldCancel)
{
  auto& outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(outputArrayPath);
  int32 currentOutputTuple = 0;
  for(const auto& inputNeighborListPath : inputArrayPaths)
  {
    if(shouldCancel)
    {
      return {};
    }

    messageHandler({IFilter::Message::Type::Info, fmt::format("Concatenating neighbor list '{}'...", inputNeighborListPath.toString())});
    const auto& inputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(inputNeighborListPath);
    for(int32 listIdx = 0; listIdx < inputNeighborList.getNumberOfLists(); ++listIdx)
    {
      outputNeighborList.setList(currentOutputTuple, inputNeighborList.getList(listIdx));
      currentOutputTuple++;
    }
  }

  return {};
}

struct SIMPLNXCORE_EXPORT ConcatenateDataArraysInputValues
{
  std::vector<DataPath> InputArrayPaths;
  DataPath OutputArrayPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ConcatenateDataArrays
{
public:
  ConcatenateDataArrays(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ConcatenateDataArraysInputValues* inputValues);
  ~ConcatenateDataArrays() noexcept;

  ConcatenateDataArrays(const ConcatenateDataArrays&) = delete;
  ConcatenateDataArrays(ConcatenateDataArrays&&) noexcept = delete;
  ConcatenateDataArrays& operator=(const ConcatenateDataArrays&) = delete;
  ConcatenateDataArrays& operator=(ConcatenateDataArrays&&) noexcept = delete;

  // Error Codes
  enum class ErrorCodes : int32
  {
    EmptyInputArrays = -2300,
    OneInputArray = -2301,
    NonPositiveTupleDimValue = -2302,
    TypeNameMismatch = -2303,
    ComponentShapeMismatch = -2304,
    TotalTuplesMismatch = -2305,
    InputArraysEqualAny = -2306,
    InputArraysUnsupported = -2307
  };

  // Warning Codes
  enum class WarningCodes : int32
  {
    MultipleTupleDimsNotSupported = -100
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConcatenateDataArraysInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  struct ConcatenateDataArraysTemplateImpl
  {
    template <typename T>
    void operator()(DataStructure& dataStructure, const std::vector<DataPath>& inputArrayPaths, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler,
                    const std::atomic_bool& shouldCancel, Result<>& result)
    {
      result = ConcatenateArraysImpl<DataArray<T>>(dataStructure, inputArrayPaths, outputArrayPath, messageHandler, shouldCancel);
    }
  };

  struct ConcatenateNeighborListsTemplateImpl
  {
    template <typename T>
    void operator()(DataStructure& dataStructure, const std::vector<DataPath>& inputArrayPaths, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler,
                    const std::atomic_bool& shouldCancel, Result<>& result)
    {
      result = ConcatenateNeighborListsImpl<T>(dataStructure, inputArrayPaths, outputArrayPath, messageHandler, shouldCancel);
    }
  };

  static Result<> ConcatenateArrays(DataStructure& dataStructure, const std::vector<DataPath>& inputArrayPaths, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler,
                                    const std::atomic_bool& shouldCancel)
  {
    const auto& outputDataArray = dataStructure.getDataRefAs<IDataArray>(outputArrayPath);
    Result<> result;
    ExecuteDataFunction(ConcatenateDataArraysTemplateImpl{}, outputDataArray.getDataType(), dataStructure, inputArrayPaths, outputArrayPath, messageHandler, shouldCancel, result);
    return result;
  }

  static Result<> ConcatenateNeighborLists(DataStructure& dataStructure, const std::vector<DataPath>& inputArrayPaths, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel)
  {
    const auto& outputNeighborList = dataStructure.getDataRefAs<INeighborList>(outputArrayPath);
    Result<> result;
    ExecuteNeighborFunction(ConcatenateNeighborListsTemplateImpl{}, outputNeighborList.getDataType(), dataStructure, inputArrayPaths, outputArrayPath, messageHandler, shouldCancel, result);
    return result;
  }
};

} // namespace nx::core
