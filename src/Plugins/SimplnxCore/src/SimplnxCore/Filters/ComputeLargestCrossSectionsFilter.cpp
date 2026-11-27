#include "ComputeLargestCrossSectionsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeLargestCrossSections.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeLargestCrossSectionsFilter::name() const
{
  return FilterTraits<ComputeLargestCrossSectionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeLargestCrossSectionsFilter::className() const
{
  return FilterTraits<ComputeLargestCrossSectionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeLargestCrossSectionsFilter::uuid() const
{
  return FilterTraits<ComputeLargestCrossSectionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeLargestCrossSectionsFilter::humanName() const
{
  return "Compute Feature Largest Cross-Section Areas";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeLargestCrossSectionsFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters ComputeLargestCrossSectionsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane of Interest", "Specifies which plane to consider when determining the maximum cross-section for each Feature", 0,
                                                   ChoicesParameter::ChoicesType{"XY", "XZ", "YZ"}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The complete path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "The path to the cell feature attribute matrix", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_LargestCrossSectionsArrayName_Key, "Largest Cross Sections",
                                                          "Area of largest cross-section for Feature perpendicular to the user specified direction", "LargestCrossSections"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeLargestCrossSectionsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ComputeLargestCrossSectionsFilter::clone() const
{
  return std::make_unique<ComputeLargestCrossSectionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeLargestCrossSectionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                          const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  auto pImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto pCellFeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto pLargestCrossSectionsArrayNameValue = filterArgs.value<std::string>(k_LargestCrossSectionsArrayName_Key);

  DataPath largestCrossSectionPath = pCellFeatureAttributeMatrixPath.createChildPath(pLargestCrossSectionsArrayNameValue);

  PreflightResult preflightResult;
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    const auto& cellFeatAM = dataStructure.getDataRefAs<AttributeMatrix>(pCellFeatureAttributeMatrixPath);
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, cellFeatAM.getShape(), std::vector<usize>{1}, largestCrossSectionPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPath);
  SizeVec3Type dims = imageGeom.getDimensions();
  if(dims[0] <= 1 || dims[1] <= 1 || dims[2] <= 1)
  {
    return MakePreflightErrorResult(-3710, fmt::format("Image Geometry at path '{}' is not 3D. The dimensions are ({})", pImageGeometryPath.toString(), string_utilities::FormatDimensions3D(dims)));
  }

  const auto& featureIds = dataStructure.getDataRefAs<Int32ArrayType>(pFeatureIdsArrayPathValue);
  const usize numCells = imageGeom.getNumberOfCells();
  if(featureIds.getNumberOfTuples() != numCells)
  {
    return MakePreflightErrorResult(-3711, fmt::format("The Feature Ids DataArray '{}' has {} tuples, but Image Geometry '{}' has {} cells. The tuple count must equal the cell count.",
                                                       pFeatureIdsArrayPathValue.toString(), featureIds.getNumberOfTuples(), pImageGeometryPath.toString(), numCells));
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeLargestCrossSectionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ComputeLargestCrossSectionsInputValues inputValues;

  inputValues.plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.feature_ids_array_path = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.largest_cross_sections_array_path = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key).createChildPath(filterArgs.value<std::string>(k_LargestCrossSectionsArrayName_Key));

  return ComputeLargestCrossSections(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace simpl
{
constexpr StringLiteralType k_PlaneKey = "Plane";
constexpr StringLiteralType k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteralType k_LargestCrossSectionsArrayPathKey = "LargestCrossSectionsArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ComputeLargestCrossSectionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeLargestCrossSectionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_PlaneKey, k_Plane_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, simpl::k_LargestCrossSectionsArrayPathKey,
                                                                                                                           k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, simpl::k_LargestCrossSectionsArrayPathKey,
                                                                                                                                    k_LargestCrossSectionsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
