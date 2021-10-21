#include "ITKProxTVImage.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKProxTVImage::name() const
{
  return FilterTraits<ITKProxTVImage>::name.str();
}

//------------------------------------------------------------------------------
std::string ITKProxTVImage::className() const
{
  return FilterTraits<ITKProxTVImage>::className;
}

//------------------------------------------------------------------------------
Uuid ITKProxTVImage::uuid() const
{
  return FilterTraits<ITKProxTVImage>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKProxTVImage::humanName() const
{
  return "ITK::Prox T V Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKProxTVImage::defaultTags() const
{
  return {"#ITK Image Processing", "#ITK NoModule"};
}

//------------------------------------------------------------------------------
Parameters ITKProxTVImage::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Float64Parameter>(k_MaximumNumberOfIterations_Key, "MaximumNumberOfIterations", "", 2.3456789));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Weights_Key, "Weights", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Norms_Key, "Norms", "", std::vector<float32>(3), std::vector<std::string>(3)));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_SelectedCellArrayPath_Key, "Attribute Array to filter", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<StringParameter>(k_NewCellArrayName_Key, "Filtered Array", "", "SomeString"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKProxTVImage::clone() const
{
  return std::make_unique<ITKProxTVImage>();
}

//------------------------------------------------------------------------------
Result<OutputActions> ITKProxTVImage::preflightImpl(const DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Write any preflight sanity checking codes in this function
   ***************************************************************************/
  auto pMaximumNumberOfIterationsValue = filterArgs.value<float64>(k_MaximumNumberOfIterations_Key);
  auto pWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Weights_Key);
  auto pNormsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Norms_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  OutputActions actions;
#if 0
  // Define a custom class that generates the changes to the DataStructure.
  auto action = std::make_unique<ITKProxTVImageAction>();
  actions.actions.push_back(std::move(action));
#endif
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ITKProxTVImage::executeImpl(DataStructure& ds, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  /****************************************************************************
   * Extract the actual input values from the 'filterArgs' object
   ***************************************************************************/
  auto pMaximumNumberOfIterationsValue = filterArgs.value<float64>(k_MaximumNumberOfIterations_Key);
  auto pWeightsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Weights_Key);
  auto pNormsValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Norms_Key);
  auto pSelectedCellArrayPathValue = filterArgs.value<DataPath>(k_SelectedCellArrayPath_Key);
  auto pNewCellArrayNameValue = filterArgs.value<StringParameter::ValueType>(k_NewCellArrayName_Key);

  /****************************************************************************
   * Write your algorithm implementation in this function
   ***************************************************************************/

  return {};
}
} // namespace complex
