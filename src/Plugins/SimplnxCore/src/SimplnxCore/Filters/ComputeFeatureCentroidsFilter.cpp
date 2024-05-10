#include "ComputeFeatureCentroidsFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureCentroids.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "simplnx/Parameters/GeometrySelectionParameter.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeFeatureCentroidsFilter::name() const
{
  return FilterTraits<ComputeFeatureCentroidsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeFeatureCentroidsFilter::className() const
{
  return FilterTraits<ComputeFeatureCentroidsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeFeatureCentroidsFilter::uuid() const
{
  return FilterTraits<ComputeFeatureCentroidsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeFeatureCentroidsFilter::humanName() const
{
  return "Compute Feature Centroids";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureCentroidsFilter::defaultTags() const
{
  return {className(), "Generic", "Morphological", "Find"};
}

//------------------------------------------------------------------------------
Parameters ComputeFeatureCentroidsFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry whose Features' centroids will be calculated", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which Feature each cell belongs", DataPath({"CellData", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_FeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "The cell feature attribute matrix",
                                                                    DataPath({"Data Container", "Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CentroidsArrayName_Key, "Centroids", "DataPath to create the 'Centroids' output array", "Centroids"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeFeatureCentroidsFilter::clone() const
{
  return std::make_unique<ComputeFeatureCentroidsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeFeatureCentroidsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                      const std::atomic_bool& shouldCancel) const
{
  auto pFeatureIdsArrayPathValue = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto featureAttrMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);

  auto pCentroidsArrayName = filterArgs.value<std::string>(k_CentroidsArrayName_Key);
  DataPath pCentroidsArrayPath = featureAttrMatrixPath.createChildPath(pCentroidsArrayName);

  PreflightResult preflightResult;

  nx::core::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  // Feature Data:
  // Validating the Feature Attribute Matrix and trying to find a child of the Group
  // that is an IDataArray subclass, so we can get the proper tuple shape
  const auto* featureAttrMatrix = dataStructure.getDataAs<AttributeMatrix>(featureAttrMatrixPath);
  if(featureAttrMatrix == nullptr)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-12700, fmt::format("Cannot find the selected feature Attribute Matrix at path '{}'", featureAttrMatrixPath.toString())}})};
  }
  IDataStore::ShapeType tupleShape = featureAttrMatrix->getShape();

  // Create the CreateArrayAction within a scope so that we do not accidentally use the variable is it is getting "moved"
  {
    auto createFeatureCentroidsAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleShape, std::vector<usize>{3ULL}, pCentroidsArrayPath);
    resultOutputActions.value().appendAction(std::move(createFeatureCentroidsAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ComputeFeatureCentroidsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                    const std::atomic_bool& shouldCancel) const
{
  ComputeFeatureCentroidsInputValues inputValues;

  auto pCentroidsArrayName = filterArgs.value<std::string>(k_CentroidsArrayName_Key);
  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  inputValues.CentroidsArrayPath = inputValues.FeatureAttributeMatrixPath.createChildPath(pCentroidsArrayName);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ComputeFeatureCentroids(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_CentroidsArrayPathKey = "CentroidsArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureCentroidsFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureCentroidsFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionToGeometrySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_SelectedImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToAMFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_FeatureAttributeMatrixPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArrayCreationToDataObjectNameFilterParameterConverter>(args, json, SIMPL::k_CentroidsArrayPathKey, k_CentroidsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
