#include "ComputeBoundaryCellsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeBoundaryCells.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeBoundaryCellsFilter::name() const
{
  return FilterTraits<ComputeBoundaryCellsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeBoundaryCellsFilter::className() const
{
  return FilterTraits<ComputeBoundaryCellsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeBoundaryCellsFilter::uuid() const
{
  return FilterTraits<ComputeBoundaryCellsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeBoundaryCellsFilter::humanName() const
{
  return "Compute Boundary Cells (Image)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeBoundaryCellsFilter::defaultTags() const
{
  return {className(), "Generic", "Spatial"};
}

//------------------------------------------------------------------------------
Parameters ComputeBoundaryCellsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_IgnoreFeatureZero_Key, "Ignore Feature 0", "Do not use feature 0", false));
  params.insert(std::make_unique<BoolParameter>(k_IncludeVolumeBoundary_Key, "Include Volume Boundary", "Include the Cell boundaries", false));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Image Geometry", "The selected geometry to which the cells belong", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_BoundaryCellsArrayName_Key, "Boundary Cells",
                                                          "The number of neighboring Cells of a given Cell that belong to a different Feature than itself. Values will range from 0 to 6",
                                                          "BoundaryCells"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeBoundaryCellsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeBoundaryCellsFilter::clone() const
{
  return std::make_unique<ComputeBoundaryCellsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeBoundaryCellsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel) const
{
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pBoundaryCellsArrayNameValue = filterArgs.value<std::string>(k_BoundaryCellsArrayName_Key);

  const DataPath boundaryCellsArrayPath = pFeatureIdsArrayPathValue.replaceName(pBoundaryCellsArrayNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  const auto& featureIds = dataStructure.getDataRefAs<Int32Array>(pFeatureIdsArrayPathValue);
  const usize numFeatureIds = featureIds.getSize();
  const usize numCells = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPath).getNumberOfCells();
  if(numFeatureIds != numCells)
  {
    return {MakeErrorResult<OutputActions>(
        -5320,
        fmt::format("The selected Image geometry '{}' and feature ids array '{}' have mismatching number of elements. Make sure the selected feature ids was created for the selected Image geometry.",
                    pImageGeometryPath.toString(), pFeatureIdsArrayPathValue.toString()))};
  }

  auto action = std::make_unique<CreateArrayAction>(DataType::int8, featureIds.getTupleShape(), std::vector<usize>{1}, boundaryCellsArrayPath);
  resultOutputActions.value().appendAction(std::move(action));

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeBoundaryCellsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel) const
{
  ComputeBoundaryCellsInputValues inputValues;

  inputValues.IgnoreFeatureZero = filterArgs.value<bool>(k_IgnoreFeatureZero_Key);
  inputValues.IncludeVolumeBoundary = filterArgs.value<bool>(k_IncludeVolumeBoundary_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.BoundaryCellsArrayName = inputValues.FeatureIdsArrayPath.replaceName(filterArgs.value<std::string>(k_BoundaryCellsArrayName_Key));

  return ComputeBoundaryCells(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_IgnoreFeatureZeroKey = "IgnoreFeatureZero";
constexpr StringLiteral k_IncludeVolumeBoundaryKey = "IncludeVolumeBoundary";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_BoundaryCellsArrayNameKey = "BoundaryCellsArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeBoundaryCellsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeBoundaryCellsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_IgnoreFeatureZeroKey, k_IgnoreFeatureZero_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_IncludeVolumeBoundaryKey, k_IncludeVolumeBoundary_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_BoundaryCellsArrayNameKey, k_BoundaryCellsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
