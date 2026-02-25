#include "InterpolatePointCloudToRegularGridFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/InterpolatePointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace nx::core
{

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::name() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::name;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::className() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::className;
}

//------------------------------------------------------------------------------
Uuid InterpolatePointCloudToRegularGridFilter::uuid() const
{
  return FilterTraits<InterpolatePointCloudToRegularGridFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string InterpolatePointCloudToRegularGridFilter::humanName() const
{
  return "Interpolate Point Cloud to Regular Grid";
}

//------------------------------------------------------------------------------
std::vector<std::string> InterpolatePointCloudToRegularGridFilter::defaultTags() const
{
  return {className(), "Geometry", "Gaussian", "Kernel", "Interpolation", "Point Cloud", "Vertex Geometry"};
}

//------------------------------------------------------------------------------
Parameters InterpolatePointCloudToRegularGridFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Specifies whether or not to use a mask array", false));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_InterpolationTechnique_Key, "Interpolation Technique", "Selected Interpolation Technique", 0, std::vector<std::string>{"Uniform", "Gaussian"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_KernelSize_Key, "Kernel Size", "Specifies the kernel size", std::vector<float32>{1.0f, 1.0f, 1.0f}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GaussianSigmas_Key, "Gaussian Sigmas", "Specifies the Gaussian sigmas", std::vector<float32>{1.0f, 1.0f, 1.0f},
                                                         std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Destination Image Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Interpolated Image Geometry", "DataPath to interpolated geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));

  params.insertSeparator(Parameters::Separator{"Input Vertex Geometry"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Vertex Geometry to Interpolate", "DataPath to geometry to interpolate", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_VoxelIndicesPath_Key, "Voxel Indices", "DataPath to voxel indices", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<ArraySelectionParameter>(k_InputMaskPath_Key, "Mask", "DataPath to the boolean mask array. Values that are true will mark that vertex as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InterpolateArrays_Key, "Attribute Arrays to Interpolate", "DataPaths to interpolate", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyArrays_Key, "Attribute Arrays to Copy", "DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  // params.insert(std::make_unique<DataObjectNameParameter>(k_InterpolatedGroupName_Key, "Interpolated Group", "DataPath to created DataGroup for interpolated data", "InterpolatedData"));

  params.insertSeparator(Parameters::Separator{"Statistics Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindLength_Key, "Find Length", "Compute the number of contributions per voxel", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMin_Key, "Find Minimum", "Compute the minimum weighted value per voxel", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMax_Key, "Find Maximum", "Compute the maximum weighted value per voxel", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindMean_Key, "Find Mean", "Compute the mean weighted value per voxel", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindStdDeviation_Key, "Find Standard Deviation", "Compute the standard deviation of weighted values per voxel", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_FindSummation_Key, "Find Summation", "Compute the sum of weighted values per voxel", false));

  params.insert(std::make_unique<DataObjectNameParameter>(k_LengthSuffix_Key, "Length Suffix", "Suffix appended to array name for length output", "_Length"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MinSuffix_Key, "Minimum Suffix", "Suffix appended to array name for minimum output", "_Minimum"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MaxSuffix_Key, "Maximum Suffix", "Suffix appended to array name for maximum output", "_Maximum"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_MeanSuffix_Key, "Mean Suffix", "Suffix appended to array name for mean output", "_Mean"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_StdDeviationSuffix_Key, "Standard Deviation Suffix", "Suffix appended to array name for standard deviation output", "_StdDeviation"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SummationSuffix_Key, "Summation Suffix", "Suffix appended to array name for summation output", "_Summation"));

  // Link parameters
  params.linkParameters(k_UseMask_Key, k_InputMaskPath_Key, std::make_any<bool>(true));
  params.linkParameters(k_InterpolationTechnique_Key, k_GaussianSigmas_Key, std::make_any<uint64>(InterpolatePointCloudToRegularGrid::k_Gaussian));
  params.linkParameters(k_FindLength_Key, k_LengthSuffix_Key, std::make_any<bool>(true));
  params.linkParameters(k_FindMin_Key, k_MinSuffix_Key, std::make_any<bool>(true));
  params.linkParameters(k_FindMax_Key, k_MaxSuffix_Key, std::make_any<bool>(true));
  params.linkParameters(k_FindMean_Key, k_MeanSuffix_Key, std::make_any<bool>(true));
  params.linkParameters(k_FindStdDeviation_Key, k_StdDeviationSuffix_Key, std::make_any<bool>(true));
  params.linkParameters(k_FindSummation_Key, k_SummationSuffix_Key, std::make_any<bool>(true));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType InterpolatePointCloudToRegularGridFilter::parametersVersion() const
{
  return 3;
  // Version 3 Changes
  // Removed the 'interpolated_group_name' key - The data is all stored in the Cell Data of the target ImageGeom
}

//------------------------------------------------------------------------------
IFilter::UniquePointer InterpolatePointCloudToRegularGridFilter::clone() const
{
  return std::make_unique<InterpolatePointCloudToRegularGridFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult InterpolatePointCloudToRegularGridFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto interpolationTechnique = filterArgs.value<uint64>(k_InterpolationTechnique_Key);
  auto kernelSize = filterArgs.value<std::vector<float32>>(k_KernelSize_Key);
  auto sigmas = filterArgs.value<std::vector<float32>>(k_GaussianSigmas_Key);

  // Input Image Geometry
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  // Input Vertex Geometry and Data
  auto useMask = filterArgs.value<bool>(k_UseMask_Key);
  auto vertexGeomPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  // auto interpolatedGroupName = filterArgs.value<std::string>(k_InterpolatedGroupName_Key);
  auto voxelIndicesPath = filterArgs.value<DataPath>(k_VoxelIndicesPath_Key);
  auto interpolatedDataPaths = filterArgs.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  auto copyDataPaths = filterArgs.value<std::vector<DataPath>>(k_CopyArrays_Key);

  // Statistics Options
  auto findLength = filterArgs.value<bool>(k_FindLength_Key);
  auto findMin = filterArgs.value<bool>(k_FindMin_Key);
  auto findMax = filterArgs.value<bool>(k_FindMax_Key);
  auto findMean = filterArgs.value<bool>(k_FindMean_Key);
  auto findStdDeviation = filterArgs.value<bool>(k_FindStdDeviation_Key);
  auto findSummation = filterArgs.value<bool>(k_FindSummation_Key);

  auto lengthSuffix = filterArgs.value<std::string>(k_LengthSuffix_Key);
  auto minSuffix = filterArgs.value<std::string>(k_MinSuffix_Key);
  auto maxSuffix = filterArgs.value<std::string>(k_MaxSuffix_Key);
  auto meanSuffix = filterArgs.value<std::string>(k_MeanSuffix_Key);
  auto stdDeviationSuffix = filterArgs.value<std::string>(k_StdDeviationSuffix_Key);
  auto summationSuffix = filterArgs.value<std::string>(k_SummationSuffix_Key);

  OutputActions actions;

  if(kernelSize[0] < 0 || kernelSize[1] < 0 || kernelSize[2] < 0)
  {
    return MakePreflightErrorResult(-11000, fmt::format("All kernel dimensions must be positive.\n "
                                                        "Current kernel dimensions:\n x = {}\n y = {}\n z = {}\n",
                                                        kernelSize[0], kernelSize[1], kernelSize[2]));
  }

  if(sigmas[0] <= 0 || sigmas[1] <= 0 || sigmas[2] <= 0)
  {
    return MakePreflightErrorResult(-11000, fmt::format("All sigmas must be positive.\n "
                                                        "Current sigmas:\n x = {}\n y = {}\n z = {}\n",
                                                        sigmas[0], sigmas[1], sigmas[2]));
  }

  const ImageGeom imageGeomRef = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const DataPath interpolatedGroupPath = imageGeomRef.getCellDataPath();
  const SizeVec3 imageDims = imageGeomRef.getDimensions();
  ShapeType tupleDims = {imageDims[2], imageDims[1], imageDims[0]};

  auto vertexGeom = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  std::vector<DataPath> dataArrays = {vertexGeomPath.createChildPath(vertexGeom->getVertices()->getName()), voxelIndicesPath};

  // Create flat DataArrays for interpolated arrays (output type = float64)
  for(const auto& interpolatePath : interpolatedDataPaths)
  {
    dataArrays.push_back(interpolatePath);

    auto srcDataArrayPtr = dataStructure.getDataAs<IDataArray>(interpolatePath);
    if(srcDataArrayPtr->getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(-11002, fmt::format("Attribute Arrays selected for interpolation must be scalar arrays"));
    }
    auto dataType = srcDataArrayPtr->getDataType();
    const std::string& destArrayName = srcDataArrayPtr->getName();

    // Create the interpolated (weighted average) output array as float64
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName);
      auto createAction = std::make_unique<CreateArrayAction>(dataType, tupleDims, srcDataArrayPtr->getComponentShape(), arrayPath);
      actions.appendAction(std::move(createAction));
    }

    // Create statistics output arrays
    if(findLength)
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName + lengthSuffix);
      auto createAction = std::make_unique<CreateArrayAction>(DataType::uint64, tupleDims, ShapeType{1}, arrayPath);
      actions.appendAction(std::move(createAction));
    }
    if(findMin)
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName + minSuffix);
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, ShapeType{1}, arrayPath);
      actions.appendAction(std::move(createAction));
    }
    if(findMax)
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName + maxSuffix);
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, ShapeType{1}, arrayPath);
      actions.appendAction(std::move(createAction));
    }
    if(findMean)
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName + meanSuffix);
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, ShapeType{1}, arrayPath);
      actions.appendAction(std::move(createAction));
    }
    if(findStdDeviation)
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName + stdDeviationSuffix);
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, ShapeType{1}, arrayPath);
      actions.appendAction(std::move(createAction));
    }
    if(findSummation)
    {
      auto arrayPath = interpolatedGroupPath.createChildPath(destArrayName + summationSuffix);
      auto createAction = std::make_unique<CreateArrayAction>(DataType::float32, tupleDims, ShapeType{1}, arrayPath);
      actions.appendAction(std::move(createAction));
    }
  }

  // Create flat DataArrays for copy arrays (matching source type)
  for(const auto& copyPath : copyDataPaths)
  {
    dataArrays.push_back(copyPath);

    auto srcDataArrayPtr = dataStructure.getDataAs<IDataArray>(copyPath);
    if(srcDataArrayPtr->getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(-11002, fmt::format("Attribute Arrays selected for copying must be scalar arrays"));
    }
    auto dataType = srcDataArrayPtr->getDataType();
    auto destArrayDataPath = interpolatedGroupPath.createChildPath(srcDataArrayPtr->getName());
    auto createAction = std::make_unique<CreateArrayAction>(dataType, tupleDims, srcDataArrayPtr->getComponentShape(), destArrayDataPath);
    actions.appendAction(std::move(createAction));
  }

  // Validate input arrays have matching tuples
  if(useMask)
  {
    dataArrays.push_back(filterArgs.value<DataPath>(k_InputMaskPath_Key));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrays);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-11003, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  InterpolatePointCloudToRegularGridInputValues inputValues;

  inputValues.interpolationTechnique = filterArgs.value<uint64>(k_InterpolationTechnique_Key);
  inputValues.vertexGeomPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  inputValues.imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.interpolatedDataPaths = filterArgs.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  inputValues.copyDataPaths = filterArgs.value<std::vector<DataPath>>(k_CopyArrays_Key);
  inputValues.voxelIndicesPath = filterArgs.value<DataPath>(k_VoxelIndicesPath_Key);
  inputValues.kernelSize = filterArgs.value<std::vector<float32>>(k_KernelSize_Key);
  inputValues.sigmas = filterArgs.value<std::vector<float32>>(k_GaussianSigmas_Key);
  inputValues.useMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.maskDataPath = filterArgs.value<DataPath>(k_InputMaskPath_Key);

  inputValues.findLength = filterArgs.value<bool>(k_FindLength_Key);
  inputValues.findMin = filterArgs.value<bool>(k_FindMin_Key);
  inputValues.findMax = filterArgs.value<bool>(k_FindMax_Key);
  inputValues.findMean = filterArgs.value<bool>(k_FindMean_Key);
  inputValues.findStdDeviation = filterArgs.value<bool>(k_FindStdDeviation_Key);
  inputValues.findSummation = filterArgs.value<bool>(k_FindSummation_Key);

  inputValues.lengthSuffix = filterArgs.value<std::string>(k_LengthSuffix_Key);
  inputValues.minSuffix = filterArgs.value<std::string>(k_MinSuffix_Key);
  inputValues.maxSuffix = filterArgs.value<std::string>(k_MaxSuffix_Key);
  inputValues.meanSuffix = filterArgs.value<std::string>(k_MeanSuffix_Key);
  inputValues.stdDeviationSuffix = filterArgs.value<std::string>(k_StdDeviationSuffix_Key);
  inputValues.summationSuffix = filterArgs.value<std::string>(k_SummationSuffix_Key);

  return InterpolatePointCloudToRegularGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_InterpolationTechniqueKey = "InterpolationTechnique";
constexpr StringLiteral k_KernelSizeKey = "KernelSize";
constexpr StringLiteral k_SigmasKey = "Sigmas";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_InterpolatedDataContainerNameKey = "InterpolatedDataContainerName";
constexpr StringLiteral k_VoxelIndicesArrayPathKey = "VoxelIndicesArrayPath";
constexpr StringLiteral k_MaskArrayPathKey = "MaskArrayPath";
constexpr StringLiteral k_ArraysToInterpolateKey = "ArraysToInterpolate";
constexpr StringLiteral k_ArraysToCopyKey = "ArraysToCopy";
constexpr StringLiteral k_InterpolatedAttributeMatrixNameKey = "InterpolatedAttributeMatrixName";
} // namespace SIMPL
} // namespace

Result<Arguments> InterpolatePointCloudToRegularGridFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = InterpolatePointCloudToRegularGridFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedChoicesFilterParameterConverter>(args, json, SIMPL::k_InterpolationTechniqueKey, k_InterpolationTechnique_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_KernelSizeKey, k_KernelSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SigmasKey, k_GaussianSigmas_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_SelectedImageGeometryPath_Key));
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_InterpolatedDataContainerNameKey, k_SelectedVertexGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_VoxelIndicesArrayPathKey, k_VoxelIndicesPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_MaskArrayPathKey, k_InputMaskPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ArraysToInterpolateKey, k_InterpolateArrays_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::MultiDataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ArraysToCopyKey, k_CopyArrays_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
