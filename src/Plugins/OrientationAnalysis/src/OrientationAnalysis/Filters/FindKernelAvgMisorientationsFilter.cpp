#include "FindKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/FindKernelAvgMisorientations.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

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
  return {className(), "Statistics", "Crystallography", "Misorientation"};
}

//------------------------------------------------------------------------------
Parameters FindKernelAvgMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_KernelSize_Key, "Kernel Radius", "Size of the kernel in the X, Y and Z directions (in number of Cells)", std::vector<int32>{1, 1, 1},
                                                       std::vector<std::string>{"X", "Y", "Z"}));
  params.insertSeparator(Parameters::Separator{"Required Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "Path to the target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"CellData", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Quaternions", "Specifies the orientation of the Cell in quaternion representation", DataPath({"CellData", "Quats"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Required Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_KernelAverageMisorientationsArrayName_Key, "Kernel Average Misorientations",
                                                          "The name of the array containing the average  misorientation (in Degrees) for all Cells within the kernel and the central Cell",
                                                          "KernelAverageMisorientations"));

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
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pKernelAverageMisorientationsArrayNameValue = pCellPhasesArrayPathValue.getParent().createChildPath(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  PreflightResult preflightResult;

  complex::Result<OutputActions> resultOutputActions;

  const auto& cellPhases = dataStructure.getDataRefAs<Int32Array>(pCellPhasesArrayPathValue);

  // Create output Kernel Average Misorientations
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellPhases.getIDataStore()->getTupleShape(), std::vector<usize>{1}, pKernelAverageMisorientationsArrayNameValue);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

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
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.KernelAverageMisorientationsArrayName = inputValues.CellPhasesArrayPath.getParent().createChildPath(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);

  return FindKernelAvgMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
