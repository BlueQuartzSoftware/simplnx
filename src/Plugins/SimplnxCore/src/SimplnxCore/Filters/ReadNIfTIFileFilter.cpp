#include "ReadNIfTIFileFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ReadNIfTIFile.hpp"
#include "SimplnxCore/Filters/CropImageGeometryFilter.hpp"
#include "SimplnxCore/utils/NiftiUtilities.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"

#include <atomic>
#include <filesystem>
#include <map>
#include <mutex>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
struct ReadNiftiHeaderCache
{
  std::string inputFile;
  fs::file_time_type timeStamp{};
  nx::core::nifti::NiftiMetadata metadata;
  bool useAffineIfPresent{true};
};

std::atomic_int32_t s_InstanceId = 0;
std::map<int32, ReadNiftiHeaderCache> s_HeaderCache;
std::mutex s_HeaderCacheMutex;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadNIfTIFileFilter::ReadNIfTIFileFilter()
: m_InstanceId(s_InstanceId.fetch_add(1))
{
  std::lock_guard<std::mutex> lock(s_HeaderCacheMutex);
  s_HeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadNIfTIFileFilter::~ReadNIfTIFileFilter() noexcept
{
  std::lock_guard<std::mutex> lock(s_HeaderCacheMutex);
  s_HeaderCache.erase(m_InstanceId);
}

//------------------------------------------------------------------------------
std::string ReadNIfTIFileFilter::name() const
{
  return FilterTraits<ReadNIfTIFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadNIfTIFileFilter::className() const
{
  return FilterTraits<ReadNIfTIFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadNIfTIFileFilter::uuid() const
{
  return FilterTraits<ReadNIfTIFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadNIfTIFileFilter::humanName() const
{
  return "Read NIfTI File (Version 1)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadNIfTIFileFilter::defaultTags() const
{
  return {"Read", "Import", "NIfTI", "NII", "Medical", "MRI", "CT", "xCT"};
}

//------------------------------------------------------------------------------
Parameters ReadNIfTIFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Input NIfTI File", "The NIfTI-1 file to read (.nii or .nii.gz)", fs::path("input.nii"),
                                                          FileSystemPathParameter::ExtensionsType{".nii", ".nii.gz"}, FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<BoolParameter>(k_UseAffineIfPresent_Key, "Use Stored Affine Transform",
                                                "If enabled and the file contains an sform/qform, use it to set the ImageGeom origin and spacing. Rotation components cannot be represented in an "
                                                "axis-aligned image geometry; a warning will be emitted if the transform has a non-trivial rotation.",
                                                true));
  params.insert(std::make_unique<BoolParameter>(k_ApplyScalingTransform_Key, "Apply Scaling Transform",
                                                "If enabled and the file defines a non-trivial scaling (scl_slope != 0 and (slope != 1 or inter != 0)), apply y = slope*x + inter at read time. The "
                                                "output array will be promoted to float32. Scaling is never applied to RGB24/RGBA32 data per the NIfTI-1 specification.",
                                                true));

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  params.insert(std::make_unique<CropGeometryParameter>(k_CroppingOptions_Key, "Cropping Options",
                                                        "Optional cropping of the volume while it is being read. When cropping is enabled, only the selected sub-volume is streamed into memory; the "
                                                        "rest of the file is discarded on read. Supports both voxel index and physical coordinate bounds.",
                                                        CropGeometryParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "Path to the created Image Geometry", DataPath({"NIfTI Image"})));

  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "Name of the attribute matrix holding the voxel data", "Cell Data"));

  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageDataArrayName_Key, "Image Data Array Name", "Name of the array that will hold the voxel values", "ImageData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadNIfTIFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadNIfTIFileFilter::clone() const
{
  return std::make_unique<ReadNIfTIFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadNIfTIFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                            const ExecutionContext& executionContext) const
{
  auto pInputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto pUseAffineIfPresent = filterArgs.value<bool>(k_UseAffineIfPresent_Key);
  auto pApplyScaling = filterArgs.value<bool>(k_ApplyScalingTransform_Key);
  auto pCroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttrMatName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayName = filterArgs.value<std::string>(k_ImageDataArrayName_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!fs::exists(pInputFilePath))
  {
    return {MakeErrorResult<OutputActions>(-34710, fmt::format("Input NIfTI file does not exist: '{}'", pInputFilePath.string()))};
  }

  // Check the cache under the lock; copy the metadata out so the rest of
  // preflight can work on a local value without holding the mutex across the
  // action-building / CropImageGeometry subfilter dance.
  const auto currentTimeStamp = fs::last_write_time(pInputFilePath);
  nifti::NiftiMetadata md;
  bool cacheValid = false;
  {
    std::lock_guard<std::mutex> lock(s_HeaderCacheMutex);
    auto& cache = s_HeaderCache[m_InstanceId];
    if(cache.inputFile == pInputFilePath.string() && cache.timeStamp == currentTimeStamp && cache.useAffineIfPresent == pUseAffineIfPresent)
    {
      md = cache.metadata;
      cacheValid = true;
    }
  }

  if(!cacheValid)
  {
    auto metadataResult = nifti::ReadNiftiHeader(pInputFilePath, pUseAffineIfPresent);
    if(metadataResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
    }
    md = std::move(metadataResult).value();

    std::lock_guard<std::mutex> lock(s_HeaderCacheMutex);
    auto& cache = s_HeaderCache[m_InstanceId];
    cache.inputFile = pInputFilePath.string();
    cache.timeStamp = currentTimeStamp;
    cache.useAffineIfPresent = pUseAffineIfPresent;
    cache.metadata = md;
  }

  // Start with the full-volume geometry from the header
  std::vector<usize> dims = {md.dimensions[0], md.dimensions[1], md.dimensions[2]};
  std::vector<float32> origin = {md.origin[0], md.origin[1], md.origin[2]};
  std::vector<float32> spacing = {md.spacing[0], md.spacing[1], md.spacing[2]};

  preflightUpdatedValues.push_back({"Full Input Geometry", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});

  // If cropping is enabled, run CropImageGeometryFilter::preflight on a temp
  // DataStructure to compute the cropped dims / origin / spacing
  if(pCroppingOptions.type != CropGeometryParameter::CropValues::TypeEnum::NoCropping)
  {
    DataStructure tmpDs;
    OutputActions tmpActions;
    {
      auto tmpGeomAction = std::make_unique<CreateImageGeometryAction>(pImageGeomPath, dims, origin, spacing, pCellAttrMatName);
      tmpActions.appendAction(std::move(tmpGeomAction));
    }
    Result<> tmpActionsResult = tmpActions.applyAll(tmpDs, IDataAction::Mode::Preflight);
    if(tmpActionsResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(tmpActionsResult), {})};
    }

    CropImageGeometryFilter cropImageGeomFilter;
    Arguments cropImageGeomArgs;
    cropImageGeomArgs.insertOrAssign("input_image_geometry_path", std::make_any<DataPath>(pImageGeomPath));
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
    const DataPath croppedGeomPath({pImageGeomPath.getTargetName() + "_cropped"});
    cropImageGeomArgs.insertOrAssign("output_image_geometry_path", std::make_any<DataPath>(croppedGeomPath));

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

    const auto& croppedGeom = tmpDs.getDataRefAs<ImageGeom>(croppedGeomPath);
    dims = croppedGeom.getDimensions().toContainer<std::vector<usize>>();
    origin = croppedGeom.getOrigin().toContainer<std::vector<float32>>();
    spacing = croppedGeom.getSpacing().toContainer<std::vector<float32>>();
  }

  {
    auto createGeomAction = std::make_unique<CreateImageGeometryAction>(pImageGeomPath, dims, origin, spacing, pCellAttrMatName);
    resultOutputActions.value().appendAction(std::move(createGeomAction));
  }

  // Determine output DataType. If the file has non-trivial scaling and the user
  // wants to apply it, promote single-component types to float32.
  DataType outputDataType = md.dataType;
  if(pApplyScaling && md.hasNontrivialScaling && md.componentCount == 1)
  {
    outputDataType = DataType::float32;
  }

  // DataArray tuple dims mirror the reversed image dims (z, y, x)
  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  const std::vector<usize> componentDims = {md.componentCount};
  const DataPath dataArrayPath = pImageGeomPath.createChildPath(pCellAttrMatName).createChildPath(pImageDataArrayName);
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(outputDataType, tupleDims, componentDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});

  std::string scalingSummary = md.hasNontrivialScaling ? fmt::format("slope={}, inter={}{}", md.sclSlope, md.sclInter, (pApplyScaling ? " (applied → float32)" : " (not applied)")) : "none";
  preflightUpdatedValues.push_back({"NIfTI Header", fmt::format("datatype code: {}  |  components: {}  |  byte-swap: {}  |  vox_offset: {}  |  scaling: {}", md.niftiDatatype, md.componentCount,
                                                                md.byteSwapRequired ? "yes" : "no", md.voxOffset, scalingSummary)});

  if(md.affineHasRotation)
  {
    resultOutputActions.warnings().push_back(Warning{
        -34750, fmt::format("NIfTI file '{}' contains a non-trivial rotation in its {} transform. The ImageGeom in simplnx is axis-aligned; the rotation component has been dropped and only the "
                            "spacing and origin were extracted. Voxels will be loaded in their stored orientation.",
                            md.filePath, (md.sformCode > 0 ? "sform" : "qform"))});
  }
  if(md.niftiDatatype == NIFTI_TYPE_RGB24 || md.niftiDatatype == NIFTI_TYPE_RGBA32)
  {
    if(pApplyScaling && (md.sclSlope != 0.0f && (md.sclSlope != 1.0f || md.sclInter != 0.0f)))
    {
      resultOutputActions.warnings().push_back(Warning{
          -34751, fmt::format("NIfTI file '{}' has datatype {} with a scaling transform. Per the NIfTI-1 specification, scaling is not applied to RGB24/RGBA32 data; voxels were copied verbatim.",
                              md.filePath, (md.niftiDatatype == NIFTI_TYPE_RGB24 ? "RGB24" : "RGBA32"))});
    }
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadNIfTIFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  ReadNIfTIFileInputValues inputValues;
  inputValues.InputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  inputValues.UseAffineIfPresent = filterArgs.value<bool>(k_UseAffineIfPresent_Key);
  inputValues.ApplyScalingTransform = filterArgs.value<bool>(k_ApplyScalingTransform_Key);
  inputValues.CroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);
  inputValues.ImageGeometryPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.CellAttributeMatrixName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.ImageDataArrayName = filterArgs.value<std::string>(k_ImageDataArrayName_Key);

  return ReadNIfTIFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

//------------------------------------------------------------------------------
Result<Arguments> ReadNIfTIFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadNIfTIFileFilter().getDefaultArguments();
  // New filter in simplnx; no SIMPL v6 equivalent.
  return {std::move(args)};
}

} // namespace nx::core
