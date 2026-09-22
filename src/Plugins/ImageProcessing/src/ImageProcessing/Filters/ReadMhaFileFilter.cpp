#include "ReadMhaFileFilter.hpp"

#include "ImageProcessing/Filters/Algorithms/ReadMhaFile.hpp"
#include "ImageProcessing/utils/MetaImageUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/Geometry/GeometryTransformation.hpp"
#include "simplnx/Utilities/GeometryHelpers.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageGeometryCrop.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

#include <array>
#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
ImageGeometryCropOptions CreateCropOptions(const DataPath& inputGeometryPath, const DataPath& outputGeometryPath, const CropGeometryParameter::ValueType& croppingOptions)
{
  ImageGeometryCropOptions options;
  options.inputImageGeometryPath = inputGeometryPath;
  options.outputImageGeometryPath = outputGeometryPath;
  options.usePhysicalBounds = croppingOptions.type == CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume;
  options.cropX = croppingOptions.cropX;
  options.cropY = croppingOptions.cropY;
  options.cropZ = croppingOptions.cropZ;
  options.minVoxel = {static_cast<uint64>(croppingOptions.xBoundVoxels[0]), static_cast<uint64>(croppingOptions.yBoundVoxels[0]), static_cast<uint64>(croppingOptions.zBoundVoxels[0])};
  options.maxVoxel = {static_cast<uint64>(croppingOptions.xBoundVoxels[1]), static_cast<uint64>(croppingOptions.yBoundVoxels[1]), static_cast<uint64>(croppingOptions.zBoundVoxels[1])};
  options.minCoordinate = {static_cast<float64>(croppingOptions.xBoundPhysical[0]), static_cast<float64>(croppingOptions.yBoundPhysical[0]), static_cast<float64>(croppingOptions.zBoundPhysical[0])};
  options.maxCoordinate = {static_cast<float64>(croppingOptions.xBoundPhysical[1]), static_cast<float64>(croppingOptions.yBoundPhysical[1]), static_cast<float64>(croppingOptions.zBoundPhysical[1])};
  return options;
}

// Interpolation choices (match ITKMhaFileReaderFilter): 0=NearestNeighbor, 1=Linear.
const ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const ChoicesParameter::Choices k_InterpolationChoices = {"Nearest Neighbor", "Linear Interpolation"};
// ApplyTransformationToGeometry "Manual matrix" transformation-type index.
const ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;

/**
 * @brief Assembles a 4x4 row-major Matrix4fR from the header TransformMatrix
 *        (nDims*nDims, row-major; empty => identity), mirroring ITK's readMhaHeader:
 *        fill 0, (3,3)=1; 2D fills the top-left 2x2 + (2,2)=1; 3D fills the 3x3.
 *        When @p transpose is requested, the matrix must be a pure rotation
 *        (|1 - det| <= 1e-4) or -35852 is returned (mirrors ITK -5002).
 */
Result<ImageRotationUtilities::Matrix4fR> BuildTransformMatrix(const mhd::MetaImageMetadata& md, bool transpose)
{
  ImageRotationUtilities::Matrix4fR mat;
  mat.fill(0.0f);
  mat(3, 3) = 1.0f;

  const std::vector<float64>& tm = md.transformMatrix;
  if(md.nDims == 2)
  {
    if(tm.size() >= 4)
    {
      mat(0, 0) = static_cast<float32>(tm[0]);
      mat(0, 1) = static_cast<float32>(tm[1]);
      mat(1, 0) = static_cast<float32>(tm[2]);
      mat(1, 1) = static_cast<float32>(tm[3]);
    }
    else
    {
      mat(0, 0) = 1.0f;
      mat(1, 1) = 1.0f;
    }
    mat(2, 2) = 1.0f;
  }
  else // md.nDims == 3
  {
    if(tm.size() >= 9)
    {
      for(usize r = 0; r < 3; r++)
      {
        for(usize c = 0; c < 3; c++)
        {
          mat(static_cast<Eigen::Index>(r), static_cast<Eigen::Index>(c)) = static_cast<float32>(tm[r * 3 + c]);
        }
      }
    }
    else
    {
      mat(0, 0) = 1.0f;
      mat(1, 1) = 1.0f;
      mat(2, 2) = 1.0f;
    }
  }

  const float32 det = mat.determinant();
  if(transpose)
  {
    if(std::abs(1.0f - det) > 0.0001f)
    {
      return MakeErrorResult<ImageRotationUtilities::Matrix4fR>(
          -35852, fmt::format("Transformation Matrix is NOT a pure rotation transform (determinant = {}). A pure transpose will not work; de-select 'Transpose Stored Transformation Matrix'.", det));
    }
    mat.transposeInPlace();
  }
  return {mat};
}

// Convert a 4x4 Matrix4fR to the DynamicTable value ApplyTransformationToGeometry expects.
DynamicTableParameter::ValueType MatrixToTable(const ImageRotationUtilities::Matrix4fR& m)
{
  return DynamicTableInfo::TableDataType{{m(0, 0), m(0, 1), m(0, 2), m(0, 3)}, {m(1, 0), m(1, 1), m(1, 2), m(1, 3)}, {m(2, 0), m(2, 1), m(2, 2), m(2, 3)}, {m(3, 0), m(3, 1), m(3, 2), m(3, 3)}};
}

// Create transformation input values for the manual-matrix path.
ApplyTransformationToGeometryInputValues MakeTransformationInputValues(const DataPath& imageGeomPath, const std::string& cellDataName, const ImageRotationUtilities::Matrix4fR& mat,
                                                                       const std::array<float32, 3>& centerOfRotation, ChoicesParameter::ValueType interpolationType)
{
  ApplyTransformationToGeometryInputValues inputValues;
  inputValues.SelectedGeometryPath = imageGeomPath;
  inputValues.CellAttributeMatrixPath = imageGeomPath.createChildPath(cellDataName);
  inputValues.TransformationSelection = k_ManualTransformationMatrixIdx;
  inputValues.ManualMatrixTableData = MatrixToTable(mat);
  inputValues.InterpolationSelection = interpolationType;
  inputValues.TranslateGeometryToGlobalOrigin = centerOfRotation == std::array<float32, 3>{0.0f, 0.0f, 0.0f};
  inputValues.RemoveOriginalGeometry = true;
  return inputValues;
}

Result<> ExecuteTransformation(DataStructure& dataStructure, const ApplyTransformationToGeometryInputValues& inputValues, const IFilter::MessageHandler& messageHandler,
                               const std::atomic_bool& shouldCancel)
{
  IFilter::PreflightResult preflightResult = PreflightGeometryTransformation(dataStructure, inputValues);
  if(preflightResult.outputActions.invalid())
  {
    return ConvertResult(std::move(preflightResult.outputActions));
  }

  OutputActions outputActions = std::move(preflightResult.outputActions.value());
  Result<> regularResult = outputActions.applyRegular(dataStructure, IDataAction::Mode::Execute);
  if(regularResult.invalid())
  {
    return regularResult;
  }

  Result<> executeResult = ApplyGeometryTransformation(dataStructure, inputValues, messageHandler, shouldCancel);
  if(executeResult.invalid())
  {
    return MergeResults(std::move(regularResult), std::move(executeResult));
  }

  Result<> deferredResult = outputActions.applyDeferred(dataStructure, IDataAction::Mode::Execute);
  return MergeResults(MergeResults(std::move(regularResult), std::move(executeResult)), std::move(deferredResult));
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ReadMhaFileFilter::name() const
{
  return FilterTraits<ReadMhaFileFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ReadMhaFileFilter::className() const
{
  return FilterTraits<ReadMhaFileFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ReadMhaFileFilter::uuid() const
{
  return FilterTraits<ReadMhaFileFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ReadMhaFileFilter::humanName() const
{
  return "Read MHA/MetaImage File";
}

//------------------------------------------------------------------------------
std::vector<std::string> ReadMhaFileFilter::defaultTags() const
{
  return {className(), "io", "input", "read", "import", "image", "mha", "mhd", "metaimage", "volume"};
}

//------------------------------------------------------------------------------
Parameters ReadMhaFileFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_InputFilePath_Key, "Input MHA/MetaImage File", "The MetaImage file to read (.mha attached, or .mhd detached header).",
                                                          fs::path("input.mha"), FileSystemPathParameter::ExtensionsType{".mha", ".mhd"}, FileSystemPathParameter::PathType::InputFile));

  params.insertSeparator(Parameters::Separator{"Cropping Options"});
  params.insert(std::make_unique<CropGeometryParameter>(k_CroppingOptions_Key, "Cropping Options",
                                                        "Optional cropping of the volume while it is being read. When cropping is enabled, only the selected sub-volume is streamed into memory; the "
                                                        "rest of the file is decoded and discarded on read. Supports both voxel index and physical coordinate bounds.",
                                                        CropGeometryParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Transformation Matrix Options"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyImageTransformation_Key, "Apply Image Transformation To Geometry",
                                                                 "When true, the transformation matrix found in the image's header metadata will be applied to the created image geometry.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Interpolation Type", "The type of interpolation algorithm that is used. 0=NearestNeighbor, 1=Linear",
                                                   k_NearestNeighborInterpolationIdx, k_InterpolationChoices));
  params.insert(std::make_unique<BoolParameter>(k_TransposeTransformMatrix_Key, "Transpose Stored Transformation Matrix",
                                                "When true, the transformation matrix found in the image's header metadata will be transposed before use.", false));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SaveImageTransformation_Key, "Save Image Transformation As Array",
                                      "When true, the transformation matrix found in the image's header metadata will be saved as a data array in the created image geometry.", false));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationMatrixPath_Key, "Transformation Matrix", "The path to the created transformation matrix data array (float32, 1x16).",
                                                         DataPath({"MHA Image", "TransformationMatrix"})));

  params.insertSeparator(Parameters::Separator{"Output Geometry"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_CreatedImageGeometryPath_Key, "Image Geometry", "Path to the created Image Geometry", DataPath({"MHA Image"})));
  params.insertSeparator(Parameters::Separator{"Output Cell Attribute Matrix"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellAttributeMatrixName_Key, "Cell Attribute Matrix Name", "Name of the attribute matrix holding the voxel data", "Cell Data"));
  params.insertSeparator(Parameters::Separator{"Output Data Array"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageDataArrayName_Key, "Image Data Array Name", "Name of the array that will hold the voxel values", "ImageData"));

  params.linkParameters(k_SaveImageTransformation_Key, k_TransformationMatrixPath_Key, true);
  params.linkParameters(k_ApplyImageTransformation_Key, k_InterpolationType_Key, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ReadMhaFileFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ReadMhaFileFilter::clone() const
{
  return std::make_unique<ReadMhaFileFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ReadMhaFileFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto pInputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto pCroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttrMatName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayName = filterArgs.value<std::string>(k_ImageDataArrayName_Key);
  auto pApplyTransformation = filterArgs.value<bool>(k_ApplyImageTransformation_Key);
  auto pInterpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  auto pTranspose = filterArgs.value<bool>(k_TransposeTransformMatrix_Key);
  auto pSaveTransformation = filterArgs.value<bool>(k_SaveImageTransformation_Key);
  auto pTransformationMatrixPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_TransformationMatrixPath_Key);

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  if(!fs::exists(pInputFilePath))
  {
    return {MakeErrorResult<OutputActions>(-35820, fmt::format("Input MHA/MetaImage file does not exist: '{}'", pInputFilePath.string()))};
  }

  auto metadataResult = mhd::ReadMetaImageHeader(pInputFilePath);
  if(metadataResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(ConvertResult(std::move(metadataResult)), {})};
  }
  const mhd::MetaImageMetadata md = std::move(metadataResult).value();

  std::vector<usize> dims = {md.dimensions[0], md.dimensions[1], md.dimensions[2]};
  std::vector<float32> origin = {md.origin[0], md.origin[1], md.origin[2]};
  std::vector<float32> spacing = {md.spacing[0], md.spacing[1], md.spacing[2]};

  preflightUpdatedValues.push_back({"Full Input Geometry", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});

  // ---- optional crop delegation (verbatim from the NRRD reader's preflight) ----
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

    const DataPath croppedGeomPath({pImageGeomPath.getTargetName() + "_cropped"});
    const ImageGeometryCropOptions cropOptions = CreateCropOptions(pImageGeomPath, croppedGeomPath, pCroppingOptions);
    ImageGeometryCropBounds cropBounds;
    PreflightResult cropImageResult = PreflightImageGeometryCrop(tmpDs, cropOptions, cropBounds);
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

  const std::vector<usize> tupleDims = {dims[2], dims[1], dims[0]};
  const std::vector<usize> componentDims = {md.componentCount};
  const DataPath dataArrayPath = pImageGeomPath.createChildPath(pCellAttrMatName).createChildPath(pImageDataArrayName);
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(md.dataType, tupleDims, componentDims, dataArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  preflightUpdatedValues.push_back({"Imported Geometry Info", nx::core::GeometryHelpers::Description::GenerateGeometryInfo(dims, spacing, origin, IGeometry::LengthUnit::Unspecified)});
  preflightUpdatedValues.push_back({"MetaImage Header", fmt::format("type: {}  |  components: {}  |  compressed: {}  |  byte-swap: {}", md.typeString, md.componentCount,
                                                                    (md.encoding == mhd::Encoding::Compressed ? "yes" : "no"), md.byteSwapRequired ? "yes" : "no")});

  // ---- transform matrix (assemble + optional transpose check) ----
  if(pSaveTransformation || pApplyTransformation || pTranspose)
  {
    auto matrixResult = BuildTransformMatrix(md, pTranspose);
    if(matrixResult.invalid())
    {
      return {ConvertResultTo<OutputActions>(ConvertResult(std::move(matrixResult)), {}), std::move(preflightUpdatedValues)};
    }
    const ImageRotationUtilities::Matrix4fR mat = matrixResult.value();
    preflightUpdatedValues.push_back({"Transformation Matrix", ImageRotationUtilities::GenerateTransformationMatrixDescription(mat)});

    if(pSaveTransformation)
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(DataType::float32, std::vector<usize>{1}, std::vector<usize>{16}, pTransformationMatrixPath);
      resultOutputActions.value().appendAction(std::move(createArrayAction));
    }

    if(pApplyTransformation)
    {
      // The created ImageGeom + cell AttributeMatrix + image DataArray only exist as QUEUED output
      // actions at this point, so preflighting ApplyTransformationToGeometry against the (unmodified)
      // incoming DataStructure cannot find the geometry -- its preflight then dereferences a missing
      // object and crashes. Stage the queued output actions onto a scratch copy of the incoming
      // DataStructure (Preflight mode) so the geometry exists, then preflight the transform on THAT
      // (the standard pattern for preflighting a downstream filter that needs created objects).
      DataStructure stagedDataStructure = dataStructure;
      Result<> stageResult = resultOutputActions.value().applyAll(stagedDataStructure, IDataAction::Mode::Preflight);
      if(stageResult.invalid())
      {
        return {ConvertResultTo<OutputActions>(std::move(stageResult), {}), std::move(preflightUpdatedValues)};
      }

      const ApplyTransformationToGeometryInputValues transformationInputValues = MakeTransformationInputValues(pImageGeomPath, pCellAttrMatName, mat, md.centerOfRotation, pInterpolationType);
      PreflightResult applyResult = PreflightGeometryTransformation(stagedDataStructure, transformationInputValues);
      for(const auto& v : applyResult.outputValues)
      {
        preflightUpdatedValues.push_back(v);
      }
      // KNOWN LIMITATION (deliberate ITK parity): we surface ApplyTransformationToGeometry's
      // errors/warnings below, but we intentionally do NOT propagate its geometry-mutating
      // output actions into resultOutputActions. So pipeline preflight still sees the
      // pre-transform ImageGeom dimensions, while execute actually resamples the geometry to
      // its post-transform extents. The legacy ITKMhaFileReaderFilter commented out that same
      // propagation, and we keep the behavior for parity. Consequence: downstream filters must
      // not rely on the transformed geometry's dimensions being known at preflight when Apply
      // Transformation is enabled.
      // Surface any transform-preflight errors/warnings to the caller.
      for(const auto& w : applyResult.outputActions.warnings())
      {
        resultOutputActions.warnings().push_back(w);
      }
      if(applyResult.outputActions.invalid())
      {
        for(const auto& e : applyResult.outputActions.errors())
        {
          resultOutputActions.errors().push_back(e);
        }
        return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
      }
    }
  }

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ReadMhaFileFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pInputFilePath = filterArgs.value<FileSystemPathParameter::ValueType>(k_InputFilePath_Key);
  auto pImageGeomPath = filterArgs.value<DataPath>(k_CreatedImageGeometryPath_Key);
  auto pCellAttrMatName = filterArgs.value<std::string>(k_CellAttributeMatrixName_Key);
  auto pImageDataArrayName = filterArgs.value<std::string>(k_ImageDataArrayName_Key);
  auto pApplyTransformation = filterArgs.value<bool>(k_ApplyImageTransformation_Key);
  auto pInterpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  auto pTranspose = filterArgs.value<bool>(k_TransposeTransformMatrix_Key);
  auto pSaveTransformation = filterArgs.value<bool>(k_SaveImageTransformation_Key);
  auto pTransformationMatrixPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_TransformationMatrixPath_Key);

  // 1) Stream the voxel data (pixels only) via the ReadMhaFile algorithm.
  {
    ReadMhaFileInputValues inputValues;
    inputValues.InputFilePath = pInputFilePath;
    inputValues.CroppingOptions = filterArgs.value<CropGeometryParameter::ValueType>(k_CroppingOptions_Key);
    inputValues.ImageGeometryPath = pImageGeomPath;
    inputValues.CellAttributeMatrixName = pCellAttrMatName;
    inputValues.ImageDataArrayName = pImageDataArrayName;
    Result<> readResult = ReadMhaFile(dataStructure, messageHandler, shouldCancel, &inputValues)();
    if(readResult.invalid())
    {
      return readResult;
    }
  }

  if(!pSaveTransformation && !pApplyTransformation)
  {
    return {};
  }

  // 2) Transform feature: re-read the header, assemble the 4x4, optional transpose.
  auto metadataResult = mhd::ReadMetaImageHeader(pInputFilePath);
  if(metadataResult.invalid())
  {
    return ConvertResult(std::move(metadataResult));
  }
  const mhd::MetaImageMetadata md = std::move(metadataResult).value();

  auto matrixResult = BuildTransformMatrix(md, pTranspose);
  if(matrixResult.invalid())
  {
    return ConvertResult(std::move(matrixResult));
  }
  const ImageRotationUtilities::Matrix4fR mat = matrixResult.value();

  if(pSaveTransformation)
  {
    auto& matrixArray = dataStructure.getDataRefAs<DataArray<float32>>(pTransformationMatrixPath);
    // Eigen 3.4 exposes STL iterators; the 4x4 is row-major so data()[0..15] is row-major.
    std::copy(mat.data(), mat.data() + 16, matrixArray.begin());
  }

  if(pApplyTransformation)
  {
    messageHandler(fmt::format("Applying transformation matrix to image geometry '{}'...", pImageGeomPath.getTargetName()));
    const ApplyTransformationToGeometryInputValues transformationInputValues = MakeTransformationInputValues(pImageGeomPath, pCellAttrMatName, mat, md.centerOfRotation, pInterpolationType);
    Result<> applyResult = ExecuteTransformation(dataStructure, transformationInputValues, messageHandler, shouldCancel);
    if(applyResult.invalid())
    {
      return applyResult;
    }
  }

  return {};
}

//------------------------------------------------------------------------------
Result<Arguments> ReadMhaFileFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ReadMhaFileFilter().getDefaultArguments();
  // New filter in simplnx; no SIMPL v6 equivalent.
  return {std::move(args)};
}

} // namespace nx::core
