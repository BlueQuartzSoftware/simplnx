#include "ImportDeformKeyFileV12Filter.hpp"

#include "Algorithms/ImportDeformKeyFileV12.hpp"

#include "complex/Parameters/BoolParameter.hpp"

#include <filesystem>

namespace fs = std::filesystem;
using namespace complex;

namespace
{
// Error Code constants
constexpr complex::int32 k_UnmatchingTupleCountError = -51001;
} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ImportDeformKeyFileV12Filter::name() const
{
  return FilterTraits<ImportDeformKeyFileV12Filter>::name.str();
}

//------------------------------------------------------------------------------
std::string ImportDeformKeyFileV12Filter::className() const
{
  return FilterTraits<ImportDeformKeyFileV12Filter>::className;
}

//------------------------------------------------------------------------------
Uuid ImportDeformKeyFileV12Filter::uuid() const
{
  return FilterTraits<ImportDeformKeyFileV12Filter>::uuid;
}

//------------------------------------------------------------------------------
std::string ImportDeformKeyFileV12Filter::humanName() const
{
  return "Import Deform Key File (v12)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ImportDeformKeyFileV12Filter::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ImportDeformKeyFileV12Filter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<BoolParameter>(k_UseVerboseOutput_Key, "Use Verbose Output", "", false));

  params.insertSeparator(Parameters::Separator{"Required Data Objects"});

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ImportDeformKeyFileV12Filter::clone() const
{
  return std::make_unique<ImportDeformKeyFileV12Filter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ImportDeformKeyFileV12Filter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto pOutputStyleValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputStyle_Key);

  // Declare the preflightResult variable
  PreflightResult preflightResult;
  complex::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ImportDeformKeyFileV12Filter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  ImportDeformKeyFileV12InputValues inputValues;

  inputValues.MultiplesOfAverage = filterArgs.value<float32>(k_MultiplesOfAverage_Key);
  inputValues.EquivalentDiametersArrayPath = filterArgs.value<DataPath>(k_EquivalentDiametersArrayPath_Key);
  inputValues.FeaturePhasesArrayPath = filterArgs.value<DataPath>(k_FeaturePhasesArrayPath_Key);
  inputValues.CentroidsArrayPath = filterArgs.value<DataPath>(k_CentroidsArrayPath_Key);
  inputValues.NeighborhoodsArrayName = filterArgs.value<DataPath>(k_NeighborhoodsArrayName_Key);
  inputValues.NeighborhoodListArrayName = filterArgs.value<DataPath>(k_NeighborhoodListArrayName_Key);
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return ImportDeformKeyFileV12(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
