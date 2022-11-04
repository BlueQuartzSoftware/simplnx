#include "FindKernelAvgMisorientationsFilter.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/FindKernelAvgMisorientations.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string FindKernelAvgMisorientationsFilter::name() const
{
  return FilterTraits<FindKernelAvgMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindKernelAvgMisorientationsFilter::className() const
{
  return FilterTraits<FindKernelAvgMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindKernelAvgMisorientationsFilter::uuid() const
{
  return FilterTraits<FindKernelAvgMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindKernelAvgMisorientationsFilter::humanName() const
{
  return "Find Kernel Average Misorientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindKernelAvgMisorientationsFilter::defaultTags() const
{
  return {"#Statistics", "#Crystallography", "#Misorientation"};
}

//------------------------------------------------------------------------------
Parameters FindKernelAvgMisorientationsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_KernelSize_Key, "Kernel Radius", "", std::vector<int32>{1, 1, 1}, std::vector<std::string>{"X", "Y", "Z"}));
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::ComponentTypes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath({"CellData", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::ComponentTypes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "", DataPath({"CellData", "Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::ComponentTypes{{4}}));
  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "", DataPath({"Ensemble Data", "CrystalStructures"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint32}, ArraySelectionParameter::ComponentTypes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_KernelAverageMisorientationsArrayName_Key, "Kernel Average Misorientations", "", "KernelAverageMisorientations"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindKernelAvgMisorientationsFilter::clone() const
{
  return std::make_unique<FindKernelAvgMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindKernelAvgMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel) const
{
  auto pKernelSizeValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_KernelSize_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pKernelAverageMisorientationsArrayNameValue = pCellPhasesArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  // Declare the preflightResult variable that will be populated with the results
  // of the preflight. The PreflightResult type contains the output Actions and
  // any preflight updated values that you want to be displayed to the user, typically
  // through a user interface (UI).
  PreflightResult preflightResult;

  // If your filter is making structural changes to the DataStructure then the filter
  // is going to create OutputActions subclasses that need to be returned. This will
  // store those actions.
  complex::Result<OutputActions> resultOutputActions;

  const auto& cellPhases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);

  // Create output Kernel Average Misorientations
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellPhases.getIDataStore()->getTupleShape(), std::vector<usize>{1}, pKernelAverageMisorientationsArrayNameValue);
    resultOutputActions.value().actions.push_back(std::move(createArrayAction));
  }

  // If your filter is going to pass back some `preflight updated values` then this is where you
  // would create the code to store those values in the appropriate object. Note that we
  // in line creating the pair (NOT a std::pair<>) of Key:Value that will get stored in
  // the std::vector<PreflightValue> object.
  std::vector<PreflightValue> preflightUpdatedValues;

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindKernelAvgMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  FindKernelAvgMisorientationsInputValues inputValues;

  inputValues.KernelSize = filterArgs.value<VectorInt32Parameter::ValueType>(k_KernelSize_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.KernelAverageMisorientationsArrayName = inputValues.CellPhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindKernelAvgMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
