#include "ReadZeissTxmFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadZeissTxmFile.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

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

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_Use_SubVolume_Key, "Use Z Sub-volume", "Only import a sub-set of the slices", false));
  params.insert(std::make_unique<UInt32Parameter>(k_SubVolumeStartSlice_Key, "Slice Start", "The starting slice to import", 1));
  params.insert(std::make_unique<UInt32Parameter>(k_SubVolumeEndSlice_Key, "Slice End (inclusive)", "The ending slice to import (inclusive)", 2));
  params.linkParameters(k_Use_SubVolume_Key, k_SubVolumeStartSlice_Key, true);
  params.linkParameters(k_Use_SubVolume_Key, k_SubVolumeEndSlice_Key, true);

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

  auto pUseSubVolume = filterArgs.value<bool>(k_Use_SubVolume_Key);
  auto pSliceStart = filterArgs.value<uint32>(k_SubVolumeStartSlice_Key);
  auto pSliceEnd = filterArgs.value<uint32>(k_SubVolumeEndSlice_Key);

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

  // Sanity Check Sub-Volumes
  if(pUseSubVolume && pSliceEnd < pSliceStart)
  {
    return {MakeErrorResult<OutputActions>(-33530, fmt::format("The start slice '{}' is greater than the slice end '{}'", pSliceStart, pSliceEnd)), preflightUpdatedValues};
  }
  if(pUseSubVolume && pSliceStart >= metadata.Dimensions[2])
  {
    return {MakeErrorResult<OutputActions>(-33531, fmt::format("The start slice '{}' is greater than the total number of slices", pSliceStart, metadata.Dimensions[2])), preflightUpdatedValues};
  }
  if(pUseSubVolume && pSliceEnd > metadata.Dimensions[2])
  {
    return {MakeErrorResult<OutputActions>(-33532, fmt::format("The end slice '{}' is greater than the total number of slices", pSliceEnd, metadata.Dimensions[2])), preflightUpdatedValues};
  }
  if(pUseSubVolume && pSliceStart < 1)
  {
    return {MakeErrorResult<OutputActions>(-33533, fmt::format("The start slice '{}' must be 0 or greater", pSliceStart)), preflightUpdatedValues};
  }

  CreateImageGeometryAction::DimensionType finalDimensions = metadata.Dimensions;
  CreateImageGeometryAction::OriginType finalOrigin = metadata.Origin;
  // If we are using a sub-volume, update the dimensions
  if(pUseSubVolume)
  {
    finalDimensions[2] = pSliceEnd - pSliceStart + 1;
    finalOrigin[2] = metadata.Spacing[2] * static_cast<float>(pSliceStart - 1);
  }

  auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pNewImageGeometryPathValue, finalDimensions, finalOrigin, metadata.Spacing, pCellAttributeMatrixNameValue);
  resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

  // Create the input data array
  const DataPath dap = pNewImageGeometryPathValue.createChildPath(pCellAttributeMatrixNameValue).createChildPath(pDensityArrayNameValue);
  CreateImageGeometryAction::DimensionType revDimensions = {finalDimensions[2], finalDimensions[1], finalDimensions[0]};

  if(metadata.DataType == ZeissTxmDataType::FLOAT_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, revDimensions, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(metadata.DataType == ZeissTxmDataType::INT16_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint16, revDimensions, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }
  if(metadata.DataType == ZeissTxmDataType::UCHAR_TYPE)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::uint8, revDimensions, std::vector<usize>{1}, dap);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(finalDimensions, metadata.Spacing, finalOrigin, metadata.Units)});

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

  inputValues.UseSubVolume = filterArgs.value<bool>(k_Use_SubVolume_Key);
  inputValues.SubVolumeStartSlice = filterArgs.value<uint32>(k_SubVolumeStartSlice_Key);
  inputValues.SubVolumeEndSlice = filterArgs.value<uint32>(k_SubVolumeEndSlice_Key);

  return ReadZeissTxmFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
