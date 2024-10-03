#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
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
typename std::enable_if<is_allowed_array_type<ArrayType>::value, Result<>>::type ReshapeArrayImpl(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath,
                                                                                                  const IFilter::MessageHandler& messageHandler)
{
  auto& inputDataArray = dataStructure.getDataRefAs<ArrayType>(inputArrayPath);
  auto& outputDataArray = dataStructure.getDataRefAs<ArrayType>(outputArrayPath);
  messageHandler({IFilter::Message::Type::Info, fmt::format("Reshaping data array '{}' from [{}] to [{}]...", inputArrayPath.toString(), fmt::join(inputDataArray.getTupleShape(), ","),
                                                            fmt::join(outputDataArray.getTupleShape(), ","))});
  usize tuplesToCopy = std::min(inputDataArray.getNumberOfTuples(), outputDataArray.getNumberOfTuples());
  auto result = CopyFromArray::CopyData(inputDataArray, outputDataArray, 0, 0, tuplesToCopy);
  if(result.invalid())
  {
    return result;
  }

  return {};
}

template <typename T>
Result<> ReshapeNeighborListImpl(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler)
{
  const auto& inputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath);
  auto& outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(outputArrayPath);

  messageHandler({IFilter::Message::Type::Info, fmt::format("Reshaping neighbor list '{}' from [{}] to [{}]...", inputArrayPath.toString(), fmt::join(inputNeighborList.getTupleShape(), ","),
                                                            fmt::join(outputNeighborList.getTupleShape(), ","))});
  for(int32 listIdx = 0; listIdx < outputNeighborList.getNumberOfTuples(); ++listIdx)
  {
    if(listIdx < inputNeighborList.getNumberOfTuples())
    {
      outputNeighborList.setList(listIdx, inputNeighborList.getList(listIdx));
    }
    else
    {
      typename NeighborList<T>::SharedVectorType inputList(new std::vector<T>({}));
      outputNeighborList.setList(listIdx, inputList);
    }
  }

  return {};
}

struct SIMPLNXCORE_EXPORT ReshapeDataArrayInputValues
{
  DataPath InputArrayPath;
  DynamicTableParameter::ValueType TupleDims;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT ReshapeDataArray
{
public:
  ReshapeDataArray(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReshapeDataArrayInputValues* inputValues);
  ~ReshapeDataArray() noexcept;

  ReshapeDataArray(const ReshapeDataArray&) = delete;
  ReshapeDataArray(ReshapeDataArray&&) noexcept = delete;
  ReshapeDataArray& operator=(const ReshapeDataArray&) = delete;
  ReshapeDataArray& operator=(ReshapeDataArray&&) noexcept = delete;

  // Error Codes
  enum class ErrorCodes : int32
  {
    NonPositiveTupleDimValue = -3501,
    InputArrayEqualsAny = -3502,
    InputArrayUnsupported = -3503,
    TupleShapesEqual = -3504
  };

  enum class WarningCodes : int32
  {
    NeighborListMultipleTupleDims = -100,
    StringArrayMultipleTupleDims = -101
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReshapeDataArrayInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  struct ReshapeDataArrayTemplateImpl
  {
    template <typename T>
    void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler, Result<>& result)
    {
      result = ReshapeArrayImpl<DataArray<T>>(dataStructure, inputArrayPath, outputArrayPath, messageHandler);
    }
  };

  struct ReshapeNeighborListTemplateImpl
  {
    template <typename T>
    void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler, Result<>& result)
    {
      result = ReshapeNeighborListImpl<T>(dataStructure, inputArrayPath, outputArrayPath, messageHandler);
    }
  };

  static Result<> ReshapeArray(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler)
  {
    const auto& outputDataArray = dataStructure.getDataRefAs<IDataArray>(outputArrayPath);
    Result<> result;
    ExecuteDataFunction(ReshapeDataArrayTemplateImpl{}, outputDataArray.getDataType(), dataStructure, inputArrayPath, outputArrayPath, messageHandler, result);
    return result;
  }

  static Result<> ReshapeNeighborList(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& outputArrayPath, const IFilter::MessageHandler& messageHandler)
  {
    const auto& outputNeighborList = dataStructure.getDataRefAs<INeighborList>(outputArrayPath);
    Result<> result;
    ExecuteNeighborFunction(ReshapeNeighborListTemplateImpl{}, outputNeighborList.getDataType(), dataStructure, inputArrayPath, outputArrayPath, messageHandler, result);
    return result;
  }
};

} // namespace nx::core
