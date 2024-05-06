#include "FindLargestCrossSectionsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/FindLargestCrossSections.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string FindLargestCrossSectionsFilter::name() const
{
  return FilterTraits<FindLargestCrossSectionsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string FindLargestCrossSectionsFilter::className() const
{
  return FilterTraits<FindLargestCrossSectionsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid FindLargestCrossSectionsFilter::uuid() const
{
  return FilterTraits<FindLargestCrossSectionsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string FindLargestCrossSectionsFilter::humanName() const
{
  return "Find Feature Largest Cross-Section Areas";
}

//------------------------------------------------------------------------------
std::vector<std::string> FindLargestCrossSectionsFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological"};
}

//------------------------------------------------------------------------------
Parameters FindLargestCrossSectionsFilter::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane of Interest", "Specifies which plane to consider when determining the maximum cross-section for each Feature", 0,
                                                   ChoicesParameter::Choices{"XY", "XZ", "YZ"}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeometryPath_Key, "Image Geometry", "The complete path to the input image geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_FeatureIdsArrayPath_Key, "Cell Feature Ids", "Data Array that specifies to which Feature each Element belongs", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "The path to the cell feature attribute matrix", DataPath{}));
  params.insert(std::make_unique<DataObjectNameParameter>(k_LargestCrossSectionsArrayName_Key, "Largest Cross Sections",
                                                          "Area of largest cross-section for Feature perpendicular to the user specified direction", "LargestCrossSections"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer FindLargestCrossSectionsFilter::clone() const
{
  return std::make_unique<FindLargestCrossSectionsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult FindLargestCrossSectionsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                       const std::atomic_bool& shouldCancel) const
{
  auto pPlaneValue = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
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
  SizeVec3 dims = imageGeom.getDimensions();
  if(dims[0] <= 1 || dims[1] <= 1 || dims[2] <= 1)
  {
    return MakePreflightErrorResult(-3710, fmt::format("Image Geometry at path '{}' is not 3D.  The dimensions are ({}, {}, {})", pImageGeometryPath.toString(), dims[0], dims[1], dims[2]));
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> FindLargestCrossSectionsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                     const std::atomic_bool& shouldCancel) const
{
  FindLargestCrossSectionsInputValues inputValues;

  inputValues.Plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  inputValues.LargestCrossSectionsArrayPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key).createChildPath(filterArgs.value<std::string>(k_LargestCrossSectionsArrayName_Key));

  return FindLargestCrossSections(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_PlaneKey = "Plane";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_LargestCrossSectionsArrayPathKey = "LargestCrossSectionsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> FindLargestCrossSectionsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = FindLargestCrossSectionsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_PlaneKey, k_Plane_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_FeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_LargestCrossSectionsArrayPathKey,
                                                                                                                         k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayNameFilterParameterConverter>(args, json, SIMPL::k_LargestCrossSectionsArrayPathKey, k_LargestCrossSectionsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
