#include "RodriguesConvertorFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/RodriguesConvertor.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/DeleteDataAction.hpp"
#include "complex/Filter/Actions/EmptyAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"

using namespace complex;
namespace
{
inline constexpr int32 k_IncorrectInputArray = -7300;
inline constexpr int32 k_MissingInputArray = -7301;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string RodriguesConvertorFilter::name() const
{
  return FilterTraits<RodriguesConvertorFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RodriguesConvertorFilter::className() const
{
  return FilterTraits<RodriguesConvertorFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RodriguesConvertorFilter::uuid() const
{
  return FilterTraits<RodriguesConvertorFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RodriguesConvertorFilter::humanName() const
{
  return "Rodrigues Convertor";
}

//------------------------------------------------------------------------------
std::vector<std::string> RodriguesConvertorFilter::defaultTags() const
{
  return {"Processing", "Crystallography", "Convert"};
}

//------------------------------------------------------------------------------
Parameters RodriguesConvertorFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_DeleteOriginalData_Key, "Delete Original Data", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_RodriguesDataArrayPath_Key, "", "Specifies the Rodrigues data to convert", DataPath({"CellData", "rodrigues"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_OutputDataArrayPath_Key, "Converted Rodrigues Data Array", "", DataPath({"rodrigues [Converted]"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RodriguesConvertorFilter::clone() const
{
  return std::make_unique<RodriguesConvertorFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RodriguesConvertorFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                 const std::atomic_bool& shouldCancel) const
{

  auto pRodriguesDataArrayPathValue = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  auto pOutputDataArrayPathValue = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Validate the Rodrigues array
  const auto& quats = dataStructure.getDataRefAs<Float32Array>(pRodriguesDataArrayPathValue);
  if(quats.getNumberOfComponents() != 3)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{k_IncorrectInputArray, "Rodrigues Input Array must be a 3 component Float32 array"}})};
  }

  {
    auto createConvertedQuatAction = std::make_unique<CreateArrayAction>(DataType::float32, quats.getTupleShape(), std::vector<usize>{4}, pOutputDataArrayPathValue);
    resultOutputActions.value().actions.push_back(std::move(createConvertedQuatAction));
  }

  auto pRemoveOriginalArray = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  if(pRemoveOriginalArray)
  {
    resultOutputActions.value().deferredActions.push_back(std::make_unique<DeleteDataAction>(pRodriguesDataArrayPathValue));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RodriguesConvertorFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                               const std::atomic_bool& shouldCancel) const
{
  RodriguesConvertorInputValues inputValues;

  inputValues.RodriguesDataArrayPath = filterArgs.value<DataPath>(k_RodriguesDataArrayPath_Key);
  inputValues.OutputDataArrayPath = filterArgs.value<DataPath>(k_OutputDataArrayPath_Key);
  inputValues.DeleteOriginalData = filterArgs.value<bool>(k_DeleteOriginalData_Key);

  return RodriguesConvertor(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
