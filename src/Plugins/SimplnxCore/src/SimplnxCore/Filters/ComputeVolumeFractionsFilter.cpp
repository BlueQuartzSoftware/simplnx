#include "ComputeVolumeFractionsFilter.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/DataObjectNameParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeVolumeFractionsFilter::name() const
{
  return FilterTraits<ComputeVolumeFractionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeVolumeFractionsFilter::className() const
{
  return FilterTraits<ComputeVolumeFractionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeVolumeFractionsFilter::uuid() const
{
  return FilterTraits<ComputeVolumeFractionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeVolumeFractionsFilter::humanName() const
{
  return "Compute Volume Fractions of Ensembles";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeVolumeFractionsFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeVolumeFractionsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Array specifying which Ensemble each Cell belong",
                                                          DataPath({"DataContainer", "Cell Data", "Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<DataGroupSelectionParameter>(k_CellEnsembleAttributeMatrixPath_Key, "Cell Ensemble Attribute Matrix",
                                                              "The path to the cell ensemble attribute matrix where the output volume fractions array will be stored",
                                                              DataPath({"DataContainer", "Cell Ensemble Data"}), DataGroupSelectionParameter::AllowedTypes{BaseGroup::GroupType::AttributeMatrix}));
  params.insertSeparator(Parameters::Separator{"Output Ensemble Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolFractionsArrayName_Key, "Volume Fractions", "Fraction of volume that belongs to each Ensemble", "Volume Fractions"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeVolumeFractionsFilter::clone() const
{
  return std::make_unique<ComputeVolumeFractionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeVolumeFractionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                     const std::atomic_bool& shouldCancel) const
{
  auto pCellEnsembleAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  auto pVolFractionsArrayNameValue = filterArgs.value<std::string>(k_VolFractionsArrayName_Key);

  const DataPath volFractionsArrayPath = pCellEnsembleAttributeMatrixPathValue.createChildPath(pVolFractionsArrayNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* cellEnsembleData = dataStructure.getDataAs<AttributeMatrix>(pCellEnsembleAttributeMatrixPathValue);
  if(cellEnsembleData == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-47630, fmt::format("Could not find selected feature Attribute Matrix at path '{}'", pCellEnsembleAttributeMatrixPathValue.toString()))};
  }

  auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellEnsembleData->getShape(), std::vector<usize>{1}, volFractionsArrayPath);
  resultOutputActions.value().appendAction(std::move(createArrayAction));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeVolumeFractionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                   const std::atomic_bool& shouldCancel) const
{
  auto pCellEnsembleAttributeMatrixPathValue = filterArgs.value<DataPath>(k_CellEnsembleAttributeMatrixPath_Key);
  auto pVolFractionsArrayNameValue = filterArgs.value<std::string>(k_VolFractionsArrayName_Key);

  const auto& cellPhases = dataStructure.getDataAs<Int32Array>(filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key))->getDataStoreRef();
  auto& volFractions = dataStructure.getDataAs<Float32Array>(pCellEnsembleAttributeMatrixPathValue.createChildPath(pVolFractionsArrayNameValue))->getDataStoreRef();

  usize totalPoints = cellPhases.getNumberOfTuples();
  usize totalEnsembles = volFractions.getNumberOfTuples();

  std::vector<usize> ensembleElements(totalEnsembles, 0);
  // Calculate the total number of elements in each Ensemble
  for(usize index = 0; index < totalPoints; index++)
  {
    ensembleElements[cellPhases[index]]++;
  }
  // Calculate the Volume Fraction
  for(usize index = 0; index < totalEnsembles; index++)
  {
    volFractions[index] = static_cast<float32>(ensembleElements[index]) / static_cast<float32>(totalPoints);
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_VolFractionsArrayPathKey = "VolFractionsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeVolumeFractionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeVolumeFractionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Cell Ensemble attribute matrix parameter is not applicable in NX

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_VolFractionsArrayPathKey, k_CellEnsembleAttributeMatrixPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_VolFractionsArrayPathKey, k_VolFractionsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
