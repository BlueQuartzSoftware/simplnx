#include "ComputeKernelAvgMisorientationsFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/ComputeKernelAvgMisorientations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/VectorParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeKernelAvgMisorientationsFilter::name() const
{
  return FilterTraits<ComputeKernelAvgMisorientationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeKernelAvgMisorientationsFilter::className() const
{
  return FilterTraits<ComputeKernelAvgMisorientationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeKernelAvgMisorientationsFilter::uuid() const
{
  return FilterTraits<ComputeKernelAvgMisorientationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeKernelAvgMisorientationsFilter::humanName() const
{
  return "Compute Kernel Average Misorientations";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeKernelAvgMisorientationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Misorientation"};
}

//------------------------------------------------------------------------------
Parameters ComputeKernelAvgMisorientationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorInt32Parameter>(k_KernelSize_Key, "Kernel Radius", "Size of the kernel in the X, Y and Z directions (in number of Cells)", std::vector<int32>{1, 1, 1},
                                                       std::vector<std::string>{"X", "Y", "Z"}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "Path to the target geometry", DataPath({"Data Container"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath({"CellData", "Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "Specifies the orientation of the Cell in quaternion representation",
                                                          DataPath({"CellData", "Quats"}), ArraySelectionParameter::AllowedTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_KernelAverageMisorientationsArrayName_Key, "Kernel Average Misorientations",
                                                          "The name of the array containing the average  misorientation (in Degrees) for all Cells within the kernel and the central Cell",
                                                          "KernelAverageMisorientations"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeKernelAvgMisorientationsFilter::clone() const
{
  return std::make_unique<ComputeKernelAvgMisorientationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeKernelAvgMisorientationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                              const std::atomic_bool& shouldCancel) const
{
  auto pKernelSizeValue = filterArgs.value<VectorInt32Parameter::ValueType>(k_KernelSize_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  auto pKernelAverageMisorientationsArrayNameValue = pCellPhasesArrayPathValue.replaceName(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  auto inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

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
Result<> ComputeKernelAvgMisorientationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  ComputeKernelAvgMisorientationsInputValues inputValues;

  inputValues.KernelSize = filterArgs.value<VectorInt32Parameter::ValueType>(k_KernelSize_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.CellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.KernelAverageMisorientationsArrayName = inputValues.CellPhasesArrayPath.replaceName(filterArgs.value<std::string>(k_KernelAverageMisorientationsArrayName_Key));
  inputValues.InputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ComputeKernelAvgMisorientations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelSizeKey = "KernelSize";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteral k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteral k_KernelAverageMisorientationsArrayNameKey = "KernelAverageMisorientationsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeKernelAvgMisorientationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeKernelAvgMisorientationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntVec3FilterParameterConverter>(args, json, SIMPL::k_KernelSizeKey, k_KernelSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_KernelAverageMisorientationsArrayNameKey,
                                                                                                                   k_KernelAverageMisorientationsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
