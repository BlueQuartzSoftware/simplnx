#include "ReshapeDataArrayFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReshapeDataArray.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/DataStructure/INeighborList.hpp"
#include "simplnx/DataStructure/StringArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Filter/Actions/RenameDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReshapeDataArrayFilter::name() const
{
  return FilterTraits<ReshapeDataArrayFilter>::name;
}

//------------------------------------------------------------------------------
std::string ReshapeDataArrayFilter::className() const
{
  return FilterTraits<ReshapeDataArrayFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReshapeDataArrayFilter::uuid() const
{
  return FilterTraits<ReshapeDataArrayFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReshapeDataArrayFilter::humanName() const
{
  return "Reshape Data Array";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReshapeDataArrayFilter::defaultTags() const
{
  return {className(), "Create", "Data Structure", "Data Array", "Reshape", "transform", "morph", "adjust", "change", "modify", "resize", "tuple"};
}

//------------------------------------------------------------------------------
Parameters ReshapeDataArrayFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_Input_Array_Key, "Input Array", "The input array that will be reshaped.", DataPath({"Data"}), GetAllDataTypes()));

  {
    DynamicTableInfo tableInfo;
    tableInfo.setRowsInfo(DynamicTableInfo::StaticVectorInfo(1));
    tableInfo.setColsInfo(DynamicTableInfo::DynamicVectorInfo(1, "DIM {}"));

    params.insert(std::make_unique<DynamicTableParameter>(k_TupleDims_Key, "New Tuple Dimensions (Slowest to Fastest Dimensions)",
                                                          "Slowest to Fastest Dimensions. Note this might be opposite displayed by an image geometry.", tableInfo));
  }

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReshapeDataArrayFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReshapeDataArrayFilter::clone() const
{
  return std::make_unique<ReshapeDataArrayFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReshapeDataArrayFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto inputArrayPath = filterArgs.value<DataPath>(k_Input_Array_Key);
  auto tupleDimsData = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<usize> tDims = {};
  const auto& rowData = tupleDimsData.at(0);
  tDims.reserve(rowData.size());
  for(usize i = 0; i < rowData.size(); ++i)
  {
    auto floatValue = rowData[i];
    if(floatValue == 0)
    {
      return MakePreflightErrorResult(to_underlying(ReshapeDataArray::ErrorCodes::NonPositiveTupleDimValue),
                                      fmt::format("Tuple dimension at index {0} cannot have a value of zero.  Please input a positive integer at index {0}.", i));
    }

    tDims.push_back(static_cast<usize>(floatValue));
  }

  auto& inputArray = dataStructure.getDataRefAs<IArray>(inputArrayPath);
  auto inputArrayTupleShape = inputArray.getTupleShape();
  if(inputArrayTupleShape == tDims)
  {
    return MakePreflightErrorResult(to_underlying(ReshapeDataArray::ErrorCodes::TupleShapesEqual),
                                    fmt::format("The chosen tuple shape [{}] is the same shape as the input array's tuple shape [{}].  Please choose a differing tuple shape.", fmt::join(tDims, ","),
                                                fmt::join(inputArrayTupleShape, ",")));
  }

  // Reshape the array
  auto inputArrayType = inputArray.getArrayType();
  auto outputArrayPath = inputArrayPath.getParent().createChildPath(fmt::format(".{}", inputArrayPath.getTargetName()));
  switch(inputArrayType)
  {
  case IArray::ArrayType::DataArray: {
    auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
    resultOutputActions.value().appendAction(
        std::make_unique<CreateArrayAction>(inputDataArray.getDataType(), tDims, inputDataArray.getComponentShape(), outputArrayPath, inputDataArray.getDataFormat()));
    break;
  }
  case IArray::ArrayType::NeighborListArray: {
    auto& inputNeighborList = dataStructure.getDataRefAs<INeighborList>(inputArrayPath);
    resultOutputActions.value().appendAction(std::make_unique<CreateNeighborListAction>(inputNeighborList.getDataType(), tDims, outputArrayPath));
    break;
  }
  case IArray::ArrayType::StringArray: {
    resultOutputActions.value().appendAction(std::make_unique<CreateStringArrayAction>(tDims, outputArrayPath));
    break;
  }
  case IArray::ArrayType::Any: {
    return MakePreflightErrorResult(to_underlying(ReshapeDataArray::ErrorCodes::InputArrayEqualsAny),
                                    fmt::format("Input array '{}' has array type 'Any'.  Something has gone horribly wrong, please contact the developers.", inputArray.getName()));
  }
  default: {
    return MakePreflightErrorResult(to_underlying(ReshapeDataArray::ErrorCodes::InputArrayUnsupported),
                                    fmt::format("Input array '{}' has array type '{}'.  This array type is not currently supported by this filter, please contact the developers.",
                                                inputArray.getName(), inputArray.getTypeName()));
  }
  }

  resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(inputArrayPath));
  resultOutputActions.value().appendDeferredAction(std::make_unique<RenameDataAction>(outputArrayPath, inputArrayPath.getTargetName()));

  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ReshapeDataArrayFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReshapeDataArrayInputValues inputValues;
  inputValues.InputArrayPath = filterArgs.value<DataPath>(k_Input_Array_Key);
  inputValues.TupleDims = filterArgs.value<DynamicTableParameter::ValueType>(k_TupleDims_Key);

  return ReshapeDataArray(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
