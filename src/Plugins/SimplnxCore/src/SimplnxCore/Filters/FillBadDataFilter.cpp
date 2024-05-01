#include "FillBadDataFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FillBadData.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FillBadDataFilter::name() const
{
  return FilterTraits<FillBadDataFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FillBadDataFilter::className() const
{
  return FilterTraits<FillBadDataFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FillBadDataFilter::uuid() const
{
  return FilterTraits<FillBadDataFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FillBadDataFilter::humanName() const
{
  return "Fill Bad Data";
}

//------------------------------------------------------------------------------
std::vector<std::string> FillBadDataFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup"};
}

//------------------------------------------------------------------------------
Parameters FillBadDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Int32Parameter>(k_MinAllowedDefectSize_Key, "Minimum Allowed Defect Size", "The size at which a group of bad Cells are left unfilled as a 'defect'", 1));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_StoreAsNewPhase_Key, "Store Defects as New Phase", "Whether to change the phase of 'defect' larger than the minimum allowed size above", false));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  //  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedCellDataGroup_Key, "Cell Data Attribute Matrix", "Cell data Attribute Matrix", DataPath{}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each Element belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "Specifies to which Ensemble each Cell belongs.", DataPath({"Phases"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "The list of arrays to ignore when performing the algorithm",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllDataTypes()));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_StoreAsNewPhase_Key, k_CellPhasesArrayPath_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FillBadDataFilter::clone() const
{
  return std::make_unique<FillBadDataFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FillBadDataFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                          const std::atomic_bool& shouldCancel) const
{
  auto pMinAllowedDefectSizeValue = filterArgs.value<int32>(k_MinAllowedDefectSize_Key);
  //  auto cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);
  auto cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);

  PreflightResult preflightResult;

  if(pMinAllowedDefectSizeValue < 1)
  {
    MakeErrorResult(-16500, fmt::format("Minimum Allowed Defect Size must be at least 1. Value given was {}", pMinAllowedDefectSizeValue));
  }

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  //  const auto* cellDataGroup = dataStructure.getDataAs<AttributeMatrix>(cellDataGroupPath);
  //  if(cellDataGroup == nullptr)
  //  {
  //    return {MakeErrorResult<OutputActions>(-11501, fmt::format("Could not find cell data Attribute Matrix at path '{}'", cellDataGroupPath.toString()))};
  //  }

  auto storeAsNewPhase = filterArgs.value<bool>(k_StoreAsNewPhase_Key);
  // Get the Feature Phases Array and get its TupleShape
  const auto* cellPhases = dataStructure.getDataAs<Int32Array>(cellPhasesArrayPath);
  if(storeAsNewPhase && nullptr == cellPhases)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12801, "Cell Phases Data Array is not of the correct type or was not found at the given path"}})};
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FillBadDataFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel) const
{
  FillBadDataInputValues inputValues;

  inputValues.minAllowedDefectSizeValue = filterArgs.value<int32>(k_MinAllowedDefectSize_Key);
  inputValues.storeAsNewPhase = filterArgs.value<bool>(k_StoreAsNewPhase_Key);
  inputValues.featureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.ignoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return FillBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinAllowedDefectSizeKey = "MinAllowedDefectSize";
constexpr StringLiteral k_StoreAsNewPhaseKey = "StoreAsNewPhase";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteral k_IgnoredDataArrayPathsKey = "IgnoredDataArrayPaths";
} // namespace SIMPL
} // namespace

Result<Arguments> FillBadDataFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FillBadDataFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<int32>>(args, json, SIMPL::k_MinAllowedDefectSizeKey, k_MinAllowedDefectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreAsNewPhaseKey, k_StoreAsNewPhase_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedCellDataGroup_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_IgnoredDataArrayPathsKey, k_IgnoredDataArrayPaths_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
