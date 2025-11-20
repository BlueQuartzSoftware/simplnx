#include "ReadZeissTxmFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadZeissTxmFile.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"

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
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;
using namespace read_zeiss_txm;

namespace
{
std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ReadZeissTxmFilterFileCache> s_HeaderCache;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadZeissTxmFileFilter::ReadZeissTxmFileFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadZeissTxmFileFilter::~ReadZeissTxmFileFilter() noexcept
{
  s_HeaderCache.erase(m_InstanceId);
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

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "Path to create the Image Geometry", DataPath({"Zeiss CT"})));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix", "The attribute matrix created as a child of the image geometry", "Cell Data"));
  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CTDataArrayName_Key, "CT Data", "The data array created as a child of the attribute matrix", "CT_Data"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadZeissTxmFileFilter::clone() const
{
  return std::make_unique<ReadZeissTxmFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadZeissTxmFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadZeissTxmFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pInputFilePathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxmInputFilePath_Key);
  auto pNewImageGeometryPathValue = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttributeMatrixNameValue = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pDensityArrayNameValue = filterArgs.value<std::string>(k_CTDataArrayName_Key);
  auto pCroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  // Read from the file if the input file has changed or the input file's time stamp is out of date.
  if(pInputFilePathValue != s_HeaderCache[m_InstanceId].inputFile || s_HeaderCache[m_InstanceId].timeStamp < fs::last_write_time(pInputFilePathValue))
  {
    Result<ZeissTxmHeaderMetadata> metadataResult = ReadHeaderMetaData(pInputFilePathValue.string());
    if(metadataResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
    }

    // Cache the results from algorithm run
    ReadZeissTxmFilterFileCache inputFileCache = {};
    inputFileCache.inputFile = pInputFilePathValue.string();
    inputFileCache.timeStamp = fs::last_write_time(pInputFilePathValue.string());
    inputFileCache.metaData = metadataResult.value();
    s_HeaderCache[m_InstanceId] = inputFileCache;
  }

  ZeissTxmHeaderMetadata& metadata = s_HeaderCache[m_InstanceId].metaData;
  preflightUpdatedValues.push_back({"Full Input Geometry", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(metadata.Dimensions, metadata.Spacing, metadata.Origin, metadata.Units)});

  CreateImageGeometryAction::DimensionType dims = metadata.Dimensions;
  CreateImageGeometryAction::OriginType origin = metadata.Origin;
  CreateImageGeometryAction::SpacingType spacing = metadata.Spacing;

  DataStructure tmpDs;
  OutputActions tmpActions;

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeometryPathValue, dims, origin, spacing, pCellAttributeMatrixNameValue);
  tmpActions.appendAction(std::move(createImageGeometryAction));

  // Create the input data array
  const DataPath dap = pNewImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  CreateImageGeometryAction::DimensionType revDims = {dims[2], dims[1], dims[0]};

  if(metadata.DataType == ZeissTxmDataType::FLOAT_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, revDims, std::vector<usize>{1}, dap);
    tmpActions.appendAction(std::move(createArrayAction));
  }
  if(metadata.DataType == ZeissTxmDataType::INT16_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint16, revDims, std::vector<usize>{1}, dap);
    tmpActions.appendAction(std::move(createArrayAction));
  }
  if(metadata.DataType == ZeissTxmDataType::UCHAR_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, revDims, std::vector<usize>{1}, dap);
    tmpActions.appendAction(std::move(createArrayAction));
  }

  Result<> tmpActionsResult = tmpActions.applyAll(tmpDs, IDataAction::Mode::Preflight);
  if(tmpActionsResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(tmpActionsResult), {})};
  }

  if(pCroppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping)
  {
    CropImageGeometryFilter cropImageGeomFilter;
    Arguments cropImageGeomArgs;
    cropImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(pNewImageGeometryPathValue));
    cropImageGeomArgs.insertOrAssign("use_physical_bounds", std::make_any<bool>(pCroppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume));
    cropImageGeomArgs.insertOrAssign("crop_x_dim", std::make_any<bool>(pCroppingOptions.cropX));
    cropImageGeomArgs.insertOrAssign("crop_y_dim", std::make_any<bool>(pCroppingOptions.cropY));
    cropImageGeomArgs.insertOrAssign("crop_z_dim", std::make_any<bool>(pCroppingOptions.cropZ));
    if(pCroppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume)
    {
      cropImageGeomArgs.insertOrAssign("min_voxel",
                                       std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(pCroppingOptions.xBoundVoxels[0]), static_cast<uint64>(pCroppingOptions.yBoundVoxels[0]),
                                                                                        static_cast<uint64>(pCroppingOptions.zBoundVoxels[0])}));
      cropImageGeomArgs.insertOrAssign("max_voxel",
                                       std::make_any<VectorUInt64Parameter::ValueType>({static_cast<uint64>(pCroppingOptions.xBoundVoxels[1]), static_cast<uint64>(pCroppingOptions.yBoundVoxels[1]),
                                                                                        static_cast<uint64>(pCroppingOptions.zBoundVoxels[1])}));
    }
    else
    {
      cropImageGeomArgs.insertOrAssign(
          "min_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(pCroppingOptions.xBoundPhysical[0]), static_cast<float64>(pCroppingOptions.yBoundPhysical[0]),
                                                                         static_cast<float64>(pCroppingOptions.zBoundPhysical[0])}));
      cropImageGeomArgs.insertOrAssign(
          "max_coord", std::make_any<VectorFloat64Parameter::ValueType>({static_cast<float64>(pCroppingOptions.xBoundPhysical[1]), static_cast<float64>(pCroppingOptions.yBoundPhysical[1]),
                                                                         static_cast<float64>(pCroppingOptions.zBoundPhysical[1])}));
    }
    cropImageGeomArgs.insertOrAssign("remove_original_geometry", std::make_any<bool>(false));
    cropImageGeomArgs.insertOrAssign("output_image_geometry_path", std::make_any<DataPath>(DataPath({pNewImageGeometryPathValue.getTargetName() + "_cropped"})));

    PreflightResult cropImageResult = cropImageGeomFilter.preflight(tmpDs, cropImageGeomArgs, messageHandler, shouldCancel);
    if(cropImageResult.outputActions.invalid())
    {
      return cropImageResult;
    }

    Result<> actionsResult = cropImageResult.outputActions.value().applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(actionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(actionsResult), {})};
    }

    auto croppedGeom = tmpDs.getDataRefAs<ImageGeom>(DataPath({pNewImageGeometryPathValue.getTargetName() + "_cropped"}));
    dims = croppedGeom.getDimensions().toContainer<std::vector<usize>>();
    origin = croppedGeom.getOrigin().toContainer<std::vector<float32>>();
    spacing = croppedGeom.getSpacing().toContainer<std::vector<float32>>();
  }

  createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeometryPathValue, dims, origin, spacing, pCellAttributeMatrixNameValue);
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  // Create the input data array
  revDims = {dims[2], dims[1], dims[0]};

  if(metadata.DataType == ZeissTxmDataType::FLOAT_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, revDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(metadata.DataType == ZeissTxmDataType::INT16_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint16, revDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(metadata.DataType == ZeissTxmDataType::UCHAR_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, revDims, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, metadata.Units)});

  return {std::move(resultOutputActions), preflightUpdatedValues};
}

//------------------------------------------------------------------------------
Result<> ReadZeissTxmFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadZeissTxmFileInputValues inputValues;

  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.DensityArrayName = filterArgs.value<std::string>(k_CTDataArrayName_Key);
  inputValues.TxmDataFile = filterArgs.value<FileSystemPathParameter::ValueType>(k_TxmInputFilePath_Key);
  inputValues.CroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);

  return ReadZeissTxmFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
