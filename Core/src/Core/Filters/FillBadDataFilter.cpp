#include "FillBadDataFilter.hpp"

#include "Core/Filters/Algorithms/FillBadData.hpp"

#include "complex/DataStructure/AttributeMatrix.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

using namespace complex;

namespace complex
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
  return {"#Processing", "#Cleanup"};
}

//------------------------------------------------------------------------------
Parameters FillBadDataFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<Int32Parameter>(k_MinAllowedDefectSize_Key, "Minimum Allowed Defect Size", "", 1));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreAsNewPhase_Key, "Store Defects as New Phase", "", false));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometry_Key, "Selected Image Geometry", "", DataPath{}, GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_SelectedCellDataGroup_Key, "Cell Data Attribute Matrix", "Cell data Attribute Matrix", DataPath{}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Feature Ids", "", DataPath({"CellData", "FeatureIds"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Cell Phases", "", DataPath({"Phases"}), ArraySelectionParameter::AllowedTypes{DataType::int32}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_IgnoredDataArrayPaths_Key, "Attribute Arrays to Ignore", "", MultiArraySelectionParameter::ValueType{}, complex::GetAllDataTypes()));
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
  auto cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  PreflightResult preflightResult;

  if(pMinAllowedDefectSizeValue < 1)
  {
    MakeErrorResult(-16500, fmt::format("Minimum Allowed Defect Size must be at least 1. Value given was {}", pMinAllowedDefectSizeValue));
  }

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  const auto* cellDataGroup = dataStructure.getDataAs<AttributeMatrix>(cellDataGroupPath);
  if(cellDataGroup == nullptr)
  {
    return {MakeErrorResult<OutputActions>(-11501, fmt::format("Could not find cell data Attribute Matrix at path '{}'", cellDataGroupPath.toString()))};
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
  inputValues.featureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.cellPhasesArrayPath = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.ignoredDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_IgnoredDataArrayPaths_Key);
  inputValues.inputImageGeometry = filterArgs.value<DataPath>(k_SelectedImageGeometry_Key);
  inputValues.cellDataGroupPath = filterArgs.value<DataPath>(k_SelectedCellDataGroup_Key);

  return FillBadData(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace complex
