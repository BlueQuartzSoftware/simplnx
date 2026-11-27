#include "ReadNIfTIFileFilter.hpp"

#include "simplnx/Utilities/ImageIO/ReaderGeometryOverrides.hpp"

#include "NXImageProcessing/Filters/Algorithms/ReadNIfTIFile.hpp"
#include "NXImageProcessing/utils/NiftiUtilities.hpp"

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
#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"

#include <atomic>
#include <cmath>
#include <filesystem>
#include <map>
#include <mutex>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
constexpr int32 k_IgnoredAxisDirectionWarning = -34753;

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
  options.min_coordinate = {static_cast<float64>(croppingOptions.x_bound_physical[0]), static_cast<float64>(croppingOptions.y_bound_physical[0]),
                            static_cast<float64>(croppingOptions.z_bound_physical[0])};
  options.max_coordinate = {static_cast<float64>(croppingOptions.x_bound_physical[1]), static_cast<float64>(croppingOptions.y_bound_physical[1]),
                            static_cast<float64>(croppingOptions.z_bound_physical[1])};
  return options;
}

struct ReadNiftiHeaderCache
{
  std::string input_file;
  fs::file_time_type time_stamp;
  nx::core::nifti::NiftiMetadata metadata;
  bool use_affine_if_present{true};
};

std::atomic_int32_t SInstanceId = 0;
std::map<int32, ReadNiftiHeaderCache> SHeaderCache;
std::mutex SHeaderCacheMutex;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
ReadNIfTIFileFilter::ReadNIfTIFileFilter()
: m_InstanceId(SInstanceId.fetch_add(1))
{
  std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
  SHeaderCache[m_InstanceId] = {};
}

//------------------------------------------------------------------------------
ReadNIfTIFileFilter::~ReadNIfTIFileFilter() noexcept
{
  std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
  SHeaderCache.erase(m_InstanceId);
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
  params.insert(std::make_unique<BoolParameter>(
      k_ApplyScalingTransform_Key, "Apply Scaling Transform",
      "If enabled and the file defines a non-trivial scaling with a finite slope (scl_slope != 0 and (slope != 1 or inter != 0)), apply y = slope*x + inter at read time. The "
      "output array will be promoted to float32. A non-finite slope disables scaling. A non-finite intercept is replaced by 0. Scaling is never applied to "
      "RGB24/RGBA32 data per the NIfTI-1 specification.",
      true));

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  params.insert(std::make_unique<CropGeometryParameter>(k_CroppingOptions_Key, "Cropping Options",
                                                        "Optional cropping of the volume while it is being read. When cropping is enabled, only the selected sub-volume is streamed into memory; the "
                                                        "rest of the file is discarded on read. Supports both voxel index and physical coordinate bounds.",
                                                        CropGeometryParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Geometry Metadata Overrides"});
  image_io::InsertGeometryOverrideParameters(params);

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
  // Version 2: Added Set Origin and Set Spacing.
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ReadNIfTIFileFilter::clone() const
{
  return std::make_unique<ReadNIfTIFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadNIfTIFileFilter::preflightImpl(const DataStructure& /*dataStructure*/, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                            const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
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
    return {.output_actions = MakeErrorResult<OutputActions>(-34710, fmt::format("Input NIfTI file does not exist: '{}'", pInputFilePath.string()))};
  }

  // Check the cache under the lock; copy the metadata out so the rest of
  // preflight can work on a local value without holding the mutex across the
  // action-building / CropImageGeometry subfilter dance.
  const auto currentTimeStamp = fs::last_write_time(pInputFilePath);
  nifti::NiftiMetadata md;
  bool cacheValid = false;
  {
    std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
    auto& cache = SHeaderCache[m_InstanceId];
    if(cache.input_file == pInputFilePath.string() && cache.time_stamp == currentTimeStamp && cache.use_affine_if_present == pUseAffineIfPresent)
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
      return {.output_actions = ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
    }
    md = std::move(metadataResult).value();

    std::lock_guard<std::mutex> lock(SHeaderCacheMutex);
    auto& cache = SHeaderCache[m_InstanceId];
    cache.input_file = pInputFilePath.string();
    cache.time_stamp = currentTimeStamp;
    cache.use_affine_if_present = pUseAffineIfPresent;
    cache.metadata = md;
  }

  // Start with the full-volume geometry from the header
  std::vector<usize> dims = {md.dimensions[0], md.dimensions[1], md.dimensions[2]};
  std::vector<float32> origin = {md.origin[0], md.origin[1], md.origin[2]};
  std::vector<float32> spacing = {md.spacing[0], md.spacing[1], md.spacing[2]};
  if(auto geometryResult = image_io::GeometryOverrides::FromArguments(filterArgs).apply(pInputFilePath, origin, spacing); geometryResult.invalid())
  {
    return {.output_actions = ConvertInvalidResult<OutputActions>(std::move(geometryResult))};
  }

  preflightUpdatedValues.push_back({"Full Input Geometry", nx::core::geometry_helpers::description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});

  // If cropping is enabled, use the shared crop planner on a temporary DataStructure.
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
      return {.output_actions = ConvertResultTo<OutputActions>(std::move(tmpActionsResult), {})};
    }

    const DataPath croppedGeomPath({pImageGeomPath.getTargetName() + "_cropped"});
    const ImageGeometryCropOptions cropOptions = CreateCropOptions(pImageGeomPath, croppedGeomPath, pCroppingOptions);
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
  DataType outputDataType = md.data_type;
  if(pApplyScaling && md.has_nontrivial_scaling && md.component_count == 1)
  {
    outputDataType = DataType::float32;
  }

  // DataArray tuple dims mirror the reversed image dims (z, y, x)
  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  const std::vector<usize> componentDims = {md.component_count};
  const DataPath dataArrayPath = pImageGeomPath.createChildPath(pCellAttrMatName).createChildPath(pImageDataArrayName);
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(outputDataType, tupleDims, componentDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::geometry_helpers::description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});

  std::string scalingSummary = md.has_nontrivial_scaling ?
                                   fmt::format("slope={}, inter={}{}", md.scl_slope, std::isfinite(md.scl_inter) ? md.scl_inter : 0.0f, (pApplyScaling ? " (applied → float32)" : " (not applied)")) :
                                   "none";
  preflightUpdatedValues.push_back({"NIfTI Header", fmt::format("datatype code: {}  |  components: {}  |  byte-swap: {}  |  vox_offset: {}  |  scaling: {}", md.nifti_datatype, md.component_count,
                                                                md.byte_swap_required ? "yes" : "no", md.vox_offset, scalingSummary)});

  if(md.affine_has_flip)
  {
    resultOutputActions.warnings().push_back(Warning{
        .code = k_IgnoredAxisDirectionWarning,
        .message = fmt::format("NIfTI file '{}' contains negative direction values. This filter uses positive spacing and does not preserve these directions. The loaded image may have a different "
                               "orientation. Voxels are loaded in their stored order.",
                               md.file_path)});
  }
  if(md.affine_has_rotation)
  {
    resultOutputActions.warnings().push_back(Warning{
        .code = -34750,
        .message = fmt::format("NIfTI file '{}' contains a non-trivial rotation in its {} transform. The ImageGeom in simplnx is axis-aligned; the rotation component has been dropped and only the "
                               "spacing and origin were extracted. Voxels will be loaded in their stored orientation.",
                               md.file_path, (md.sform_code > 0 ? "sform" : "qform"))});
  }
  if(pApplyScaling && md.component_count == 1 && std::isfinite(md.scl_slope) && md.scl_slope != 0.0f && !std::isfinite(md.scl_inter))
  {
    resultOutputActions.warnings().push_back(Warning{.code = -34752, .message = fmt::format("NIfTI file '{}' scaling intercept is not finite; 0 is used for the intercept.", md.file_path)});
  }
  if(md.nifti_datatype == NIFTI_TYPE_RGB24 || md.nifti_datatype == NIFTI_TYPE_RGBA32)
  {
    if(pApplyScaling && std::isfinite(md.scl_slope) && (md.scl_slope != 0.0f && (md.scl_slope != 1.0f || (std::isfinite(md.scl_inter) && md.scl_inter != 0.0f))))
    {
      resultOutputActions.warnings().push_back(Warning{
          .code = -34751,
          .message = fmt::format("NIfTI file '{}' has datatype {} with a scaling transform. Per the NIfTI-1 specification, scaling is not applied to RGB24/RGBA32 data; voxels were copied verbatim.",
                                 md.file_path, (md.nifti_datatype == NIFTI_TYPE_RGB24 ? "RGB24" : "RGBA32"))});
    }
  }

  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadNIfTIFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ReadNIfTIFileInputValues inputValues;
  inputValues.geometry_overrides = image_io::GeometryOverrides::FromArguments(filterArgs);
  inputValues.input_file_path = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  inputValues.use_affine_if_present = filterArgs.value<bool>(k_UseAffineIfPresent_Key);
  inputValues.apply_scaling_transform = filterArgs.value<bool>(k_ApplyScalingTransform_Key);
  inputValues.cropping_options = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);
  inputValues.image_geometry_path = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  inputValues.cell_attribute_matrix_name = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  inputValues.image_data_array_name = filterArgs.value<std::string>(k_ImageDataArrayName_Key);

  return ReadNIfTIFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

//------------------------------------------------------------------------------
Result<Arguments> ReadNIfTIFileFilter::FromSIMPLJson(const nlohmann::json& /*json*/)
{
  Arguments args = ReadNIfTIFileFilter().getDefaultArguments();
  // New filter in simplnx; no SIMPL v6 equivalent.
  return {std::move(args)};
}

} // namespace nx::core
