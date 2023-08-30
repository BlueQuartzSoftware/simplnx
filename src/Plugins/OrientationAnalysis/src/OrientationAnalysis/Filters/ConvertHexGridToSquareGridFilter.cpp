#include "ConvertHexGridToSquareGridFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ConvertHexGridToSquareGrid.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::name() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::className() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ConvertHexGridToSquareGridFilter::uuid() const
{
  return FilterTraits<ConvertHexGridToSquareGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ConvertHexGridToSquareGridFilter::humanName() const
{
  return "Convert Hexagonal Grid Data to Square Grid Data (TSL - .ang)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ConvertHexGridToSquareGridFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion"};
}

//------------------------------------------------------------------------------
Parameters ConvertHexGridToSquareGridFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ConversionType_Key, "Conversion Type", "", 0, {}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellQuatsArrayPath_Key, "Input Quaternions", "Specifies the quaternions to convert", DataPath({"CellData", "Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputDataArrayPath_Key, "Output Quaternions", "", "Quaternions [Converted]"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ConvertHexGridToSquareGridFilter::clone() const
{
  return std::make_unique<ConvertHexGridToSquareGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ConvertHexGridToSquareGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                const std::atomic_bool& shouldCancel) const
{
  auto pQuaternionDataArrayPathValue = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  auto pOutputDataArrayPathValue = pQuaternionDataArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));

  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ConvertHexGridToSquareGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                              const std::atomic_bool& shouldCancel) const
{
  ConvertHexGridToSquareGridInputValues inputValues;

  inputValues.QuaternionDataArrayPath = filterArgs.value<DataPath>(k_CellQuatsArrayPath_Key);
  inputValues.OutputDataArrayPath = inputValues.QuaternionDataArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_OutputDataArrayPath_Key));
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);
  inputValues.ConversionType = filterArgs.value<ChoicesParameter::ValueType>(k_ConversionType_Key);

  return ConvertHexGridToSquareGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
