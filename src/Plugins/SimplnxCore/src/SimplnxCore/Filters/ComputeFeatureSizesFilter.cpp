#include "ComputeFeatureSizesFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ComputeFeatureSizes.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{
namespace
{
constexpr int32 k_InvalidInputDimensions = -74770;
} // namespace

std::string ComputeFeatureSizesFilter::name() const
{
  return FilterTraits<ComputeFeatureSizesFilter>::name;
}

std::string ComputeFeatureSizesFilter::className() const
{
  return FilterTraits<ComputeFeatureSizesFilter>::className;
}

Uuid ComputeFeatureSizesFilter::uuid() const
{
  return FilterTraits<ComputeFeatureSizesFilter>::uuid;
}

std::string ComputeFeatureSizesFilter::humanName() const
{
  return "Compute Feature Sizes";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeFeatureSizesFilter::defaultTags() const
{
  return {className(), "Statistics", "Morphological", "Feature Calculation", "Find Feature Sizes"};
}

Parameters ComputeFeatureSizesFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<BoolParameter>(k_SaveElementSizes_Key, "Generate Missing Element Sizes",
                                                "If checked this will generate and store the element sizes ONLY if the geometry does not already contain them.", false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_GeometryPath_Key, "Input Grid Geometry", "DataPath to input Image or Rectilinear Grid Geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image, IGeometry::Type::RectGrid}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_CellFeatureIdsArrayPath_Key, "Cell Feature Ids", "Specifies to which feature each cell belongs.", DataPath({"Cell Data", "FeatureIds"}),
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Feature Data"});
  params.insert(std::make_unique<AttributeMatrixSelectionParameter>(k_CellFeatureAttributeMatrixPath_Key, "Feature Attribute Matrix", "Feature Attribute Matrix of the selected Feature Ids",
                                                                    DataPath({"Cell Feature Data"})));

  params.insertSeparator(Parameters::Separator{"Output Feature Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_EquivalentDiametersName_Key, "Feature Equivalent Diameters Name",
                                                          "Name of the array that will store the equivalent spherical/circular diameters of each feature", "EquivalentDiameters"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_NumElementsName_Key, "Feature Voxel Counts Name", "Name of the array that will store the number of voxels per feature", "NumElements"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_VolumesName_Key, "Volumes/Area Name", "Name of the array that will store the volumes/area of each feature", "Volumes"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeFeatureSizesFilter::parametersVersion() const
{
  return 1;
}

IFilter::UniquePointer ComputeFeatureSizesFilter::clone() const
{
  return std::make_unique<ComputeFeatureSizesFilter>();
}

IFilter::PreflightResult ComputeFeatureSizesFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto geometryPath = filterArgs.value<DataPath>(k_GeometryPath_Key);

  auto featureIdsPath = filterArgs.value<DataPath>(k_CellFeatureIdsArrayPath_Key);
  auto featureAttributeMatrixPath = filterArgs.value<DataPath>(k_CellFeatureAttributeMatrixPath_Key);
  auto volumesName = filterArgs.value<std::string>(k_VolumesName_Key);
  auto equivalentDiametersName = filterArgs.value<std::string>(k_EquivalentDiametersName_Key);
  auto numElementsName = filterArgs.value<std::string>(k_NumElementsName_Key);
  DataPath volumesPath = featureAttributeMatrixPath.createChildPath(volumesName);
  DataPath equivalentDiametersPath = featureAttributeMatrixPath.createChildPath(equivalentDiametersName);
  DataPath numElementsPath = featureAttributeMatrixPath.createChildPath(numElementsName);

  const auto& geomRef = dataStructure.getDataRefAs<IGeometry>(geometryPath);
  IGeometry::Type geomType = geomRef.getGeomType();
  if(geomType == IGeometry::Type::Image)
  {
    const auto& imageGeom = dynamic_cast<const ImageGeom&>(geomRef);

    usize emptyDimCount = 0;
    if(imageGeom.getNumXCells() < 2)
    {
      emptyDimCount++;
    }
    if(imageGeom.getNumYCells() < 2)
    {
      emptyDimCount++;
    }
    if(imageGeom.getNumZCells() < 2)
    {
      emptyDimCount++;
    }

    if(emptyDimCount > 1)
    {
      return MakePreflightErrorResult(k_InvalidInputDimensions, "This filter requires at least 2 valid dimensions in the image geom. Two or more 1's were found in the image's dimensions");
    }
  }

  const auto& featureIdsArray = dataStructure.getDataRefAs<Int32Array>(featureIdsPath);

  const std::string arrayDataFormat = featureIdsArray.getDataFormat();

  const auto& featAttributeMatrix = dataStructure.getDataRefAs<AttributeMatrix>(featureAttributeMatrixPath);

  ShapeType tupleDimensions = featAttributeMatrix.getShape();
  usize numberOfComponents = 1;

  auto createVolumesAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, volumesPath, arrayDataFormat);
  auto createEquivalentDiametersAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDimensions, std::vector<usize>{numberOfComponents}, equivalentDiametersPath, arrayDataFormat);
  auto createNumElementsAction = std::make_unique<CreateArrayAction>(DataType::int32, tupleDimensions, std::vector<usize>{numberOfComponents}, numElementsPath, arrayDataFormat);

  OutputActions actions;
  actions.appendAction(std::move(createVolumesAction));
  actions.appendAction(std::move(createEquivalentDiametersAction));
  actions.appendAction(std::move(createNumElementsAction));

  return {std::move(actions)};
}

Result<> ComputeFeatureSizesFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ComputeFeatureSizesInputValues inputValues;

  inputValues.EquivalentDiametersName = filterArgs.value<DataObjectNameParameter::ValueType>(k_EquivalentDiametersName_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<AttributeMatrixSelectionParameter::ValueType>(k_CellFeatureAttributeMatrixPath_Key);
  inputValues.FeatureIdsPath = filterArgs.value<ArraySelectionParameter::ValueType>(k_CellFeatureIdsArrayPath_Key);
  inputValues.InputImageGeometryPath = filterArgs.value<GeometrySelectionParameter::ValueType>(k_GeometryPath_Key);
  inputValues.NumElementsName = filterArgs.value<DataObjectNameParameter::ValueType>(k_NumElementsName_Key);
  inputValues.SaveElementSizes = filterArgs.value<BoolParameter::ValueType>(k_SaveElementSizes_Key);
  inputValues.VolumesName = filterArgs.value<DataObjectNameParameter::ValueType>(k_VolumesName_Key);

  return ComputeFeatureSizes(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SaveElementSizesKey = "SaveElementSizes";
constexpr StringLiteral k_FeatureIdsArrayPathKey = "FeatureIdsArrayPath";
constexpr StringLiteral k_FeatureAttributeMatrixNameKey = "FeatureAttributeMatrixName";
constexpr StringLiteral k_EquivalentDiametersArrayNameKey = "EquivalentDiametersArrayName";
constexpr StringLiteral k_NumElementsArrayNameKey = "NumElementsArrayName";
constexpr StringLiteral k_VolumesArrayNameKey = "VolumesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ComputeFeatureSizesFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ComputeFeatureSizesFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SaveElementSizesKey, k_SaveElementSizes_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_GeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayPathKey, k_CellFeatureIdsArrayPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::AttributeMatrixSelectionFilterParameterConverter>(args, json, SIMPL::k_FeatureAttributeMatrixNameKey, k_CellFeatureAttributeMatrixPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_EquivalentDiametersArrayNameKey, k_EquivalentDiametersName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_NumElementsArrayNameKey, k_NumElementsName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_VolumesArrayNameKey, k_VolumesName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
