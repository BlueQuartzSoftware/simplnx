#include "ReadZeissTxmFileFilter.hpp"

#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"

#include "NXImageProcessing/Filters/Algorithms/ReadZeissTxmFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <mutex>

using namespace nx::core;
using namespace read_zeiss_txm;

namespace
{
ImageGeometryCropOptions CreateCropOptions(const DataPath& inputGeometryPath, const DataPath& outputGeometryPath, const CropGeometryParameter::ValueType& croppingOptions)
{
  ImageGeometryCropOptions options;
  options.input_image_geometry_path = inputGeometryPath;
  options.output_image_geometry_path = outputGeometryPath;
  options.use_physical_bounds = croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  options.crop_x = croppingOptions.crop_x;
  options.crop_y = croppingOptions.crop_y;
  options.crop_z = croppingOptions.crop_z;
  options.min_voxel = {static_cast<uint64>(croppingOptions.x_bound_voxels[0]), static_cast<uint64>(croppingOptions.y_bound_voxels[0]), static_cast<uint64>(croppingOptions.z_bound_voxels[0])};
  options.max_voxel = {static_cast<uint64>(croppingOptions.x_bound_voxels[1]), static_cast<uint64>(croppingOptions.y_bound_voxels[1]), static_cast<uint64>(croppingOptions.z_bound_voxels[1])};
  options.min_coordinate = {static_cast<float64>(croppingOptions.x_bound_physical[0]), static_cast<float64>(croppingOptions.y_bound_physical[0]), static_cast<float64>(croppingOptions.z_bound_physical[0])};
  options.max_coordinate = {static_cast<float64>(croppingOptions.x_bound_physical[1]), static_cast<float64>(croppingOptions.y_bound_physical[1]), static_cast<float64>(croppingOptions.z_bound_physical[1])};
  return options;
}

std::atomic_int32_t SInstanceId = 0;
std::map<int32, ReadZeissTxmFilterFileCache> SHeaderCache;
std::mutex SHeaderCacheMutex;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadZeissTxmFileFilter::ReadZeissTxmFileFilter()
: m_InstanceId(SInstanceId.fetch_add(1))
{
  std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
  SHeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadZeissTxmFileFilter::~ReadZeissTxmFileFilter() noexcept
{
  std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
  SHeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ReadZeissTxmFileFilter::name() const
{
  return FilterTraits<ReadZeissTxmFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadZeissTxmFileFilter::className() const
{
  return FilterTraits<ReadZeissTxmFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadZeissTxmFileFilter::uuid() const
{
  return FilterTraits<ReadZeissTxmFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadZeissTxmFileFilter::humanName() const
{
  return "Read Zeiss TXM/TXRM Files";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadZeissTxmFileFilter::defaultTags() const
{
  return {"Read", "Import", "Zeiss", "CT", "xCT"};
}

//------------------------------------------------------------------------------
Parameters ReadZeissTxmFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_TxmInputFilePath_Key, "Zeiss TXM/TXRM File", "The input Zeiss TXM/TXRM file", fs::path("input.txm"),
                                                          FileSystemPathParameter::ExtensionsType{".txm", ".txrm"}, FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  params.insert(std::make_unique<CropGeometryParameter>(
      k_CroppingOptions_Key, "Cropping Options",
      "The cropping options used to crop the incoming data.  These include picking the cropping type, the cropping dimensions, and the cropping ranges for each chosen dimension.",
      CropGeometryParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Geometry Metadata Overrides"});
  image_io::InsertGeometryOverrideParameters(params);

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "Path to create the Image Geometry", DataPath({"Zeiss CT"})));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The attribute matrix created as a child of the image geometry", "Cell Data"));
  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CTDataArrayName_Key, "CT Data", "The data array created as a child of the attribute matrix", "CT_Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ReadZeissTxmFileFilter::clone() const
{
  return std::make_unique<ReadZeissTxmFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadZeissTxmFileFilter::parametersVersion() const
{
  // Version 2: Added Set Origin and Set Spacing.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadZeissTxmFileFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                               const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxmInputFilePath_Key);
  auto pNewImageGeometryPathValue = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_CTDataArrayName_Key);
  auto pCroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Check the cache under the lock. If the cached entry is fresh, copy the
  // metadata out so the rest of preflight can work on a local value without
  // holding the cache mutex across actions/IO.
  ZeissTxmHeaderMetadata metadata;
  bool cacheValid = false;
  const auto currentTimeStamp = fs::last_write_time(pInputFilePathValue);
  {
    std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
    auto& cached = SHeaderCache[m_InstanceId];
    if(pInputFilePathValue == cached.input_file && cached.time_stamp >= currentTimeStamp)
    {
      metadata = cached.meta_data;
      cacheValid = true;
    }
  }

  if(!cacheValid)
  {
    Result<ZeissTxmHeaderMetadata> metadataResult = ReadHeaderMetaData(pInputFilePathValue.string());
    if(metadataResult.invalid())
    {
      return {.output_actions = ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
    }
    metadata = metadataResult.value();

    std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
    auto& cached = SHeaderCache[m_InstanceId];
    cached.input_file = pInputFilePathValue.string();
    cached.time_stamp = currentTimeStamp;
    cached.meta_data = metadata;
  }
  if(auto geometryResult = image_io::GeometryOverrides::FromArguments(filterArgs).apply(pInputFilePathValue, metadata.origin, metadata.spacing); geometryResult.invalid())
  {
    return {.output_actions = ConvertInvalidResult<OutputActions>(std::move(geometryResult))};
  }
  preflightUpdatedValues.push_back({"Full Input Geometry", nx::core::geometry_helpers::description::GenerateGeometryInfo(metadata.dimensions, metadata.spacing, metadata.origin, metadata.units)});

  CreateImageGeometryAction::DimensionType dims = metadata.dimensions;
  CreateImageGeometryAction::OriginType origin = metadata.origin;
  CreateImageGeometryAction::SpacingType spacing = metadata.spacing;

  DataStructure tmpDs;
  OutputActions tmpActions;

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeometryPathValue, dims, origin, spacing, pCellAttributeMatrixNameValue);
  tmpActions.appendAction(std::move(createImageGeometryAction));

  // Create the input data array
  const DataPath dap = pNewImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  CreateImageGeometryAction::DimensionType revDims = {dims[2], dims[1], dims[0]};

  if(metadata.data_type == ZeissTxmDataType::FLOAT_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, revDims, std::vector<usize>{1}, dap);
    tmpActions.appendAction(std::move(createArrayAction));
  }
  if(metadata.data_type == ZeissTxmDataType::INT16_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint16, revDims, std::vector<usize>{1}, dap);
    tmpActions.appendAction(std::move(createArrayAction));
  }
  if(metadata.data_type == ZeissTxmDataType::UCHAR_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, revDims, std::vector<usize>{1}, dap);
    tmpActions.appendAction(std::move(createArrayAction));
  }

  Result<> tmpActionsResult = tmpActions.applyAll(tmpDs, IDataAction::Mode::Preflight);
  if(tmpActionsResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tmpActionsResult), {})};
  }

  if(pCroppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping)
  {
    const DataPath croppedGeomPath({pNewImageGeometryPathValue.getTargetName() + "_cropped"});
    const ImageGeometryCropOptions cropOptions = CreateCropOptions(pNewImageGeometryPathValue, croppedGeomPath, pCroppingOptions);
    ImageGeometryCropBounds cropBounds;
    PreflightResult cropImageResult = PreflightImageGeometryCrop(tmpDs, cropOptions, cropBounds);
    if(cropImageResult.output_actions.invalid())
    {
      return cropImageResult;
    }

    Result<> actionsResult = cropImageResult.output_actions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {.output_actions = ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    auto croppedGeom = tmpDs.getDataRefAs<ImageGeom>(croppedGeomPath);
    dims = croppedGeom.getDimensions().toContainer<std::vector<usize>>();
    origin = croppedGeom.getOrigin().toContainer<std::vector<float32>>();
    spacing = croppedGeom.getSpacing().toContainer<std::vector<float32>>();
  }

  createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeometryPathValue, dims, origin, spacing, pCellAttributeMatrixNameValue);
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  // Create the input data array
  revDims = {dims[2], dims[1], dims[0]};

  if(metadata.data_type == ZeissTxmDataType::FLOAT_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, revDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(metadata.data_type == ZeissTxmDataType::INT16_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint16, revDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(metadata.data_type == ZeissTxmDataType::UCHAR_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, revDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::geometry_helpers::description::GenerateGeometryInfo(dims, spacing, origin, metadata.units)});

  return {.output_actions = std::move(resultOutputActions), .output_values = preflightUpdatedValues};
}

//------------------------------------------------------------------------------
Result<> ReadZeissTxmFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ReadZeissTxmFileInputValues inputValues;
  inputValues.geometry_overrides = image_io::GeometryOverrides::FromArguments(filterArgs);

  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.cell_attribute_matrix_name = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.density_array_name = filterArgs.value<std::string>(k_CTDataArrayName_Key);
  inputValues.txm_data_file = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxmInputFilePath_Key);
  inputValues.cropping_options = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  return ReadZeissTxmFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
