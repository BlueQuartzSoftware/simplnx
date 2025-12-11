#include "RegularGridSampleSurfaceMeshFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/RegularGridSampleSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateGeometry1DAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <sstream>

using namespace nx::core;

namespace
{
RegularGridSampleSurfaceMeshFilter::GeometryOption ConvertIndexToGeometryOption(std::underlying_type_t<RegularGridSampleSurfaceMeshFilter::GeometryOption> value)
{
  switch(value)
  {
  case to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::Create): {
    return RegularGridSampleSurfaceMeshFilter::GeometryOption::Create;
  }
  case to_underlying(RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting): {
    return RegularGridSampleSurfaceMeshFilter::GeometryOption::UseExisting;
  }
  }
  throw std::runtime_error(fmt::format("RegularGridSampleSurfaceMeshFilter: Invalid value '{}' for GeometryOption", value));
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::name() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::className() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RegularGridSampleSurfaceMeshFilter::uuid() const
{
  return FilterTraits<RegularGridSampleSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RegularGridSampleSurfaceMeshFilter::humanName() const
{
  return "Sample Triangle Geometry on Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> RegularGridSampleSurfaceMeshFilter::defaultTags() const
{
  return {className(), "Sampling", "Spacing"};
}

//------------------------------------------------------------------------------
Parameters RegularGridSampleSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_UseExistingGeometry_Key, "Use existing geometry", "Should the filter create a new geometry or use an existing one",
                                                                    to_underlying(GeometryOption::Create), ChoicesParameter::Choices{"Create geometry", "Use existing geometry"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_Dimensions_Key, "Dimensions (Voxels)", "The dimensions of the created Image geometry", std::vector<uint64>{128, 128, 128},
                                                        std::vector<std::string>{"x", "y", "z"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the created Image geometry", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "The spacing of the created Image geometry", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Units (For Description Only)", "The units to be displayed below", to_underlying(IGeometry::LengthUnit::Micrometer),
                                                   IGeometry::GetAllLengthUnitStrings()));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryPath_Key, "Triangle Geometry", "The geometry to be sampled onto grid", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{GeometrySelectionParameter::AllowedType ::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Array specifying which Features are on either side of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{nx::core::DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{2}}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ExistingImageGeomPath_Key, "Image Geometry", "The path to the existing image geometry to use", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{GeometrySelectionParameter::AllowedType::Image}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeomPath_Key, "Image Geometry", "The name and path for the image geometry to be created", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAMName_Key, "Cell Attribute Matrix", "The name for the cell data Attribute Matrix within the Image geometry", "Cell Data"));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Feature Ids", "The name for the feature ids array in cell data Attribute Matrix", "Feature Ids"));

  params.linkParameters(k_UseExistingGeometry_Key, k_Dimensions_Key, to_underlying(GeometryOption::Create));
  params.linkParameters(k_UseExistingGeometry_Key, k_Origin_Key, to_underlying(GeometryOption::Create));
  params.linkParameters(k_UseExistingGeometry_Key, k_Spacing_Key, to_underlying(GeometryOption::Create));
  params.linkParameters(k_UseExistingGeometry_Key, k_LengthUnit_Key, to_underlying(GeometryOption::Create));
  params.linkParameters(k_UseExistingGeometry_Key, k_ImageGeomPath_Key, to_underlying(GeometryOption::Create));
  params.linkParameters(k_UseExistingGeometry_Key, k_CellAMName_Key, to_underlying(GeometryOption::Create));

  params.linkParameters(k_UseExistingGeometry_Key, k_ExistingImageGeomPath_Key, to_underlying(GeometryOption::UseExisting));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType RegularGridSampleSurfaceMeshFilter::parametersVersion() const
{
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RegularGridSampleSurfaceMeshFilter::clone() const
{
  return std::make_unique<RegularGridSampleSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RegularGridSampleSurfaceMeshFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto pLengthUnitValue = filterArgs.value<ChoicesParameter::ValueType>(k_LengthUnit_Key);
  auto pImageGeomPathValue = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto pCellAMNameValue = filterArgs.value<std::string>(k_CellAMName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  auto triangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);
  auto geometryOptionIndex = filterArgs.value<ChoicesParameter::ValueType>(k_UseExistingGeometry_Key);
  auto existingImageGeomPathValue = filterArgs.value<DataPath>(k_ExistingImageGeomPath_Key);

  GeometryOption geometryOption = ConvertIndexToGeometryOption(geometryOptionIndex);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  DataPath cellAttributeMatrixPath;
  ShapeType tupleDims;

  if(geometryOption == GeometryOption::Create)
  {
    tupleDims = {static_cast<usize>(pDimensionsValue[0]), static_cast<usize>(pDimensionsValue[1]), static_cast<usize>(pDimensionsValue[2])};

    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeomPathValue, tupleDims, std::vector<float32>(pOriginValue), std::vector<float32>(pSpacingValue), pCellAMNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));

    cellAttributeMatrixPath = pImageGeomPathValue.createChildPath(pCellAMNameValue);

    std::stringstream boxDimensions = std::stringstream();

    std::string lengthUnit = IGeometry::LengthUnitToString(static_cast<IGeometry::LengthUnit>(pLengthUnitValue));

    boxDimensions << "X Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[0] << " to " << std::setprecision(8) << std::noshowpoint
                  << (pOriginValue[0] + (pDimensionsValue[0] * pSpacingValue[0])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[0] * pSpacingValue[0]) << ") "
                  << lengthUnit << "\n";
    boxDimensions << "Y Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[1] << " to " << std::setprecision(8) << std::noshowpoint
                  << (pOriginValue[1] + (pDimensionsValue[1] * pSpacingValue[1])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[1] * pSpacingValue[1]) << ") "
                  << lengthUnit << "\n";
    boxDimensions << "Z Range: " << std::setprecision(8) << std::noshowpoint << pOriginValue[2] << " to " << std::setprecision(8) << std::noshowpoint
                  << (pOriginValue[2] + (pDimensionsValue[2] * pSpacingValue[2])) << " (Delta: " << std::setprecision(8) << std::noshowpoint << (pDimensionsValue[2] * pSpacingValue[2]) << ") "
                  << lengthUnit << "\n";

    float32 vol = (pDimensionsValue[0] * pSpacingValue[0]) * (pDimensionsValue[1] * pSpacingValue[1]) * (pDimensionsValue[2] * pSpacingValue[2]);

    boxDimensions << "Volume: " << std::setprecision(8) << std::noshowpoint << vol << " " << lengthUnit << "s ^3"
                  << "\n";

    preflightUpdatedValues.push_back({"BoxDimensions", boxDimensions.str()});
  }
  else
  {
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(existingImageGeomPathValue);
    cellAttributeMatrixPath = imageGeom.getCellDataPath();
    tupleDims = imageGeom.getCellDataRef().getShape();
  }
  DataPath featIdsPath = cellAttributeMatrixPath.createChildPath(pFeatureIdsArrayNameValue);
  {
    auto createDataGroupAction = std::make_unique<CreateArrayAction>(DataType::int32, std::vector<usize>{tupleDims.rbegin(), tupleDims.rend()}, std::vector<usize>{1}, featIdsPath);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  /////////////////////////////////////////////////////////////////////////////
  // CREATE THE EDGE GEOMETRY THAT WILL BE USED FOR THE POLYGONS
  // This Geometry will be deleted after the filter is completed
  {
    DataPath pSliceDataContainerNameValue({fmt::format(".{}_sliced", triangleGeometryPath.getTargetName())});
    std::string pEdgeAttributeMatrixNameValue("EdgeAttributeMatrix");
    std::string pSliceIdArrayNameValue("SliceIds");
    DataPath pRegionIdArrayPathValue({"NOT USED"});
    std::string pSliceAttributeMatrixNameValue("SliceAttributeMatrix");
    // create the edge geometry
    {
      auto createGeometryAction = std::make_unique<CreateEdgeGeometryAction>(pSliceDataContainerNameValue, 1, 2, INodeGeometry0D::k_VertexAttributeMatrixName, pEdgeAttributeMatrixNameValue,
                                                                             EdgeGeom::k_SharedVertexListName, EdgeGeom::k_SharedEdgeListName);
      resultOutputActions.value().appendAction(std::move(createGeometryAction));
    }

    std::vector<size_t> tDims = {1};
    const std::vector<size_t> compDims = {1};
    {
      DataPath path = pSliceDataContainerNameValue.createChildPath(pEdgeAttributeMatrixNameValue).createChildPath(pSliceIdArrayNameValue);
      auto createArray = std::make_unique<CreateArrayAction>(DataType::int32, tDims, compDims, path);
      resultOutputActions.value().appendAction(std::move(createArray));
    }

    DataPath featureSliceAttrMatPath = pSliceDataContainerNameValue.createChildPath(pSliceAttributeMatrixNameValue);
    {
      auto createAttributeMatrixAction = std::make_unique<CreateAttributeMatrixAction>(featureSliceAttrMatPath, tDims);
      resultOutputActions.value().appendAction(std::move(createAttributeMatrixAction));
    }

    auto deferredDeleteGeometryAction = std::make_unique<DeleteDataAction>(pSliceDataContainerNameValue);
    resultOutputActions.value().appendDeferredAction(std::move(deferredDeleteGeometryAction));
  }
  /////////////////////////////////////////////////////////////////////////////

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RegularGridSampleSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  RegularGridSampleSurfaceMeshInputValues inputValues;

  auto geometryOptionIndex = filterArgs.value<ChoicesParameter::ValueType>(k_UseExistingGeometry_Key);
  GeometryOption geometryOption = ConvertIndexToGeometryOption(geometryOptionIndex);

  auto featureIdsArrayName = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);

  if(geometryOption == GeometryOption::Create)
  {
    auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);

    inputValues.Dimensions = filterArgs.value<VectorUInt64Parameter::ValueType>(k_Dimensions_Key);
    inputValues.Spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
    inputValues.Origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
    inputValues.FeatureIdsArrayPath = imageGeomPath.createChildPath(filterArgs.value<std::string>(k_CellAMName_Key)).createChildPath(featureIdsArrayName);
    inputValues.ImageGeometryOutputPath = imageGeomPath;
  }
  else
  {
    auto existingImageGeomPathValue = filterArgs.value<DataPath>(k_ExistingImageGeomPath_Key);
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(existingImageGeomPathValue);
    inputValues.Dimensions = imageGeom.getDimensions().toContainer<VectorUInt64Parameter::ValueType>();
    inputValues.Spacing = imageGeom.getSpacing().toContainer<VectorFloat32Parameter::ValueType>();
    inputValues.Origin = imageGeom.getOrigin().toContainer<VectorFloat32Parameter::ValueType>();
    inputValues.ImageGeometryOutputPath = existingImageGeomPathValue;
    inputValues.FeatureIdsArrayPath = imageGeom.getCellDataPath().createChildPath(featureIdsArrayName);
  }

  inputValues.TriangleGeometryPath = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);
  inputValues.SurfaceMeshFaceLabelsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);

  return RegularGridSampleSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteral k_DimensionsKey = "Dimensions";
constexpr StringLiteral k_SpacingKey = "Spacing";
constexpr StringLiteral k_ResolutionKey = "Resolution";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_LengthUnitKey = "LengthUnit";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
constexpr StringLiteral k_XPointsKey = "XPoints";
constexpr StringLiteral k_YPointsKey = "YPoints";
constexpr StringLiteral k_ZPointsKey = "ZPoints";
} // namespace SIMPL
} // namespace

Result<Arguments> RegularGridSampleSurfaceMeshFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RegularGridSampleSurfaceMeshFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_TriangleGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  Result<> dimsResult = SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64Vec3FilterParameterConverter>(args, json, SIMPL::k_DimensionsKey, k_Dimensions_Key);
  if(dimsResult.invalid())
  {
    // 6.5 stores dims as 3 parameter
    results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::UInt64ToVec3FilterParameterConverter>(args, json, SIMPL::k_XPointsKey, SIMPL::k_YPointsKey, SIMPL::k_ZPointsKey, k_Dimensions_Key));
  }
  Result<> spacingResult = SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key);
  if(spacingResult.invalid())
  {
    // 6.5 key for spacing was named resolution
    results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_ResolutionKey, k_Spacing_Key));
  }
  else
  {
    results.push_back(std::move(spacingResult));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  Result<> lengthResult = SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_LengthUnitKey, k_LengthUnit_Key);
  if(lengthResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(lengthResult));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DCPathBuilderFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellAMName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
