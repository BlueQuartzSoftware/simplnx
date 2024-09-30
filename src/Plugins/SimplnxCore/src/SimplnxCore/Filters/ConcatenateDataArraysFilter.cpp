#include "ConcatenateDataArraysFilter.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/ConcatenateDataArrays.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ConcatenateDataArraysFilter::name() const
{
  return FilterTraits<ConcatenateDataArraysFilter>::name;
}

//------------------------------------------------------------------------------
std::string ConcatenateDataArraysFilter::className() const
{
  return FilterTraits<ConcatenateDataArraysFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConcatenateDataArraysFilter::uuid() const
{
  return FilterTraits<ConcatenateDataArraysFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConcatenateDataArraysFilter::humanName() const
{
  return "Concatenate Data Arrays";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConcatenateDataArraysFilter::defaultTags() const
{
  return {className(), "Concatenate", "Data Array", "Join", "Make", "Append", "Push", "Pushback"};
}

//------------------------------------------------------------------------------
Parameters ConcatenateDataArraysFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InputArrays_Key, "Arrays To Concatenate",
                                                               "Select the arrays that will be concatenated together.  The arrays will be concatenated in the order they are listed here.",
                                                               std::vector<DataPath>{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::Any}, GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArray_Key, "Output Array", "The output array that contains the concatenated arrays.", DataPath({"Concatenated Array"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ConcatenateDataArraysFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConcatenateDataArraysFilter::clone() const
{
  return std::make_unique<ConcatenateDataArraysFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConcatenateDataArraysFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto inputArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputArrays_Key);
  auto outputArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_OutputArray_Key);

  if(inputArrayPaths.empty())
  {
    return MakePreflightErrorResult(to_underlying(ConcatenateDataArrays::ErrorCodes::EmptyInputArrays), "No input arrays have been selected.  Please select at least 2 input arrays.");
  }

  if(inputArrayPaths.size() == 1)
  {
    return MakePreflightErrorResult(to_underlying(ConcatenateDataArrays::ErrorCodes::OneInputArray), "Only one input array has been selected.  Please select at least 2 input arrays.");
  }

  // Check for unequal array types, data types, and component dimensions
  std::vector<usize> cDims;
  IArray::ArrayType arrayType;
  std::string arrayTypeName;
  usize numTuples = 0;
  for(usize i = 0; i < inputArrayPaths.size(); ++i)
  {
    const auto& inputDataArray = dataStructure.getDataRefAs<IArray>(inputArrayPaths[i]);
    for(usize j = i + 1; j < inputArrayPaths.size(); ++j)
    {
      const auto& inputDataArray2 = dataStructure.getDataRefAs<IArray>(inputArrayPaths[j]);

      if(inputDataArray.getTypeName() != inputDataArray2.getTypeName())
      {
        return MakePreflightErrorResult(to_underlying(ConcatenateDataArrays::ErrorCodes::TypeNameMismatch),
                                        fmt::format("Input array '{}' has array type '{}', but input array '{}' has array type '{}'.  The array types must match.", inputArrayPaths[i].toString(),
                                                    inputDataArray.getTypeName(), inputArrayPaths[j].toString(), inputDataArray2.getTypeName()));
      }

      if(inputDataArray.getComponentShape() != inputDataArray2.getComponentShape())
      {
        return MakePreflightErrorResult(to_underlying(ConcatenateDataArrays::ErrorCodes::ComponentShapeMismatch),
                                        fmt::format("Input array '{}' has component shape '{}', but input array '{}' has component shape '{}'.  The component shapes must match.",
                                                    inputArrayPaths[i].toString(), fmt::join(inputDataArray.getComponentShape(), ","), inputArrayPaths[j].toString(),
                                                    fmt::join(inputDataArray2.getComponentShape(), ",")));
      }

      cDims = inputDataArray.getComponentShape();
      arrayType = inputDataArray.getArrayType();
      arrayTypeName = inputDataArray.getTypeName();
    }

    auto tupleShape = inputDataArray.getTupleShape();
    numTuples += std::accumulate(tupleShape.begin(), tupleShape.end(), static_cast<usize>(1), std::multiplies<>());
  }

  std::vector<usize> tDims = {numTuples};

  // Create the output array
  nx::core::Result<OutputActions> resultOutputActions;

  switch(arrayType)
  {
  case IArray::ArrayType::DataArray: {
    const auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPaths[0]);
    auto action = std::make_unique<CreateArrayAction>(inputDataArray.getDataType(), tDims, cDims, outputArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
    break;
  }
  case IArray::ArrayType::StringArray: {
    auto action = std::make_unique<CreateStringArrayAction>(tDims, outputArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
    break;
  }
  case IArray::ArrayType::NeighborListArray: {
    const auto& inputNeighborList = dataStructure.getDataRefAs<INeighborList>(inputArrayPaths[0]);
    auto action = std::make_unique<CreateNeighborListAction>(inputNeighborList.getDataType(), numTuples, outputArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
    break;
  }
  case IArray::ArrayType::Any: {
    return MakePreflightErrorResult(to_underlying(ConcatenateDataArrays::ErrorCodes::InputArraysEqualAny),
                                    "Every array in the input arrays list has array type 'Any'.  This SHOULD NOT be possible, so please contact the developers.");
  }
  default:
    return MakePreflightErrorResult(
        to_underlying(ConcatenateDataArrays::ErrorCodes::InputArraysUnsupported),
        fmt::format("Every array in the input arrays list has array type '{}'.  This is an array type that is currently not supported by this filter, so please contact the developers.",
                    arrayTypeName));
  }

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ConcatenateDataArraysFilter::executeImpl(DataStructure& dataStructure, const Arguments& args, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ConcatenateDataArraysInputValues inputValues;

  inputValues.InputArrayPaths = args.value<MultiArraySelectionParameter::ValueType>(k_InputArrays_Key);
  inputValues.OutputArrayPath = args.value<ArrayCreationParameter::ValueType>(k_OutputArray_Key);

  return ConcatenateDataArrays(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
