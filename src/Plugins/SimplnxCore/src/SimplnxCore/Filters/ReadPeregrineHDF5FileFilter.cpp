#include "ReadPeregrineHDF5FileFilter.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateAttributeMatrixAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/Parsing/HDF5/H5.hpp"
#include "simplnx/Utilities/Parsing/HDF5/Readers/FileReader.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadPeregrineHDF5File.hpp"

using namespace nx::core;
namespace fs = std::filesystem;

namespace
{
const std::string k_XRealDimensionPath = "printer/x_real_dimension";
const std::string k_YRealDimensionPath = "printer/y_real_dimension";
const std::string k_XCameraDimensionPath = "printer/x_camera_dimension";
const std::string k_YCameraDimensionPath = "printer/y_camera_dimension";
const std::string k_LayerThicknessPath = "material/layer_thickness";
const std::string k_XUnitsPath = "printer/x_camera_dimension/units";
const std::string k_YUnitsPath = "printer/y_camera_dimension/units";

//------------------------------------------------------------------------------
Result<std::vector<usize>> readDatasetDimensions(nx::core::HDF5::FileReader& h5FileReader, const std::string& h5DatasetPath)
{
  nx::core::HDF5::DatasetReader datasetReader = h5FileReader.openDataset(h5DatasetPath);
  if(!datasetReader.isValid())
  {
    return MakeErrorResult<std::vector<usize>>(-3002, fmt::format("Error opening dataset at path '{}' in HDF5 file '{}'", h5DatasetPath, h5FileReader.getName()));
  }

  std::vector<hsize_t> dims = datasetReader.getDimensions();
  std::vector<usize> dimsUSize(dims.size());
  std::transform(dims.begin(), dims.end(), dimsUSize.begin(), [](hsize_t val) { return static_cast<usize>(val); });
  return {dimsUSize};
}

//------------------------------------------------------------------------------
Result<> validateDatasetDimensions(const std::string& datasetPath, std::vector<usize>& dims, std::vector<usize>& sliceDims)
{
  if(dims != sliceDims)
  {
    std::vector<std::string_view> strDims(dims.size());
    std::transform(dims.begin(), dims.end(), strDims.begin(), [](usize val) { return std::to_string(val); });
    std::vector<std::string_view> strSliceDims(sliceDims.size());
    std::transform(sliceDims.begin(), sliceDims.end(), strSliceDims.begin(), [](usize val) { return std::to_string(val); });
    return MakeErrorResult(-3013, fmt::format("Dataset at path '{}' has dimensions ({}) that do not match the slice dimensions ({})", datasetPath, StringUtilities::join(strDims, "x"),
                                              StringUtilities::join(strSliceDims, "x")));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> validateDatasetDimensions(nx::core::HDF5::FileReader& h5FileReader, const std::string& datasetPath, std::vector<usize>& sliceDims)
{
  Result<std::vector<usize>> dimsResult = readDatasetDimensions(h5FileReader, datasetPath);
  if(dimsResult.invalid())
  {
    return {nonstd::make_unexpected(dimsResult.errors())};
  }
  std::vector<usize> dims = dimsResult.value();

  return validateDatasetDimensions(datasetPath, dims, sliceDims);
}

//------------------------------------------------------------------------------
Result<std::vector<usize>> readSliceDimensions(nx::core::HDF5::FileReader& h5FileReader, std::vector<std::string>& segmentationResultsList)
{
  std::vector<usize> sliceDims;
  for(const auto& segmentationResult : segmentationResultsList)
  {
    std::string segmentationResultPath;
    segmentationResultPath.append(ReadPeregrineHDF5File::k_SegmentationResultsH5ParentPath).append("/").append(segmentationResult);

    Result<std::vector<usize>> dimsResult = readDatasetDimensions(h5FileReader, segmentationResultPath);
    if(dimsResult.invalid())
    {
      return dimsResult;
    }

    auto dims = dimsResult.value();
    if(sliceDims.empty())
    {
      // Set the slice dimensions for the first time
      sliceDims = dims;
    }
    else
    {
      Result<> result = validateDatasetDimensions(segmentationResultPath, dims, sliceDims);
      if(result.invalid())
      {
        return ConvertResultTo(std::move(result), std::move(std::vector<usize>{}));
      }
    }
  }

  return {sliceDims};
}

//------------------------------------------------------------------------------
Result<> validateSubvolumeDimensions(std::vector<usize>& volumeDims, std::vector<uint64> subvolumeMinMaxX, std::vector<uint64> subvolumeMinMaxY, std::vector<uint64> subvolumeMinMaxZ)
{
  usize subvolumeX = subvolumeMinMaxX[1] - subvolumeMinMaxX[0] + 1;
  usize subvolumeY = subvolumeMinMaxY[1] - subvolumeMinMaxY[0] + 1;
  usize subvolumeZ = subvolumeMinMaxZ[1] - subvolumeMinMaxZ[0] + 1;

  if(subvolumeMinMaxX[0] > subvolumeMinMaxX[1])
  {
    return MakeErrorResult(-3020, fmt::format("Subvolume minimum X dimension '{}' is larger than the subvolume maximum X dimension '{}'.", subvolumeMinMaxX[0], subvolumeMinMaxX[1]));
  }

  if(subvolumeX > volumeDims[2])
  {
    return MakeErrorResult(-3021, fmt::format("Subvolume X dimension '{}' ({} - {} + 1) is larger than the X dimension value found in the input file data ('{}')", subvolumeX, subvolumeMinMaxX[1],
                                              subvolumeMinMaxX[0], volumeDims[2]));
  }

  if(subvolumeMinMaxY[0] > subvolumeMinMaxY[1])
  {
    return MakeErrorResult(-3022, fmt::format("Subvolume minimum Y dimension '{}' is larger than the subvolume maximum Y dimension '{}'.", subvolumeMinMaxY[0], subvolumeMinMaxY[1]));
  }

  if(subvolumeY > volumeDims[1])
  {
    return MakeErrorResult(-3023, fmt::format("Subvolume Y dimension '{}' ({} - {} + 1) is larger than the Y dimension value found in the input file data ('{}')", subvolumeY, subvolumeMinMaxY[1],
                                              subvolumeMinMaxY[0], volumeDims[1]));
  }

  if(subvolumeMinMaxZ[0] > subvolumeMinMaxZ[1])
  {
    return MakeErrorResult(-3024, fmt::format("Subvolume minimum Z dimension '{}' is larger than the subvolume maximum Z dimension '{}'.", subvolumeMinMaxZ[0], subvolumeMinMaxZ[1]));
  }

  if(subvolumeZ > volumeDims[0])
  {
    return MakeErrorResult(-3025, fmt::format("Subvolume Z dimension '{}' ({} - {} + 1) is larger than the Z dimension value found in the input file data ('{}')", subvolumeZ, subvolumeMinMaxZ[1],
                                              subvolumeMinMaxZ[0], volumeDims[0]));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<CreateImageGeometryAction::SpacingType> calculateSpacing(nx::core::HDF5::FileReader& h5FileReader)
{
  auto attrReader = h5FileReader.getAttribute(k_XRealDimensionPath);
  if(!attrReader.isValid())
  {
    return MakeErrorResult<CreateImageGeometryAction::SpacingType>(-3007, fmt::format("Attribute at path '{}' cannot be found, so the X spacing cannot be calculated!", k_XRealDimensionPath));
  }
  const auto xRealDim = attrReader.readAsValue<float64>();

  auto attrReader2 = h5FileReader.getAttribute(k_YRealDimensionPath);
  if(!attrReader2.isValid())
  {
    return MakeErrorResult<CreateImageGeometryAction::SpacingType>(-3008, fmt::format("Attribute at path '{}' cannot be found, so the Y spacing cannot be calculated!", k_YRealDimensionPath));
  }
  const auto yRealDim = attrReader2.readAsValue<float64>();

  auto attrReader3 = h5FileReader.getAttribute(k_XCameraDimensionPath);
  if(!attrReader3.isValid())
  {
    return MakeErrorResult<CreateImageGeometryAction::SpacingType>(-3009, fmt::format("Attribute at path '{}' cannot be found, so the X spacing cannot be calculated!", k_XCameraDimensionPath));
  }
  const auto xCameraDim = attrReader3.readAsValue<int32>();

  auto attrReader4 = h5FileReader.getAttribute(k_YCameraDimensionPath);
  if(!attrReader4.isValid())
  {
    return MakeErrorResult<CreateImageGeometryAction::SpacingType>(-3010, fmt::format("Attribute at path '{}' cannot be found, so the Y spacing cannot be calculated!", k_YCameraDimensionPath));
  }
  const auto yCameraDim = attrReader4.readAsValue<int32>();

  auto attrReader5 = h5FileReader.getAttribute(k_LayerThicknessPath);
  if(!attrReader5.isValid())
  {
    return MakeErrorResult<CreateImageGeometryAction::SpacingType>(-3011, fmt::format("Attribute at path '{}' cannot be found, so the Z spacing cannot be calculated!", k_LayerThicknessPath));
  }
  const auto zSpacing = attrReader5.readAsValue<float64>();

  return {CreateImageGeometryAction::SpacingType{static_cast<float32>(xRealDim / xCameraDim), static_cast<float32>(yRealDim / yCameraDim), static_cast<float32>(zSpacing)}};
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadPeregrineHDF5FileFilter::name() const
{
  return FilterTraits<ReadPeregrineHDF5FileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadPeregrineHDF5FileFilter::className() const
{
  return FilterTraits<ReadPeregrineHDF5FileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadPeregrineHDF5FileFilter::uuid() const
{
  return FilterTraits<ReadPeregrineHDF5FileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadPeregrineHDF5FileFilter::humanName() const
{
  return "Read Peregrine HDF5 File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadPeregrineHDF5FileFilter::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ReadPeregrineHDF5FileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Input Peregrine HDF5 File", "The input Peregrine HDF5 file that will be read.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".hdf5"}, {".h5"}}, FileSystemPathParameter::PathType::InputFile, false));
  params.insert(std::make_unique<StringParameter>(k_SegmentationResults_Key, "Segmentation Results (comma-delimited)", "The segmentation results numbers that will be read, separated by commas",
                                                  "0,1,2,3,4,5,6,7,8,9,10,11"));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadCameraData_Key, "Read Camera Data", "Specifies whether or not to read the camera data from the input file.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadPartIds_Key, "Read Part Ids", "Specifies whether or not to read the part ids from the input file.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadSampleIds_Key, "Read Sample Ids", "Specifies whether or not to read the sample ids from the input file.", true));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadAnomalyDetection_Key, "Read Anomaly Detection",
                                                                 "Specifies whether or not to read the anomaly detection (part of the registered data) from the input file.", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_ReadXRayCT_Key, "Read X-Ray CT", "Specifies whether or not to read the x-ray CT (part of the registered data) from the input file.", true));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_ReadSlicesSubvolume_Key, "Read Slices Subvolume", "Specifies whether or not to read a subvolume of the slices from the input file.", false));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_SlicesSubvolumeMinMaxX_Key, "Slices Subvolume Min/Max X Dimension", "The min/max indices of the X  dimension for the Slices subvolume.",
                                                        std::vector<uint64>{0ULL, 99ULL}, std::vector<std::string>{"X Min", "X Max"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_SlicesSubvolumeMinMaxY_Key, "Slices Subvolume Min/Max Y Dimension", "The min/max indices of the Y dimension for the Slices subvolume.",
                                                        std::vector<uint64>{0ULL, 99ULL}, std::vector<std::string>{"Y Min", "Y Max"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_SlicesSubvolumeMinMaxZ_Key, "Slices Subvolume Min/Max Z Dimension", "The min/max indices of the Z dimension for the Slices subvolume.",
                                                        std::vector<uint64>{0ULL, 99ULL}, std::vector<std::string>{"Z Min", "Z Max"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ReadRegisteredDataSubvolume_Key, "Read Registered Data Subvolume",
                                                                 "Specifies whether or not to read a subvolume of the registered data from the input file.", false));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_RegisteredDataSubvolumeMinMaxX_Key, "Registered Data Subvolume Min/Max X Dimension",
                                                        "The min/max indices of the X  dimension for the Registered Data subvolume.", std::vector<uint64>{0ULL, 99ULL},
                                                        std::vector<std::string>{"X Min", "X Max"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_RegisteredDataSubvolumeMinMaxY_Key, "Registered Data Subvolume Min/Max Y Dimension",
                                                        "The min/max indices of the Y dimension for the Registered Data subvolume.", std::vector<uint64>{0ULL, 99ULL},
                                                        std::vector<std::string>{"Y Min", "Y Max"}));
  params.insert(std::make_unique<VectorUInt64Parameter>(k_RegisteredDataSubvolumeMinMaxZ_Key, "Registered Data Subvolume Min/Max Z Dimension",
                                                        "The min/max indices of the Z dimension for the Registered Data subvolume.", std::vector<uint64>{0ULL, 99ULL},
                                                        std::vector<std::string>{"Z Min", "Z Max"}));

  params.insertSeparator(Parameters::Separator{"Created Data Objects"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_SliceData_Key, "Slice Data Image Geometry", "The path to the newly created Slice Data image geometry", DataPath({"Slice Data"})));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_SliceDataCellAttrMat_Key, "Slice Data Cell Attribute Matrix Name", "The name of the Slice Data cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CameraData0ArrayName_Key, "Camera Data 0 Array Name", "The name of the camera data 0 array.", "Camera Data 0"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CameraData1ArrayName_Key, "Camera Data 1 Array Name", "The name of the camera data 1 array.", "Camera Data 1"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_PartIdsArrayName_Key, "Part Ids Array Name", "The name of the part ids array.", "Part Ids"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_SampleIdsArrayName_Key, "Sample Ids Array Name", "The name of the sample ids array.", "Sample Ids"));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_RegisteredData_Key, "Registered Data Image Geometry", "The path to the newly created Registered Data image geometry",
                                                             DataPath({"Registered Data"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_RegisteredDataCellAttrMat_Key, "Registered Data Cell Attribute Matrix Name", "The name of the Registered Data cell attribute matrix",
                                                          ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_AnomalyDetectionArrayName_Key, "Anomaly Detection Array Name", "The name of the Anomaly Detection array.", "Anomaly Detection"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_XRayCTArrayName_Key, "X-Ray CT Array Name", "The name of the X-Ray CT array.", "X-Ray CT"));

  params.linkParameters(k_ReadSlicesSubvolume_Key, k_SlicesSubvolumeMinMaxX_Key, true);
  params.linkParameters(k_ReadSlicesSubvolume_Key, k_SlicesSubvolumeMinMaxY_Key, true);
  params.linkParameters(k_ReadSlicesSubvolume_Key, k_SlicesSubvolumeMinMaxZ_Key, true);
  params.linkParameters(k_ReadRegisteredDataSubvolume_Key, k_RegisteredDataSubvolumeMinMaxX_Key, true);
  params.linkParameters(k_ReadRegisteredDataSubvolume_Key, k_RegisteredDataSubvolumeMinMaxY_Key, true);
  params.linkParameters(k_ReadRegisteredDataSubvolume_Key, k_RegisteredDataSubvolumeMinMaxZ_Key, true);
  params.linkParameters(k_ReadCameraData_Key, k_CameraData0ArrayName_Key, true);
  params.linkParameters(k_ReadCameraData_Key, k_CameraData1ArrayName_Key, true);
  params.linkParameters(k_ReadPartIds_Key, k_PartIdsArrayName_Key, true);
  params.linkParameters(k_ReadSampleIds_Key, k_SampleIdsArrayName_Key, true);
  params.linkParameters(k_ReadAnomalyDetection_Key, k_AnomalyDetectionArrayName_Key, true);
  params.linkParameters(k_ReadXRayCT_Key, k_XRayCTArrayName_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadPeregrineHDF5FileFilter::clone() const
{
  return std::make_unique<ReadPeregrineHDF5FileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadPeregrineHDF5FileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel) const
{
  auto inputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto segmentationResultsStr = filterArgs.value<StringParameter::ValueType>(k_SegmentationResults_Key);
  auto readCameraData = filterArgs.value<BoolParameter::ValueType>(k_ReadCameraData_Key);
  auto readPartIds = filterArgs.value<BoolParameter::ValueType>(k_ReadPartIds_Key);
  auto readSampleIds = filterArgs.value<BoolParameter::ValueType>(k_ReadSampleIds_Key);
  auto readSlicesSubvolume = filterArgs.value<BoolParameter::ValueType>(k_ReadSlicesSubvolume_Key);
  auto slicesSubvolumeMinMaxX = filterArgs.value<VectorUInt64Parameter::ValueType>(k_SlicesSubvolumeMinMaxX_Key);
  auto slicesSubvolumeMinMaxY = filterArgs.value<VectorUInt64Parameter::ValueType>(k_SlicesSubvolumeMinMaxY_Key);
  auto slicesSubvolumeMinMaxZ = filterArgs.value<VectorUInt64Parameter::ValueType>(k_SlicesSubvolumeMinMaxZ_Key);
  auto sliceDataImageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_SliceData_Key);
  auto sliceDataCellAttrMatName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceDataCellAttrMat_Key);
  auto cameraData0ArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CameraData0ArrayName_Key);
  auto cameraData1ArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CameraData1ArrayName_Key);
  auto partIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartIdsArrayName_Key);
  auto sampleIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SampleIdsArrayName_Key);
  auto registeredDataImageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_RegisteredData_Key);
  auto registeredDataCellAttrMatName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RegisteredDataCellAttrMat_Key);
  auto readRegisteredDataSubvolume = filterArgs.value<BoolParameter::ValueType>(k_ReadRegisteredDataSubvolume_Key);
  auto registeredDataSubvolumeMinMaxX = filterArgs.value<VectorUInt64Parameter::ValueType>(k_RegisteredDataSubvolumeMinMaxX_Key);
  auto registeredDataSubvolumeMinMaxY = filterArgs.value<VectorUInt64Parameter::ValueType>(k_RegisteredDataSubvolumeMinMaxY_Key);
  auto registeredDataSubvolumeMinMaxZ = filterArgs.value<VectorUInt64Parameter::ValueType>(k_RegisteredDataSubvolumeMinMaxZ_Key);
  auto readAnomalyDetection = filterArgs.value<BoolParameter::ValueType>(k_ReadAnomalyDetection_Key);
  auto anomalyDetectionArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AnomalyDetectionArrayName_Key);
  auto readXRayCT = filterArgs.value<BoolParameter::ValueType>(k_ReadXRayCT_Key);
  auto xRayCTArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_XRayCTArrayName_Key);

  OutputActions actions;
  std::vector<PreflightValue> preflightUpdatedValues;

  segmentationResultsStr = StringUtilities::trimmed(segmentationResultsStr);
  auto segmentationResultsList = StringUtilities::split(segmentationResultsStr, std::vector<char>{','}, true);
  if(segmentationResultsList.empty())
  {
    return {nonstd::make_unexpected(
        std::vector<Error>{Error{-3000, "The segmentation results are empty.  Please input the segmentation results that this filter should read from the input file, separated by commas."}})};
  }

  nx::core::HDF5::FileReader h5FileReader(inputFilePath.string());
  hid_t fileId = h5FileReader.getId();
  if(fileId < 0)
  {
    return {nonstd::make_unexpected(std::vector<Error>{Error{-3001, fmt::format("Error opening Peregrine HDF5 file: '{}'", inputFilePath.string())}})};
  }

  Result<std::vector<usize>> sliceDimsResult = readSliceDimensions(h5FileReader, segmentationResultsList);
  if(sliceDimsResult.invalid())
  {
    return {nonstd::make_unexpected(sliceDimsResult.errors())};
  }

  std::vector<usize> sliceDims = sliceDimsResult.value();
  std::vector<usize> slicesImageGeomDims = sliceDims;

  if(readSlicesSubvolume)
  {
    Result<> result = validateSubvolumeDimensions(sliceDims, slicesSubvolumeMinMaxX, slicesSubvolumeMinMaxY, slicesSubvolumeMinMaxZ);
    if(result.invalid())
    {
      return {nonstd::make_unexpected(result.errors())};
    }
    slicesImageGeomDims = {slicesSubvolumeMinMaxZ[1] - slicesSubvolumeMinMaxZ[0] + 1, slicesSubvolumeMinMaxY[1] - slicesSubvolumeMinMaxY[0] + 1,
                           slicesSubvolumeMinMaxX[1] - slicesSubvolumeMinMaxX[0] + 1};
  }

  Result<CreateImageGeometryAction::SpacingType> spacingResult = calculateSpacing(h5FileReader);
  if(spacingResult.invalid())
  {
    return {nonstd::make_unexpected(spacingResult.errors())};
  }

  std::vector<float32> origin = {0.0f, 0.0f, 0.0f};
  std::vector<float32> spacing = spacingResult.value();

  actions.appendAction(
      std::make_unique<CreateImageGeometryAction>(sliceDataImageGeomPath, std::vector<usize>(slicesImageGeomDims.rbegin(), slicesImageGeomDims.rend()), origin, spacing, sliceDataCellAttrMatName));

  for(const auto& segmentationResult : segmentationResultsList)
  {
    DataPath segmentationResultPath = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(segmentationResult);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, slicesImageGeomDims, std::vector<usize>{1}, segmentationResultPath));
  }

  if(readCameraData)
  {
    Result<> result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_CameraData0H5Path, sliceDims);
    if(result.invalid())
    {
      return {nonstd::make_unexpected(result.errors())};
    }

    DataPath cameraData0Path = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(cameraData0ArrayName);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, slicesImageGeomDims, std::vector<usize>{1}, cameraData0Path));

    result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_CameraData1H5Path, sliceDims);
    if(result.invalid())
    {
      return {nonstd::make_unexpected(result.errors())};
    }

    DataPath cameraData1Path = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(cameraData1ArrayName);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, slicesImageGeomDims, std::vector<usize>{1}, cameraData1Path));
  }

  if(readPartIds)
  {
    Result<> result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_PartIdsH5Path, sliceDims);
    if(result.invalid())
    {
      return {nonstd::make_unexpected(result.errors())};
    }

    DataPath partIdsPath = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(partIdsArrayName);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint32, slicesImageGeomDims, std::vector<usize>{1}, partIdsPath));
  }

  if(readSampleIds)
  {
    Result<> result = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_SampleIdsH5Path, sliceDims);
    if(result.invalid())
    {
      return {nonstd::make_unexpected(result.errors())};
    }

    DataPath sampleIdsPath = sliceDataImageGeomPath.createChildPath(sliceDataCellAttrMatName).createChildPath(sampleIdsArrayName);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint32, slicesImageGeomDims, std::vector<usize>{1}, sampleIdsPath));
  }

  Result<std::vector<usize>> anomalyDetectionDimsResult = readDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_RegisteredAnomalyDetectionH5Path);
  if(anomalyDetectionDimsResult.invalid())
  {
    return {nonstd::make_unexpected(anomalyDetectionDimsResult.errors())};
  }
  std::vector<usize> registeredDataDims = anomalyDetectionDimsResult.value();
  std::vector<usize> registeredDataImgGeomDims = registeredDataDims;

  Result<> xRayCTDimsValidationResult = validateDatasetDimensions(h5FileReader, ReadPeregrineHDF5File::k_RegisteredXRayCtH5Path, registeredDataDims);
  if(xRayCTDimsValidationResult.invalid())
  {
    return {nonstd::make_unexpected(xRayCTDimsValidationResult.errors())};
  }

  if(readRegisteredDataSubvolume)
  {
    Result<> subvolumeValidationResult = validateSubvolumeDimensions(registeredDataDims, registeredDataSubvolumeMinMaxX, registeredDataSubvolumeMinMaxY, registeredDataSubvolumeMinMaxZ);
    if(subvolumeValidationResult.invalid())
    {
      return {nonstd::make_unexpected(subvolumeValidationResult.errors())};
    }
    registeredDataImgGeomDims = {registeredDataSubvolumeMinMaxZ[1] - registeredDataSubvolumeMinMaxZ[0] + 1, registeredDataSubvolumeMinMaxY[1] - registeredDataSubvolumeMinMaxY[0] + 1,
                                 registeredDataSubvolumeMinMaxX[1] - registeredDataSubvolumeMinMaxX[0] + 1};
  }

  actions.appendAction(std::make_unique<CreateImageGeometryAction>(registeredDataImageGeomPath, std::vector<usize>(registeredDataImgGeomDims.rbegin(), registeredDataImgGeomDims.rend()), origin,
                                                                   spacing, registeredDataCellAttrMatName));

  if(readAnomalyDetection)
  {
    DataPath anomalyDetectionPath = registeredDataImageGeomPath.createChildPath(registeredDataCellAttrMatName).createChildPath(anomalyDetectionArrayName);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, registeredDataImgGeomDims, std::vector<usize>{1}, anomalyDetectionPath));
  }

  if(readXRayCT)
  {
    DataPath xRayCTPath = registeredDataImageGeomPath.createChildPath(registeredDataCellAttrMatName).createChildPath(xRayCTArrayName);
    actions.appendAction(std::make_unique<CreateArrayAction>(DataType::uint8, registeredDataImgGeomDims, std::vector<usize>{1}, xRayCTPath));
  }

  if(readSlicesSubvolume)
  {
    std::stringstream ss;
    ss << "Extents:\n"
       << "X Extent: 0 to " << sliceDims[2] - 1 << " (dimension: " << sliceDims[2] << ")\n"
       << "Y Extent: 0 to " << sliceDims[1] - 1 << " (dimension: " << sliceDims[1] << ")\n"
       << "Z Extent: 0 to " << sliceDims[0] - 1 << " (dimension: " << sliceDims[0] << ")\n";
    std::string sliceDataDimsStr = ss.str();

    preflightUpdatedValues.push_back({"Original Slice Data Dimensions (in pixels)", sliceDataDimsStr});
  }

  if(readRegisteredDataSubvolume)
  {
    std::stringstream ss;
    ss << "Extents:\n"
       << "X Extent: 0 to " << registeredDataDims[2] - 1 << " (dimension: " << registeredDataDims[2] << ")\n"
       << "Y Extent: 0 to " << registeredDataDims[1] - 1 << " (dimension: " << registeredDataDims[1] << ")\n"
       << "Z Extent: 0 to " << registeredDataDims[0] - 1 << " (dimension: " << registeredDataDims[0] << ")\n";
    std::string registeredDataDimsStr = ss.str();

    preflightUpdatedValues.push_back({"Original Registered Data Dimensions (in pixels)", registeredDataDimsStr});
  }

  return {{actions}, std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadPeregrineHDF5FileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel) const
{
  ReadPeregrineHDF5FileInputValues inputValues;

  inputValues.inputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  inputValues.segmentationResultsStr = StringUtilities::trimmed(filterArgs.value<StringParameter::ValueType>(k_SegmentationResults_Key));
  inputValues.readCameraData = filterArgs.value<BoolParameter::ValueType>(k_ReadCameraData_Key);
  inputValues.readPartIds = filterArgs.value<BoolParameter::ValueType>(k_ReadPartIds_Key);
  inputValues.readSampleIds = filterArgs.value<BoolParameter::ValueType>(k_ReadSampleIds_Key);
  inputValues.readSlicesSubvolume = filterArgs.value<BoolParameter::ValueType>(k_ReadSlicesSubvolume_Key);
  inputValues.slicesSubvolumeMinMaxX = filterArgs.value<VectorUInt64Parameter::ValueType>(k_SlicesSubvolumeMinMaxX_Key);
  inputValues.slicesSubvolumeMinMaxY = filterArgs.value<VectorUInt64Parameter::ValueType>(k_SlicesSubvolumeMinMaxY_Key);
  inputValues.slicesSubvolumeMinMaxZ = filterArgs.value<VectorUInt64Parameter::ValueType>(k_SlicesSubvolumeMinMaxZ_Key);
  inputValues.sliceDataImageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_SliceData_Key);
  inputValues.sliceDataCellAttrMatName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SliceDataCellAttrMat_Key);
  inputValues.cameraData0ArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CameraData0ArrayName_Key);
  inputValues.cameraData1ArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CameraData1ArrayName_Key);
  inputValues.partIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_PartIdsArrayName_Key);
  inputValues.sampleIdsArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_SampleIdsArrayName_Key);
  inputValues.registeredDataImageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(k_RegisteredData_Key);
  inputValues.registeredDataCellAttrMatName = filterArgs.value<DataObjectNameParameter::ValueType>(k_RegisteredDataCellAttrMat_Key);
  inputValues.readRegisteredDataSubvolume = filterArgs.value<BoolParameter::ValueType>(k_ReadRegisteredDataSubvolume_Key);
  inputValues.registeredDataSubvolumeMinMaxX = filterArgs.value<VectorUInt64Parameter::ValueType>(k_RegisteredDataSubvolumeMinMaxX_Key);
  inputValues.registeredDataSubvolumeMinMaxY = filterArgs.value<VectorUInt64Parameter::ValueType>(k_RegisteredDataSubvolumeMinMaxY_Key);
  inputValues.registeredDataSubvolumeMinMaxZ = filterArgs.value<VectorUInt64Parameter::ValueType>(k_RegisteredDataSubvolumeMinMaxZ_Key);
  inputValues.readAnomalyDetection = filterArgs.value<BoolParameter::ValueType>(k_ReadAnomalyDetection_Key);
  inputValues.anomalyDetectionArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_AnomalyDetectionArrayName_Key);
  inputValues.readXRayCT = filterArgs.value<BoolParameter::ValueType>(k_ReadXRayCT_Key);
  inputValues.xRayCTArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_XRayCTArrayName_Key);

  return ReadPeregrineHDF5File(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
