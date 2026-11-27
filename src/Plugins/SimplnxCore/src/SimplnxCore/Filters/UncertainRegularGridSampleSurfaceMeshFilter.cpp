#include "UncertainRegularGridSampleSurfaceMeshFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/UncertainRegularGridSampleSurfaceMesh.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <random>

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMeshFilter::name() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMeshFilter::className() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::className;
}

//------------------------------------------------------------------------------
Uuid UncertainRegularGridSampleSurfaceMeshFilter::uuid() const
{
  return FilterTraits<UncertainRegularGridSampleSurfaceMeshFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string UncertainRegularGridSampleSurfaceMeshFilter::humanName() const
{
  return "Sample Triangle Geometry on Uncertain Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> UncertainRegularGridSampleSurfaceMeshFilter::defaultTags() const
{
  return {className(), "Sampling", "Spacing", "Surface Mesh", "Grid Interpolation"};
}

//------------------------------------------------------------------------------
Parameters UncertainRegularGridSampleSurfaceMeshFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "UncertainRegularGridSampleSurfaceMesh SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt64ParameterType>(k_Dimensions_Key, "Number of Cells per Axis", "The dimensions of the created Image geometry", std::vector<uint64>{128, 128, 128},
                                                        std::vector<std::string>{"X Points", "Y Points", "Z Points"}));
  params.insert(
      std::make_unique<VectorFloat32ParameterType>(k_Origin_Key, "Origin", "The origin of the created Image geometry", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(
      std::make_unique<VectorFloat32ParameterType>(k_Spacing_Key, "Spacing", "The spacing of the created Image geometry", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<VectorFloat32ParameterType>(k_Uncertainty_Key, "Uncertainty", "uncertainty values associated with X, Y and Z positions of Cells", std::vector<float32>{0.1F, 0.1F, 0.1F},
                                                         std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometryPath_Key, "Triangle Geometry", "The geometry to be sampled onto grid", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{GeometrySelectionParameter::AllowedType ::Triangle}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_SurfaceMeshFaceLabelsArrayPath_Key, "Face Labels", "Array specifying which Features are on either side of each Face", DataPath{},
                                                          nx::core::GetIntegerDataTypes(), ArraySelectionParameter::AllowedComponentShapesType{{2}}));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeomPath_Key, "Image Geometry", "The name and path for the image geometry to be created", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAMName_Key, "Cell Data Name", "The name for the cell data Attribute Matrix within the Image geometry", "Cell Data"));
  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<ChoicesParameter>(k_OutputType_Key, "Output Type for Feature Ids", "The data type for the `Feature Ids` array",
                                                   static_cast<ChoicesParameter::ValueType>(to_underlying(DataType::int32)), GetIntegerDataTypesAsHumanStrings()));
  params.insert(std::make_unique<DataObjectNameParameter>(k_FeatureIdsArrayName_Key, "Feature Ids Name", "The name for the feature ids array in cell data Attribute Matrix", "Feature Ids"));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType UncertainRegularGridSampleSurfaceMeshFilter::parametersVersion() const
{
  return 2;
  // Version 1->2
  // Description:
  // Expanded acceptable output types
  //
  // Change 1:
  // Added - k_OutputType_Key = "output_type";
  // Solution - None, default value preserves backwards functionality
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType UncertainRegularGridSampleSurfaceMeshFilter::clone() const
{
  return std::make_unique<UncertainRegularGridSampleSurfaceMeshFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult UncertainRegularGridSampleSurfaceMeshFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                                    const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pSurfaceMeshFaceLabelsArrayPathValue = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  auto pDimensionsValue = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_Dimensions_Key);
  auto pSpacingValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Spacing_Key);
  auto pOriginValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Origin_Key);
  auto pUncertaintyValue = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Uncertainty_Key);
  auto pImageGeomPathValue = filterArgs.value<DataPath>(k_ImageGeomPath_Key);
  auto pCellAMNameValue = filterArgs.value<std::string>(k_CellAMName_Key);
  auto pFeatureIdsArrayNameValue = filterArgs.value<std::string>(k_FeatureIdsArrayName_Key);
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);
  auto pOutputTypeValue = filterArgs.value<ChoicesParameter::ValueType>(k_OutputType_Key);

  nx::core::Result<OutputActions> resultOutputActions;

  ShapeType imageDims = {static_cast<usize>(pDimensionsValue[0]), static_cast<usize>(pDimensionsValue[1]), static_cast<usize>(pDimensionsValue[2])};
  ShapeType tupleDims = {imageDims[2], imageDims[1], imageDims[0]};

  {
    auto createDataGroupAction = std::make_unique<CreateImageGeometryAction>(pImageGeomPathValue, imageDims, std::vector<float32>(pOriginValue), std::vector<float32>(pSpacingValue), pCellAMNameValue);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }
  DataPath cellAMPath = pImageGeomPathValue.createChildPath(pCellAMNameValue);
  DataPath featIdsPath = cellAMPath.createChildPath(pFeatureIdsArrayNameValue);
  {
    auto createDataGroupAction = std::make_unique<CreateArrayAction>(static_cast<DataType>(pOutputTypeValue), tupleDims, std::vector<usize>{1}, featIdsPath);
    resultOutputActions.value().appendAction(std::move(createDataGroupAction));
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> UncertainRegularGridSampleSurfaceMeshFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/,
                                                                  const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  UncertainRegularGridSampleSurfaceMeshInputValues inputValues;

  inputValues.seed_value = seed;
  inputValues.dimensions = filterArgs.value<VectorUInt64ParameterType::ValueType>(k_Dimensions_Key);
  inputValues.spacing = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Spacing_Key);
  inputValues.origin = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Origin_Key);
  inputValues.uncertainty = filterArgs.value<VectorFloat32ParameterType::ValueType>(k_Uncertainty_Key);
  inputValues.triangle_geometry_path = filterArgs.value<DataPath>(k_TriangleGeometryPath_Key);
  inputValues.surface_mesh_face_labels_array_path = filterArgs.value<DataPath>(k_SurfaceMeshFaceLabelsArrayPath_Key);
  inputValues.feature_ids_array_path =
      filterArgs.value<DataPath>(k_ImageGeomPath_Key).createChildPath(filterArgs.value<std::string>(k_CellAMName_Key)).createChildPath(filterArgs.value<std::string>(k_FeatureIdsArrayName_Key));

  return UncertainRegularGridSampleSurfaceMesh(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_SurfaceMeshFaceLabelsArrayPathKey = "SurfaceMeshFaceLabelsArrayPath";
constexpr StringLiteralType k_XPointsKey = "XPoints";
constexpr StringLiteralType k_YPointsKey = "YPoints";
constexpr StringLiteralType k_ZPointsKey = "ZPoints";
constexpr StringLiteralType k_SpacingKey = "Spacing";
constexpr StringLiteralType k_OriginKey = "Origin";
constexpr StringLiteralType k_UncertaintyKey = "Uncertainty";
constexpr StringLiteralType k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteralType k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteralType k_FeatureIdsArrayNameKey = "FeatureIdsArrayName";
} // namespace simpl
} // namespace

Result<Arguments> UncertainRegularGridSampleSurfaceMeshFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = UncertainRegularGridSampleSurfaceMeshFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceLabelsArrayPathKey, k_TriangleGeometryPath_Key));
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SurfaceMeshFaceLabelsArrayPathKey, k_SurfaceMeshFaceLabelsArrayPath_Key));
  results.push_back(
      simpl_conversion::Convert3Parameters<simpl_conversion::UInt64ToVec3FilterParameterConverterType>(args, json, simpl::k_XPointsKey, simpl::k_YPointsKey, simpl::k_ZPointsKey, k_Dimensions_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_SpacingKey, k_Spacing_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_OriginKey, k_Origin_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::FloatVec3FilterParameterConverterType>(args, json, simpl::k_UncertaintyKey, k_Uncertainty_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DCPathBuilderFilterParameterConverter>(args, json, simpl::k_DataContainerNameKey, k_ImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_CellAttributeMatrixNameKey, k_CellAMName_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_FeatureIdsArrayNameKey, k_FeatureIdsArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
