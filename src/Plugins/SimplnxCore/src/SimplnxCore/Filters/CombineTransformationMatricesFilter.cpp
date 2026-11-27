#include "CombineTransformationMatricesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/CombineTransformationMatrices.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string CombineTransformationMatricesFilter::name() const
{
  return FilterTraits<CombineTransformationMatricesFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string CombineTransformationMatricesFilter::className() const
{
  return FilterTraits<CombineTransformationMatricesFilter>::className;
}

//------------------------------------------------------------------------------
Uuid CombineTransformationMatricesFilter::uuid() const
{
  return FilterTraits<CombineTransformationMatricesFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string CombineTransformationMatricesFilter::humanName() const
{
  return "Combine Transformation Matrices";
}

//------------------------------------------------------------------------------
std::vector<std::string> CombineTransformationMatricesFilter::defaultTags() const
{
  return {className(), "Matrix", "Calculate", "Multiplication"};
}

//------------------------------------------------------------------------------
Parameters CombineTransformationMatricesFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InputArrays_Key, "Input Matrices", "The list of Attribute Arrays that represent Square Matrices of all the same dimensions",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedArrayTypes{IArray::ArrayType::DataArray},
                                                               MultiArraySelectionParameter::AllowedDataTypesType{DataType::float32}));

  params.insertSeparator(Parameters::Separator{"Output Parameters"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputArray_Key, "Output Array", "The output array that contains the output from the operations.", DataPath({""})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType CombineTransformationMatricesFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType CombineTransformationMatricesFilter::clone() const
{
  return std::make_unique<CombineTransformationMatricesFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult CombineTransformationMatricesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                            const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto inputArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputArrays_Key);
  auto outputArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_OutputArray_Key);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(inputArrayPaths.empty())
  {
    return MakePreflightErrorResult(to_underlying(CombineTransformationMatrices::ErrorCodes::EmptyInputArrays), "No input arrays have been selected.  Please select at least 2 input arrays.");
  }

  if(inputArrayPaths.size() == 1)
  {
    return MakePreflightErrorResult(to_underlying(CombineTransformationMatrices::ErrorCodes::OneInputArray), "Only one input array has been selected.  Please select at least 2 input arrays.");
  }

  for(usize i = 0; i < inputArrayPaths.size(); ++i)
  {
    const auto& inputDataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPaths[i]);

    auto totalElements = inputDataArray.getNumberOfTuples() * inputDataArray.getNumberOfComponents();
    if(totalElements != 16)
    {
      return {.output_actions =
                  MakeErrorResult<OutputActions>(to_underlying(CombineTransformationMatrices::ErrorCodes::WrongElementCount),
                                                 fmt::format("Input transformation matrix at path '{}' has {} total elements ({} tuples * {} components), but it MUST have 16 total elements.",
                                                             inputArrayPaths[i].toString(), totalElements, inputDataArray.getNumberOfTuples(), inputDataArray.getNumberOfComponents()))};
    }

    if(i + 1 < inputArrayPaths.size() && inputDataArray.getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(
          to_underlying(CombineTransformationMatrices::ErrorCodes::ComponentShapeMismatch),
          fmt::format("Input array '{}' has component shape '{}'. Input arrays must only have a single component.", inputArrayPaths[i].toString(), fmt::join(inputDataArray.getComponentShape(), ",")));
    }
  }

  // create the destination array for the calculated results
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, ShapeType{4, 4}, ShapeType{1}, outputArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> CombineTransformationMatricesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  CombineTransformationMatricesInputValues inputValues;
  inputValues.selected_paths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_InputArrays_Key);
  inputValues.output_path = filterArgs.value<ArrayCreationParameter::ValueType>(k_OutputArray_Key);

  return CombineTransformationMatrices(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
