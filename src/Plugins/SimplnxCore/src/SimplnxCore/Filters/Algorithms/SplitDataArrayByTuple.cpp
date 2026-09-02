#include "SplitDataArrayByTuple.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ParallelTaskAlgorithm.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

using namespace nx::core;

namespace
{
template <typename ArrayType>
class SplitDataArrayByTupleImpl
{
public:
  SplitDataArrayByTupleImpl(const ArrayType& inputArray, ArrayType& outputArray, const std::vector<usize> inputTupleShapeOffsets, const std::atomic_bool& shouldCancel)
  : m_InputArray(inputArray)
  , m_OutputArray(outputArray)
  , m_InputTupleShapeOffsets(inputTupleShapeOffsets)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~SplitDataArrayByTupleImpl() = default;

  SplitDataArrayByTupleImpl(const SplitDataArrayByTupleImpl&) = default;
  SplitDataArrayByTupleImpl(SplitDataArrayByTupleImpl&&) noexcept = default;
  SplitDataArrayByTupleImpl& operator=(const SplitDataArrayByTupleImpl&) = delete;
  SplitDataArrayByTupleImpl& operator=(SplitDataArrayByTupleImpl&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    auto inputTupleShape = m_InputArray.getTupleShape();
    auto outputTupleShape = m_OutputArray.getTupleShape();
    const std::vector<usize> startOutputTupleOffsets(inputTupleShape.size(), 0);
    CopyFromArray::CopyDataND(m_InputArray, m_OutputArray, m_InputTupleShapeOffsets, startOutputTupleOffsets, outputTupleShape);
  }

private:
  const ArrayType& m_InputArray;
  ArrayType& m_OutputArray;
  const std::vector<usize> m_InputTupleShapeOffsets;
  const std::atomic_bool& m_ShouldCancel;
};

template <typename T>
class SplitNeighborListByTupleImpl
{
public:
  SplitNeighborListByTupleImpl(const NeighborList<T>& inputNL, NeighborList<T>& outputNL, usize inputTupleOffset, const std::atomic_bool& shouldCancel)
  : m_InputNL(inputNL)
  , m_OutputNL(outputNL)
  , m_InputTupleOffset(inputTupleOffset)
  , m_ShouldCancel(shouldCancel)
  {
  }

  ~SplitNeighborListByTupleImpl() = default;

  SplitNeighborListByTupleImpl(const SplitNeighborListByTupleImpl&) = default;
  SplitNeighborListByTupleImpl(SplitNeighborListByTupleImpl&&) noexcept = default;
  SplitNeighborListByTupleImpl& operator=(const SplitNeighborListByTupleImpl&) = delete;
  SplitNeighborListByTupleImpl& operator=(SplitNeighborListByTupleImpl&&) noexcept = delete;

  void operator()() const
  {
    convert();
  }

protected:
  void convert() const
  {
    auto outputTupleShape = m_OutputNL.getTupleShape();
    usize startOutputOffset = 0;
    CopyFromArray::CopyDataND(m_InputNL, m_OutputNL, {m_InputTupleOffset}, {startOutputOffset}, outputTupleShape);
  }

private:
  const NeighborList<T>& m_InputNL;
  NeighborList<T>& m_OutputNL;
  usize m_InputTupleOffset;
  const std::atomic_bool& m_ShouldCancel;
};

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
typename std::enable_if<is_allowed_array_type<ArrayType>::value, Result<>>::type SplitArraysByTupleImpl(DataStructure& dataStructure, const DataPath& inputArrayPath,
                                                                                                        const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                                                                                                        const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  // The actual splitting of the dataStructure array is done in parallel where parallel here
  // refers to the splitting of the DataArray into each output array being done on a separate thread.
  ParallelTaskAlgorithm taskRunner;
  auto& inputArray = dataStructure.getDataRefAs<ArrayType>(inputArrayPath);
  auto inputTupleShape = inputArray.getTupleShape();
  std::vector<usize> inputTupleShapeOffset(inputTupleShape.size(), 0);
  for(usize i = 0; i < outputArrayPaths.size(); ++i)
  {
    if(shouldCancel)
    {
      return {};
    }

    auto& outputArray = dataStructure.getDataRefAs<ArrayType>(outputArrayPaths[i]);

    messageHandler.sendInfoMessage(fmt::format("Splitting data array '{}' by tuple ({}/{})", inputArrayPath.toString(), i + 1, outputArrayPaths.size()));

    // Run this directly since ArrayType is the template parameter
    taskRunner.execute(SplitDataArrayByTupleImpl<ArrayType>(inputArray, outputArray, inputTupleShapeOffset, shouldCancel));

    inputTupleShapeOffset[splitDimension] += outputArray.getTupleShape()[splitDimension];
  }
  taskRunner.wait(); // This will spill over if the number of DataArrays to process does not divide evenly by the number of threads.

  return {};
}

template <typename T>
Result<> SplitNeighborListsByTupleImpl(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, const IFilter::MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel)
{
  ParallelTaskAlgorithm taskRunner;
  auto& inputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(inputArrayPath);

  usize inputTupleOffset = 0;
  for(usize i = 0; i < outputArrayPaths.size(); ++i)
  {
    if(shouldCancel)
    {
      return {};
    }

    messageHandler.sendInfoMessage(fmt::format("Splitting neighbor list '{}' by tuple ({}/{})", inputArrayPath.toString(), i + 1, outputArrayPaths.size()));

    auto& outputNeighborList = dataStructure.getDataRefAs<NeighborList<T>>(outputArrayPaths[i]);
    taskRunner.execute(SplitNeighborListByTupleImpl(inputNeighborList, outputNeighborList, inputTupleOffset, shouldCancel));
    inputTupleOffset += outputNeighborList.getNumberOfTuples();
  }

  return {};
}

struct SplitDataArraysTemplateImpl
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension, const IFilter::MessageHandler& messageHandler,
                  const std::atomic_bool& shouldCancel, Result<>& result)
  {
    result = SplitArraysByTupleImpl<DataArray<T>>(dataStructure, inputArrayPath, outputArrayPaths, splitDimension, messageHandler, shouldCancel);
  }
};

struct SplitNeighborListsTemplateImpl
{
  template <typename T>
  void operator()(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, const IFilter::MessageHandler& messageHandler,
                  const std::atomic_bool& shouldCancel, Result<>& result)
  {
    result = SplitNeighborListsByTupleImpl<T>(dataStructure, inputArrayPath, outputArrayPaths, messageHandler, shouldCancel);
  }
};

Result<> SplitArraysByTuple(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, usize splitDimension,
                            const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  const auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  Result<> result;
  ExecuteDataFunction(SplitDataArraysTemplateImpl{}, inputDataArray.getDataType(), dataStructure, inputArrayPath, outputArrayPaths, splitDimension, messageHandler, shouldCancel, result);
  return result;
}

Result<> SplitNeighborLists(DataStructure& dataStructure, const DataPath& inputArrayPath, const std::vector<DataPath>& outputArrayPaths, const IFilter::MessageHandler& messageHandler,
                            const std::atomic_bool& shouldCancel)
{
  const auto& inputNeighborList = dataStructure.getDataRefAs<INeighborList>(inputArrayPath);
  Result<> result;
  ExecuteNeighborFunction(SplitNeighborListsTemplateImpl{}, inputNeighborList.getDataType(), dataStructure, inputArrayPath, outputArrayPaths, messageHandler, shouldCancel, result);
  return result;
}
} // namespace

// -----------------------------------------------------------------------------
SplitDataArrayByTuple::SplitDataArrayByTuple(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                             SplitDataArrayByTupleInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
SplitDataArrayByTuple::~SplitDataArrayByTuple() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& SplitDataArrayByTuple::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> SplitDataArrayByTuple::operator()()
{
  const auto& inputDataArray = m_DataStructure.getDataRefAs<IArray>(m_InputValues->InputArrayPath);
  std::string arrayTypeName = inputDataArray.getTypeName();
  switch(inputDataArray.getArrayType())
  {
  case IArray::ArrayType::DataArray: {
    return SplitArraysByTuple(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPaths, m_InputValues->SplitDimension, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::StringArray: {
    return SplitArraysByTupleImpl<StringArray>(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPaths, m_InputValues->SplitDimension, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::NeighborListArray: {
    return SplitNeighborLists(m_DataStructure, m_InputValues->InputArrayPath, m_InputValues->OutputArrayPaths, m_MessageHandler, m_ShouldCancel);
  }
  case IArray::ArrayType::Any: {
    return MakeErrorResult(to_underlying(SplitDataArrayByTuple::ErrorCodes::AnyArrayType),
                           fmt::format("The input array '{}' has array type 'Any'.  This SHOULD NOT be possible, so please contact the developers.", m_InputValues->InputArrayPath.toString()));
  }
  default: {
    return MakeErrorResult(
        to_underlying(SplitDataArrayByTuple::ErrorCodes::UnsupportedArrayType),
        fmt::format("The input array '{}' has an array type that is currently not supported by this filter, so please contact the developers.", m_InputValues->InputArrayPath.toString()));
  }
  }
}
