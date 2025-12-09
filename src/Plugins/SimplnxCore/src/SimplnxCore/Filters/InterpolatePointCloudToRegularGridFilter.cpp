#include "InterpolatePointCloudToRegularGridFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/InterpolatePointCloudToRegularGrid.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/Geometry/VertexGeom.hpp"
#include "simplnx/Filter/Actions/CopyArrayInstanceAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateNeighborListAction.hpp"
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
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_StoreKernelDistances_Key, "Store Kernel Distances", "Specifies whether or not to store kernel distances", false));
  params.insertLinkableParameter(
      std::make_unique<ChoicesParameter>(k_InterpolationTechnique_Key, "Interpolation Technique", "Selected Interpolation Technique", 0, std::vector<std::string>{"Uniform", "Gaussian"}));
  params.insert(
      std::make_unique<VectorFloat32Parameter>(k_KernelSize_Key, "Kernel Size", "Specifies the kernel size", std::vector<float32>{1.0f, 1.0f, 1.0f}, std::vector<std::string>{"x", "y", "z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_GaussianSigmas_Key, "Gaussian Sigmas", "Specifies the Gaussian sigmas", std::vector<float32>{1.0f, 1.0f, 1.0f},
                                                         std::vector<std::string>{"x", "y", "z"}));

  params.insertSeparator(Parameters::Separator{"Input Data Objects"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedVertexGeometryPath_Key, "Vertex Geometry to Interpolate", "DataPath to geometry to interpolate", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Vertex}));
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Interpolated Image Geometry", "DataPath to interpolated geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_VoxelIndicesPath_Key, "Voxel Indices", "DataPath to voxel indices", DataPath{}, ArraySelectionParameter::AllowedTypes{DataType::uint64},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputMaskPath_Key, "Mask", "DataPath to the boolean mask array. Values that are true will mark that cell/point as usable.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_InterpolateArrays_Key, "Attribute Arrays to Interpolate", "DataPaths to interpolate", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllNumericTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<MultiArraySelectionParameter>(k_CopyArrays_Key, "Attribute Arrays to Copy", "DataPaths to copy", std::vector<DataPath>(),
                                                               MultiArraySelectionParameter::AllowedTypes{IArray::ArrayType::DataArray}, GetAllDataTypes(),
                                                               MultiArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_InterpolatedGroupName_Key, "Interpolated Group", "DataPath to created DataGroup for interpolated data", "InterpolatedData"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_KernelDistancesArrayName_Key, "Kernel Distances Group", "DataPath to created DataGroup for kernel distances data", "KernelDistances"));

  params.linkParameters(k_UseMask_Key, k_InputMaskPath_Key, std::make_any<bool>(true));
  params.linkParameters(k_StoreKernelDistances_Key, k_KernelDistancesArrayName_Key, std::make_any<bool>(true));
  params.linkParameters(k_InterpolationTechnique_Key, k_GaussianSigmas_Key, std::make_any<uint64>(InterpolatePointCloudToRegularGrid::k_Gaussian));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType InterpolatePointCloudToRegularGridFilter::parametersVersion() const
{
  return 1;
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
  auto useMask = filterArgs.value<bool>(k_UseMask_Key);
  auto storeKernelDistances = filterArgs.value<bool>(k_StoreKernelDistances_Key);
  auto interpolationTechnique = filterArgs.value<uint64>(k_InterpolationTechnique_Key);
  auto vertexGeomPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  auto interpolatedGroupName = filterArgs.value<std::string>(k_InterpolatedGroupName_Key);
  auto voxelIndicesPath = filterArgs.value<DataPath>(k_VoxelIndicesPath_Key);
  auto interpolatedDataPaths = filterArgs.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  auto copyDataPaths = filterArgs.value<std::vector<DataPath>>(k_CopyArrays_Key);
  auto kernelSize = filterArgs.value<std::vector<float32>>(k_KernelSize_Key);
  auto sigmas = filterArgs.value<std::vector<float32>>(k_GaussianSigmas_Key);

  OutputActions actions;

  if(interpolationTechnique != InterpolatePointCloudToRegularGrid::k_Uniform && interpolationTechnique != InterpolatePointCloudToRegularGrid::k_Gaussian)
  {
    return MakePreflightErrorResult(-11000, fmt::format("Interpolation Technique must be 0 [Uniform] or 1 [Gaussian] "));
  }

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

  const DataPath interpolatedGroupPath = imageGeomPath.createChildPath(interpolatedGroupName);
  auto vertexGeom = dataStructure.getDataAs<VertexGeom>(vertexGeomPath);
  auto image = dataStructure.getDataAs<ImageGeom>(imageGeomPath);
  const SizeVec3 imageDims = image->getDimensions();
  ShapeType tupleDims = {imageDims[2], imageDims[1], imageDims[0]};

  // Create the attribute matrix for storing the interpolated/copied arrays
  {
    auto createGroupAction = std::make_unique<CreateAttributeMatrixAction>(interpolatedGroupPath, tupleDims);
    actions.appendAction(std::move(createGroupAction));
  }

  std::vector<DataPath> dataArrays = {vertexGeomPath.createChildPath(vertexGeom->getVertices()->getName()), voxelIndicesPath};

  // Create the neighbor list arrays for storing the interpolated array data
  for(const auto& interpolatePath : interpolatedDataPaths)
  {
    dataArrays.push_back(interpolatePath);

    auto targetArray = dataStructure.getDataAs<IDataArray>(interpolatePath);
    auto targetPath = interpolatedGroupPath.createChildPath(targetArray->getName());
    if(targetArray->getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(-11002, fmt::format("Attribute Arrays selected for copying must be scalar arrays"));
    }
    auto dataType = targetArray->getDataType();
    if(dataType != DataType::boolean)
    {
      auto neighborPath = interpolatedGroupPath.createChildPath(targetArray->getName());
      auto neighborAction = std::make_unique<CreateNeighborListAction>(dataType, tupleDims, neighborPath);
      actions.appendAction(std::move(neighborAction));
    }
  }

  // Create the neighbor list arrays for storing the copied array data
  for(const auto& copyPath : copyDataPaths)
  {
    dataArrays.push_back(copyPath);

    auto targetArray = dataStructure.getDataAs<IDataArray>(copyPath);
    auto targetPath = interpolatedGroupPath.createChildPath(targetArray->getName());
    if(targetArray->getNumberOfComponents() != 1)
    {
      return MakePreflightErrorResult(-11002, fmt::format("Attribute Arrays selected for copying must be scalar arrays"));
    }
    auto dataType = targetArray->getDataType();
    if(dataType != DataType::boolean)
    {
      auto neighborPath = interpolatedGroupPath.createChildPath(targetArray->getName());
      auto neighborAction = std::make_unique<CreateNeighborListAction>(dataType, tupleDims, neighborPath);
      actions.appendAction(std::move(neighborAction));
    }
  }

  // validate the input arrays have matching tuples (i.e. it should all come from the input vertex geometry's vertex data)
  if(useMask)
  {
    dataArrays.push_back(filterArgs.value<DataPath>(k_InputMaskPath_Key));
  }

  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataArrays);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-11003, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  // Create the neighbor list array for storing the kernel distances
  if(storeKernelDistances)
  {
    auto action = std::make_unique<CreateNeighborListAction>(DataType::float32, tupleDims, interpolatedGroupPath.createChildPath(filterArgs.value<std::string>(k_KernelDistancesArrayName_Key)));
    actions.appendAction(std::move(action));
  }

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> InterpolatePointCloudToRegularGridFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  InterpolatePointCloudToRegularGridInputValues inputValues;

  inputValues.storeKernelDistances = filterArgs.value<bool>(k_StoreKernelDistances_Key);
  inputValues.interpolationTechnique = filterArgs.value<uint64>(k_InterpolationTechnique_Key);
  inputValues.vertexGeomPath = filterArgs.value<DataPath>(k_SelectedVertexGeometryPath_Key);
  inputValues.imageGeomPath = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);
  inputValues.interpolatedGroupName = filterArgs.value<std::string>(k_InterpolatedGroupName_Key);
  inputValues.interpolatedDataPaths = filterArgs.value<std::vector<DataPath>>(k_InterpolateArrays_Key);
  inputValues.copyDataPaths = filterArgs.value<std::vector<DataPath>>(k_CopyArrays_Key);
  inputValues.voxelIndicesPath = filterArgs.value<DataPath>(k_VoxelIndicesPath_Key);
  inputValues.kernelSize = filterArgs.value<std::vector<float32>>(k_KernelSize_Key);
  inputValues.sigmas = filterArgs.value<std::vector<float32>>(k_GaussianSigmas_Key);
  inputValues.kernelDistanceArrayName = filterArgs.value<std::string>(k_KernelDistancesArrayName_Key);
  inputValues.useMask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.maskDataPath = filterArgs.value<DataPath>(k_InputMaskPath_Key);
  return InterpolatePointCloudToRegularGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_UseMaskKey = "UseMask";
constexpr StringLiteral k_StoreKernelDistancesKey = "StoreKernelDistances";
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
constexpr StringLiteral k_KernelDistancesArrayNameKey = "KernelDistancesArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> InterpolatePointCloudToRegularGridFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = InterpolatePointCloudToRegularGridFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_UseMaskKey, k_UseMask_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedBooleanFilterParameterConverter>(args, json, SIMPL::k_StoreKernelDistancesKey, k_StoreKernelDistances_Key));
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
  results.push_back(
      SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_InterpolatedAttributeMatrixNameKey, k_InterpolatedGroupName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_KernelDistancesArrayNameKey, k_KernelDistancesArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}

} // namespace nx::core
