#include "GenerateVectorColorsFilter.hpp"

#include "ComplexCore/Filters/Algorithms/GenerateVectorColors.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"

using namespace complex;

namespace
{
const DataPath k_MaskArrayPath = DataPath({"mask array"});
}

namespace complex
{
//------------------------------------------------------------------------------
std::string GenerateVectorColorsFilter::name() const
{
  return FilterTraits<GenerateVectorColorsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string GenerateVectorColorsFilter::className() const
{
  return FilterTraits<GenerateVectorColorsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid GenerateVectorColorsFilter::uuid() const
{
  return FilterTraits<GenerateVectorColorsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string GenerateVectorColorsFilter::humanName() const
{
  return "Generate Vector Colors";
}

//------------------------------------------------------------------------------
std::vector<std::string> GenerateVectorColorsFilter::defaultTags() const
{
  return {"Generic", "Coloring"};
}

//------------------------------------------------------------------------------
Parameters GenerateVectorColorsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_UseGoodVoxels_Key, "Apply to Good Voxels Only (Bad Voxels Will Be Black)", "Whether or not to assign colors to bad voxels or leave them black", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_GoodVoxelsArrayPath_Key, "Mask", "Used to define Elements as good or bad ", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_VectorsArrayPath_Key, "Vector Attribute Array", "Vectors the colors will represent", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  params.insertSeparator(Parameters::Separator{"Created Element Data Object"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellVectorColorsArrayName_Key, "Vector Colors", "RGB colors", "Vector Colors Array"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseGoodVoxels_Key, k_GoodVoxelsArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer GenerateVectorColorsFilter::clone() const
{
  return std::make_unique<GenerateVectorColorsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult GenerateVectorColorsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pUseGoodVoxelsValue = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  auto pVectorsArrayPathValue = filterArgs.value<DataPath>(k_VectorsArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  auto pCellVectorColorsArrayNameValue = filterArgs.value<std::string>(k_CellVectorColorsArrayName_Key);

  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<usize> vectorsTupShape = dataStructure.getDataAs<Float32Array>(pVectorsArrayPathValue)->getTupleShape();
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::uint8, vectorsTupShape, std::vector<usize>{3}, pVectorsArrayPathValue.getParent().createChildPath(pCellVectorColorsArrayNameValue));
    resultOutputActions.value().appendAction(std::move(action));
  }

  if(!pUseGoodVoxelsValue)
  {
    auto action = std::make_unique<CreateArrayAction>(DataType::boolean, vectorsTupShape, std::vector<usize>{1}, k_MaskArrayPath);
    resultOutputActions.value().appendAction(std::move(action));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> GenerateVectorColorsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  GenerateVectorColorsInputValues inputValues;

  inputValues.UseGoodVoxels = filterArgs.value<bool>(k_UseGoodVoxels_Key);
  inputValues.VectorsArrayPath = filterArgs.value<DataPath>(k_VectorsArrayPath_Key);
  if(!inputValues.UseGoodVoxels)
  {
    inputValues.GoodVoxelsArrayPath = k_MaskArrayPath;
    dataStructure.getDataAs<BoolArray>(k_MaskArrayPath)->fill(true);
  }
  else
  {
    inputValues.GoodVoxelsArrayPath = filterArgs.value<DataPath>(k_GoodVoxelsArrayPath_Key);
  }
  inputValues.CellVectorColorsArrayPath = inputValues.VectorsArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_CellVectorColorsArrayName_Key));

  return GenerateVectorColors(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
