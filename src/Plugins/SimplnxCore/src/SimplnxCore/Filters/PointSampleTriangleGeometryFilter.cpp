#include "PointSampleTriangleGeometryFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/PointSampleTriangleGeometry.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateVertexGeometryAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string PointSampleTriangleGeometryFilter::name() const
{
  return FilterTraits<PointSampleTriangleGeometryFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string PointSampleTriangleGeometryFilter::className() const
{
  return FilterTraits<PointSampleTriangleGeometryFilter>::className;
}

//------------------------------------------------------------------------------
Uuid PointSampleTriangleGeometryFilter::uuid() const
{
  return FilterTraits<PointSampleTriangleGeometryFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string PointSampleTriangleGeometryFilter::humanName() const
{
  return "Point Sample Triangle Geometry";
}

//------------------------------------------------------------------------------
std::vector<std::string> PointSampleTriangleGeometryFilter::defaultTags() const
{
  return {className(), "SimplnxCore", "Geometry", "TriangleGeometry", "Resample"};
}

//------------------------------------------------------------------------------
Parameters PointSampleTriangleGeometryFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<Int32ParameterType>(k_NumberOfSamples_Key, "Number of Sample Points", "The number of sample points to use", 1000));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.",
                                                          DataPath{}, ArraySelectionParameter::AllowedTypeSetType{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Random Number Seed Parameters"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseSeed_Key, "Use Seed for Random Generation", "When true the user will be able to put in a seed for random generation", false));
  params.insert(std::make_unique<NumberParameter<uint64>>(k_SeedValue_Key, "Seed Value", "The seed fed into the random generator", std::mt19937::default_seed));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SeedArrayName_Key, "Stored Seed Value Array Name", "Name of array holding the seed value", "PointSampleTriangleGeometry SeedValue"));

  params.insertSeparator(Parameters::Separator{"Input Triangle Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_TriangleGeometry_Key, "Triangle Geometry to Sample", "The complete path to the triangle Geometry from which to sample", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Triangle}));

  params.insertSeparator(Parameters::Separator{"Input Triangle Face Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_TriangleAreasArrayPath_Key, "Face Areas", "The complete path to the array specifying the area of each Face", DataPath{},
                                                          ArraySelectionParameter::AllowedTypeSetType{DataType::float64}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_SelectedDataArrayPaths_Key, "Face Attribute Arrays to Transfer",
                                                               "The paths to the Face Attribute Arrays to transfer to the created Vertex Geometry where the mask is false, if Use Mask is checked",
                                                               MultiArraySelectionParameter::ValueType{}, MultiArraySelectionParameter::AllowedTypeSetType{IArray::ArrayType::DataArray},
                                                               nx::core::GetAllDataTypes()));

  params.insertSeparator(Parameters::Separator{"Output Vertex Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_VertexGeometryPath_Key, "Vertex Geometry Name",
                                                             "The complete path to the DataGroup holding the Vertex Geometry that represents the sampling points", DataPath({"[Vertex Geometry]"})));
  params.insertSeparator(Parameters::Separator{"Output Vertex Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_VertexDataGroupName_Key, "Vertex Data", "The complete path to the vertex data arrays for the Vertex Geometry",
                                                          INodeGeometry0D::k_VertexAttributeMatrixName));

  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);
  params.linkParameters(k_UseSeed_Key, k_SeedValue_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType PointSampleTriangleGeometryFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType PointSampleTriangleGeometryFilter::clone() const
{
  return std::make_unique<PointSampleTriangleGeometryFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult PointSampleTriangleGeometryFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                                          const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pUseMask = filterArgs.value<bool>(k_UseMask_Key);
  auto pTriangleGeometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  auto pTriangleAreasArrayPath = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  auto pMaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pSelectedDataArrayPaths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  auto pVertexGeometryDataPath = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  DataPath pVertexGroupDataPath = pVertexGeometryDataPath.createChildPath(pVertexGroupDataName);
  auto pSeedArrayNameValue = filterArgs.value<std::string>(k_SeedArrayName_Key);

  nx::core::Result<OutputActions> resultOutputActions = {};

  std::vector<PreflightValue> preflightUpdatedValues;

  // Create the Vertex Geometry action and store it
  {
    uint64 numTuples = 0;
    if(!pSelectedDataArrayPaths.empty())
    {
      numTuples = dataStructure.getDataAs<IDataArray>(pSelectedDataArrayPaths[0])->getNumberOfTuples();
    }

    auto createVertexGeometryAction = std::make_unique<CreateVertexGeometryAction>(pVertexGeometryDataPath, numTuples, pVertexGroupDataName, VertexGeom::k_SharedVertexListName);
    resultOutputActions.value().appendAction(std::move(createVertexGeometryAction));
  }

  // Create all the target DataArray based on the Selected Node Arrays
  for(const auto& selectedDataPath : pSelectedDataArrayPaths)
  {
    DataPath createdDataPath = pVertexGroupDataPath.createChildPath(selectedDataPath.getTargetName());
    auto createArrayAction = std::make_unique<CopyArrayInstanceAction>(selectedDataPath, createdDataPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // Collect all the errors
  std::vector<Error> errors;

  // Ensure that if pMaskValue is TRUE that the Mask Path is valid
  if(pUseMask)
  {
    const DataObject* maskArrayPtr = dataStructure.getData(pMaskArrayPath);
    if(nullptr == maskArrayPtr)
    {
      Error result = {.code = -500,
                      .message =
                          fmt::format("'Use Mask Array' is selected but the DataPath '{}' does not exist. Please ensure the mask array exists in the DataStructure.", pMaskArrayPath.toString())};
      errors.push_back(result);
    }
  }

  if(!errors.empty())
  {
    return {nonstd::make_unexpected(std::move(errors))};
  }

  {
    auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, std::vector<usize>{1}, std::vector<usize>{1}, DataPath({pSeedArrayNameValue}));
    resultOutputActions.value().appendAction(std::move(createAction));
  }

  // Return both the resultOutputActions and the preflightUpdatedValues  LinkGeometryDataFilter via std::move()
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> PointSampleTriangleGeometryFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                                        const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto seed = filterArgs.value<std::mt19937_64::result_type>(k_SeedValue_Key);
  if(!filterArgs.value<bool>(k_UseSeed_Key))
  {
    seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  }

  // Store Seed Value in Top Level Array
  dataStructure.getDataRefAs<UInt64ArrayType>(DataPath({filterArgs.value<std::string>(k_SeedArrayName_Key)}))[0] = seed;

  PointSampleTriangleGeometryInputs inputs;

  inputs.p_number_of_samples = filterArgs.value<int32>(k_NumberOfSamples_Key);
  inputs.p_use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputs.p_triangle_geometry = filterArgs.value<DataPath>(k_TriangleGeometry_Key);
  inputs.p_triangle_areas_array_path = filterArgs.value<DataPath>(k_TriangleAreasArrayPath_Key);
  inputs.p_mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputs.p_selected_data_array_paths = filterArgs.value<MultiArraySelectionParameter::ValueType>(k_SelectedDataArrayPaths_Key);
  inputs.p_vertex_geometry_path = filterArgs.value<DataPath>(k_VertexGeometryPath_Key);
  auto pVertexGroupDataName = filterArgs.value<std::string>(k_VertexDataGroupName_Key);
  inputs.p_vertex_group_data_path = inputs.p_vertex_geometry_path.createChildPath(pVertexGroupDataName);
  inputs.seed = seed;

  MultiArraySelectionParameter::ValueType createdDataPaths;
  for(const auto& selectedDataPath : inputs.p_selected_data_array_paths)
  {
    DataPath createdDataPath = inputs.p_vertex_group_data_path.createChildPath(selectedDataPath.getTargetName());
    createdDataPaths.push_back(createdDataPath);
  }
  inputs.p_created_data_array_paths = createdDataPaths;

  return PointSampleTriangleGeometry(dataStructure, &inputs, shouldCancel, messageHandler)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_NumberOfSamplesKey = "NumberOfSamples";
constexpr StringLiteralType k_UseMaskKey = "UseMask";
constexpr StringLiteralType k_TriangleGeometryKey = "TriangleGeometry";
constexpr StringLiteralType k_TriangleAreasArrayPathKey = "TriangleAreasArrayPath";
constexpr StringLiteralType k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteralType k_SelectedDataArrayPathsKey = "SelectedDataArrayPaths";
constexpr StringLiteralType k_VertexGeometryKey = "VertexGeometry";
constexpr StringLiteralType k_VertexAttributeMatrixNameKey = "VertexAttributeMatrixName";
} // namespace simpl
} // namespace

Result<Arguments> PointSampleTriangleGeometryFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = PointSampleTriangleGeometryFilter().getDefaultArguments();

  std::vector<Result<>> results;

  // Samples number type parameter is not applicable in NX
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumberOfSamplesKey, k_NumberOfSamples_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseMaskKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_TriangleGeometryKey, k_TriangleGeometry_Key));
  // ParentGeometry parameter is not applicable in NX
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_TriangleAreasArrayPathKey, k_TriangleAreasArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::MultiDataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedDataArrayPathsKey, k_SelectedDataArrayPaths_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringToDataPathFilterParameterConverter>(args, json, simpl::k_VertexGeometryKey, k_VertexGeometryPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedPathCreationFilterParameterConverter>(args, json, simpl::k_VertexAttributeMatrixNameKey, k_VertexDataGroupName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
