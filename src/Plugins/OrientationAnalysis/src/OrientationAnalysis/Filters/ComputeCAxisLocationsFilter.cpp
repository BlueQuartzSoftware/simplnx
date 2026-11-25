#include "ComputeCAxisLocationsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeCAxisLocations.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeCAxisLocationsFilter::name() const
{
  return FilterTraits<ComputeCAxisLocationsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeCAxisLocationsFilter::className() const
{
  return FilterTraits<ComputeCAxisLocationsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeCAxisLocationsFilter::uuid() const
{
  return FilterTraits<ComputeCAxisLocationsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeCAxisLocationsFilter::humanName() const
{
  return "Compute C-Axis Locations";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeCAxisLocationsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeCAxisLocationsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Cell Quaternions", "DataPath to input quaternion values", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapesType{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CAxisLocationsArrayName_Key, "C-Axis Locations", "DataPath to calculated C-Axis locations", "CAxisLocation"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeCAxisLocationsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeCAxisLocationsFilter::clone() const
{
  return std::make_unique<ComputeCAxisLocationsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeCAxisLocationsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pQuatsArrayPathValue = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  PreflightResult preflightResult;
  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  std::vector<DataPath> dataPaths = {pQuatsArrayPathValue, pCellPhasesArrayPathValue};
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {.output_actions =
                MakeErrorResult<OutputActions>(-3520, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  {
    const DataPath pCAxisLocationsPathValue = pQuatsArrayPathValue.replaceName(filterArgs.value<std::string>(k_CAxisLocationsArrayName_Key));
    const ShapeType tupleShape = dataStructure.getDataRefAs<Float32ArrayType>(pQuatsArrayPathValue).getTupleShape();
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3}, pCAxisLocationsPathValue);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  resultOutputActions.warnings().push_back(
      {-3521, "Finding the c-axis locations requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Make sure your data is of one of these two types."});

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeCAxisLocationsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeCAxisLocationsInputValues inputValues;

  inputValues.quats_array_path = filterArgs.value<DataPath>(k_QuatsArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.c_axis_locations_array_name = inputValues.quats_array_path.replaceName(filterArgs.value<std::string>(k_CAxisLocationsArrayName_Key));

  return ComputeCAxisLocations(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_QuatsArrayPathKey = "QuatsArrayPath";
constexpr StringLiteralType k_CAxisLocationsArrayNameKey = "CAxisLocationsArrayName";
} // namespace simpl
} // namespace

Result<Arguments> ComputeCAxisLocationsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeCAxisLocationsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_QuatsArrayPathKey, k_QuatsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CAxisLocationsArrayNameKey, k_CAxisLocationsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
