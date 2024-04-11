#include "ITKMhaFileReader.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/DynamicTableParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <Eigen/Dense>

#include <itkMetaImageIO.h>

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{
const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_ApplyTransformationToGeometryFilterId = *Uuid::FromString("f5bbc16b-3426-4ae0-b27b-ba7862dc40fe");
const FilterHandle k_ApplyTransformationToGeometryFilterHandle(k_ApplyTransformationToGeometryFilterId, k_SimplnxCorePluginId);

inline constexpr StringLiteral k_SelectedImageGeometryKey = "input_image_geometry_path";
inline constexpr StringLiteral k_CellAttributeMatrixPathKey = "cell_attribute_matrix_path";
inline constexpr StringLiteral k_TransformationTypeKey = "transformation_type";
inline constexpr StringLiteral k_ManualTransformationMatrixKey = "manual_transformation_matrix";
inline constexpr StringLiteral k_InterpolationTypeKey = "interpolation_type";
inline constexpr StringLiteral k_TranslateGeometryToGlobalOrigin_Key = "translate_geometry_to_global_origin";

const nx::core::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;

const std::string k_NearestNeighborInterpolation("Nearest Neighbor");
const std::string k_LinearInterpolation("Linear Interpolation");
const nx::core::ChoicesParameter::Choices k_InterpolationChoices = {k_NearestNeighborInterpolation, k_LinearInterpolation};

const nx::core::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const nx::core::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;

struct MHAHeaderInfo
{
  ImageRotationUtilities::Matrix4fR TransformationMatrix;
  std::array<float32, 3> CenterOfRotation = {0.0f, 0.0f, 0.0f};
  std::array<float32, 3> Origin = {0.0f, 0.0f, 0.0f};
  std::array<usize, 3> Dimensions = {0UL, 0UL, 0UL};
  std::vector<nx::core::Error> Errors;
  std::vector<nx::core::Warning> Warnings;
  std::vector<nx::core::IFilter::PreflightValue> PreflightUpdatedValues;
};

/**
 * @brief
 * @param transformMatrix
 * @return
 */
std::string generateTransformMatrixString(const ImageRotationUtilities::Matrix4fR& transformMatrix)
{
  std::string result;

  for(Eigen::Index r = 0; r < 4; r++)
  {
    result += "|  ";
    for(Eigen::Index c = 0; c < 4; c++)
    {
      result += std::to_string(transformMatrix(r, c));
      if(c < 3)
      {
        result += "\t  ";
      }
    }
    result += "  |\n";
  }

  return result;
}

/**
 * @brief
 * @param transformationMatrix
 * @return
 */
DynamicTableParameter::ValueType convertEigenMatrix(const ImageRotationUtilities::Matrix4fR& transformMatrix)
{
  return DynamicTableInfo::TableDataType{{transformMatrix(0, 0), transformMatrix(0, 1), transformMatrix(0, 2), transformMatrix(0, 3)},
                                         {transformMatrix(1, 0), transformMatrix(1, 1), transformMatrix(1, 2), transformMatrix(1, 3)},
                                         {transformMatrix(2, 0), transformMatrix(2, 1), transformMatrix(2, 2), transformMatrix(2, 3)},
                                         {transformMatrix(3, 0), transformMatrix(3, 1), transformMatrix(3, 2), transformMatrix(3, 3)}};
}

/**
 * @brief
 * @param filePath
 * @param transpose
 * @return
 */
MHAHeaderInfo readMhaHeader(const std::string& filePath, bool transpose)
{
  MHAHeaderInfo headerInfo;
  headerInfo.TransformationMatrix.fill(0.0F);
  headerInfo.TransformationMatrix(3, 3) = 1.0f;

  const itk::MetaImageIO::Pointer metaImageIO = itk::MetaImageIO::New();
  try
  {
    metaImageIO->SetFileName(filePath);
    metaImageIO->ReadImageInformation();
  } catch(itk::ExceptionObject& e)
  {
    headerInfo.Errors.push_back({-5000, fmt::format("Exception caught while reading image: {}", e.what())});
    return headerInfo;
  }

  /*
   * The ITK API hands back the Transform Matrix as either a raw pointer to the values,
   * or individual values if passed the row and column indices as parameters.
   * The actual Transform Matrix internal array has a size of 100 with only the first
   * several slots filled with matrix values (the rest of the slots are 0's).
   * Since it's only possible to get a raw pointer and no exact size, this code will
   * key off of the "NDims" header value in the .mha file to determine how many values
   * should be grabbed from the raw pointer.  So far it appears that it's 4 values (2x2)
   * for 2D image data and 9 values (3x3) for 3D image data, but it is currently unknown
   * whether it's possible to have >4 matrix values for 2D image data or >9 matrix values
   * for 3D image data.  Therefore, this code is going to make the following assumptions:
   *
   * 1. 2D image data has a transformation matrix with 4 values
   * 2. 3D image data has a transformation matrix with 9 values
   *
   * If these assumptions are violated, then unexpected behavior may occur.
   */
  MetaImage* metaImagePtr = metaImageIO->GetMetaImagePointer();
  const int nDims = metaImagePtr->NDims();
  if(nDims == 2)
  {
    headerInfo.TransformationMatrix(0, 0) = static_cast<float32>(metaImagePtr->TransformMatrix(0, 0));
    headerInfo.TransformationMatrix(0, 1) = static_cast<float32>(metaImagePtr->TransformMatrix(0, 1));
    headerInfo.TransformationMatrix(1, 0) = static_cast<float32>(metaImagePtr->TransformMatrix(1, 0));
    headerInfo.TransformationMatrix(1, 1) = static_cast<float32>(metaImagePtr->TransformMatrix(1, 1));
    headerInfo.TransformationMatrix(2, 2) = 1.0f;
  }
  else if(nDims == 3)
  {
    headerInfo.TransformationMatrix(0, 0) = static_cast<float32>(metaImagePtr->TransformMatrix(0, 0));
    headerInfo.TransformationMatrix(0, 1) = static_cast<float32>(metaImagePtr->TransformMatrix(0, 1));
    headerInfo.TransformationMatrix(0, 2) = static_cast<float32>(metaImagePtr->TransformMatrix(0, 2));
    headerInfo.TransformationMatrix(1, 0) = static_cast<float32>(metaImagePtr->TransformMatrix(1, 0));
    headerInfo.TransformationMatrix(1, 1) = static_cast<float32>(metaImagePtr->TransformMatrix(1, 1));
    headerInfo.TransformationMatrix(1, 2) = static_cast<float32>(metaImagePtr->TransformMatrix(1, 2));
    headerInfo.TransformationMatrix(2, 0) = static_cast<float32>(metaImagePtr->TransformMatrix(2, 0));
    headerInfo.TransformationMatrix(2, 1) = static_cast<float32>(metaImagePtr->TransformMatrix(2, 1));
    headerInfo.TransformationMatrix(2, 2) = static_cast<float32>(metaImagePtr->TransformMatrix(2, 2));
  }
  else
  {
    headerInfo.Errors.push_back({-5001, fmt::format("Image data has an unsupported number of dimensions ({}).  This filter only supports 2-dimensional and 3-dimensional image data.", nDims)});
  }

  headerInfo.CenterOfRotation[0] = static_cast<float32>(metaImagePtr->CenterOfRotation(static_cast<int>(0)));
  headerInfo.CenterOfRotation[1] = static_cast<float32>(metaImagePtr->CenterOfRotation(static_cast<int>(1)));
  headerInfo.CenterOfRotation[2] = static_cast<float32>(metaImagePtr->CenterOfRotation(static_cast<int>(2)));

  headerInfo.Origin[0] = static_cast<float32>(metaImagePtr->Offset(static_cast<int>(0)));
  headerInfo.Origin[1] = static_cast<float32>(metaImagePtr->Offset(static_cast<int>(1)));
  headerInfo.Origin[2] = static_cast<float32>(metaImagePtr->Offset(static_cast<int>(2)));

  headerInfo.Dimensions[0] = static_cast<usize>(metaImagePtr->DimSize(static_cast<int>(0)));
  headerInfo.Dimensions[1] = static_cast<usize>(metaImagePtr->DimSize(static_cast<int>(1)));
  headerInfo.Dimensions[2] = static_cast<usize>(metaImagePtr->DimSize(static_cast<int>(2)));
  for(auto& dim : headerInfo.Dimensions)
  {
    if(dim == 0)
    {
      dim = 1;
    }
  }

  // Generate all the PreflightUpdatedValues
  headerInfo.PreflightUpdatedValues.push_back({"Image Dimensions (voxels): ", fmt::format("{}", fmt::join(headerInfo.Dimensions, ",  "))});
  headerInfo.PreflightUpdatedValues.push_back({"Image Origin: ", fmt::format("{: > 8.8}", fmt::join(headerInfo.Origin, ",  "))});
  headerInfo.PreflightUpdatedValues.push_back({"Image Center of Rotation: ", fmt::format("{: > 8.8}", fmt::join(headerInfo.CenterOfRotation, ",  "))});

  float det = headerInfo.TransformationMatrix.determinant();

  if(transpose)
  {
    if(std::abs(1.0 - det) > 0.0001f)
    {
      headerInfo.Errors.push_back(
          {-5002,
           fmt::format("Transformation Matrix is NOT a pure rotation transform. A pure transpose will not work. De-select the option to transpose the matrix. Determinant of Transform matrix is {}",
                       det)});
    }
    headerInfo.TransformationMatrix.transposeInPlace();
  }
  headerInfo.PreflightUpdatedValues.push_back({"Transformation Matrix: ", generateTransformMatrixString(headerInfo.TransformationMatrix)});
  headerInfo.PreflightUpdatedValues.push_back({"Transform Matrix Determinant: ", fmt::format("{}", det)});

  return headerInfo;
}

/**
 * @brief
 */
struct CopyImageDataFunctor
{
  template <typename T>
  Result<> operator()(const DataStructure& srcDataStructure, DataStructure& destDataStructure, DataPath dataArrayPath, const fs::path& fileNamePath)
  {
    const DataArray<T>& srcDataArray = srcDataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
    DataArray<T>& destDataArray = destDataStructure.getDataRefAs<DataArray<T>>(dataArrayPath);
    auto& srcDataStore = srcDataArray.getDataStoreRef();
    auto& destDataStore = destDataArray.getDataStoreRef();
    // Copy imported array into the data structure
    if(!destDataStore.copyFrom(0, srcDataStore, 0, srcDataStore.getNumberOfTuples()))
    {
      return MakeErrorResult(-5008, fmt::format("Error copying image data read from file '{}' into destination array.", fileNamePath.string()));
    }
    return {};
  }
};
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKMhaFileReader::name() const
{
  return FilterTraits<ITKMhaFileReader>::name;
}

//------------------------------------------------------------------------------
std::string ITKMhaFileReader::className() const
{
  return FilterTraits<ITKMhaFileReader>::className;
}

//------------------------------------------------------------------------------
Uuid ITKMhaFileReader::uuid() const
{
  return FilterTraits<ITKMhaFileReader>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKMhaFileReader::humanName() const
{
  return "ITK MHA File Reader";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKMhaFileReader::defaultTags() const
{
  return {className(), "io", "input", "read", "import", "image", "ITK", "MHA"};
}

//------------------------------------------------------------------------------
Parameters ITKMhaFileReader::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(ITKImageReader::k_FileName_Key, "Input MHA File", "The input .mha file that will be read.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".mha"}}, FileSystemPathParameter::PathType::InputFile, false));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyImageTransformation, "Apply Image Transformation To Geometry",
                                                                 "When true, the transformation matrix found in the image's header metadata will be applied to the created image geometry.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationTypeKey, "Interpolation Type", "The type of interpolation algorithm that is used. 0=NearestNeighbor, 1=Linear",
                                                   k_NearestNeighborInterpolationIdx, k_InterpolationChoices));

  params.insertSeparator(Parameters::Separator{"Transformation Matrix Options"});
  params.insert(std::make_unique<BoolParameter>(k_TransposeTransformMatrix, "Transpose Stored Transformation Matrix",
                                                "When true, the transformation matrix found in the image's header metadata will be transposed before use.", false));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SaveImageTransformationAsArray, "Save Image Transformation As Array",
                                      "When true, the transformation matrix found in the image's header metadata will be saved as a data array in the created image geometry.", false));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationMatrixDataArrayPathKey, "Transformation Matrix", "The path to the transformation matrix data array",
                                                         DataPath({"ImageDataContainer", "TransformationMatrix"})));

  params.insertSeparator(Parameters::Separator{"Created Image Data Objects"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(ITKImageReader::k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));

  params.insert(
      std::make_unique<DataObjectNameParameter>(ITKImageReader::k_CellDataName_Key, "Created Cell Attribute Matrix", "The name of the created cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(ITKImageReader::k_ImageDataArrayPath_Key, "Created Cell Data",
                                                          "The name of the created image data array. Will be stored in the created Cell Attribute Matrix", "ImageData"));

  params.linkParameters(k_SaveImageTransformationAsArray, k_TransformationMatrixDataArrayPathKey, true);
  params.linkParameters(k_ApplyImageTransformation, k_InterpolationTypeKey, true);

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKMhaFileReader::clone() const
{
  return std::make_unique<ITKMhaFileReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKMhaFileReader::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel) const
{
  auto fileNamePath = filterArgs.value<FileSystemPathParameter::ValueType>(ITKImageReader::k_FileName_Key);
  auto imageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(ITKImageReader::k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(ITKImageReader::k_CellDataName_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(ITKImageReader::k_ImageDataArrayPath_Key);
  auto saveImageTransformationAsArray = filterArgs.value<BoolParameter::ValueType>(k_SaveImageTransformationAsArray);
  auto transformationMatrixDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_TransformationMatrixDataArrayPathKey);
  auto applyImageTransformation = filterArgs.value<BoolParameter::ValueType>(k_ApplyImageTransformation);
  auto transposeTransform = filterArgs.value<BoolParameter::ValueType>(k_TransposeTransformMatrix);
  auto interpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationTypeKey);

  DataPath imageDataArrayPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataArrayName);
  nx::core::Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  {
    const ITKImageReader imageReaderFilter;
    Arguments imageReaderArgs;
    imageReaderArgs.insertOrAssign(ITKImageReader::k_FileName_Key, fileNamePath);
    imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imageGeomPath);
    imageReaderArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, cellDataName);
    imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, imageDataArrayName);
    IFilter::PreflightResult preflightResult = imageReaderFilter.preflight(dataStructure, imageReaderArgs, messageHandler, shouldCancel);
    if(preflightResult.outputActions.invalid())
    {
      resultOutputActions.m_Warnings.insert(resultOutputActions.m_Warnings.end(), preflightResult.outputActions.m_Warnings.begin(), preflightResult.outputActions.m_Warnings.end());
      resultOutputActions.errors().insert(resultOutputActions.errors().end(), preflightResult.outputActions.errors().begin(), preflightResult.outputActions.errors().end());
      return {resultOutputActions, preflightUpdatedValues};
    }
    preflightUpdatedValues = std::move(preflightResult.outputValues);
    resultOutputActions = std::move(preflightResult.outputActions);
  }

  // Read the header information from the MHA file
  MHAHeaderInfo headerInfo = readMhaHeader(fileNamePath.string(), transposeTransform);
  // Transfer any warnings that were produced.
  resultOutputActions.m_Warnings.insert(resultOutputActions.m_Warnings.begin(), headerInfo.Warnings.begin(), headerInfo.Warnings.end());
  // Append any Preflight updated values generated when reading the MHA Header
  preflightUpdatedValues.insert(preflightUpdatedValues.end(), headerInfo.PreflightUpdatedValues.begin(), headerInfo.PreflightUpdatedValues.end());
  // Check for errors that happened during the MHA Header reading
  if(!headerInfo.Errors.empty())
  {
    // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
    return {{nonstd::make_unexpected(headerInfo.Errors), resultOutputActions.m_Warnings}, preflightUpdatedValues};
  }

  // If the user wants to save the Transform Matrix as an Array
  if(saveImageTransformationAsArray)
  {
    auto createArrayAction = std::make_unique<CreateArrayAction>(nx::core::DataType::float32, std::vector<usize>{1}, std::vector<usize>{16}, transformationMatrixDataArrayPath);
    resultOutputActions.value().appendAction(std::move(createArrayAction));
  }

  // If the user wants to apply the transform matrix to the incoming image
  if(applyImageTransformation)
  {
    auto* filterListPtr = Application::Instance()->getFilterList();
    auto applyTransformationToGeometryFilter = filterListPtr->createFilter(k_ApplyTransformationToGeometryFilterHandle);
    if(applyTransformationToGeometryFilter == nullptr)
    {
      resultOutputActions.errors().push_back({-5001, fmt::format("Error preflighting application of transformation to geometry - Unable to instantiate filter 'Apply Transformation To Geometry'.")});
      return {resultOutputActions, preflightUpdatedValues};
    }

    Arguments args;
    args.insertOrAssign(k_SelectedImageGeometryKey, std::make_any<GeometrySelectionParameter::ValueType>(DataPath{imageGeomPath}));
    args.insertOrAssign(k_CellAttributeMatrixPathKey, std::make_any<AttributeMatrixSelectionParameter::ValueType>(DataPath{imageGeomPath.createChildPath(cellDataName)}));
    args.insertOrAssign(k_TransformationTypeKey, std::make_any<ChoicesParameter::ValueType>(k_ManualTransformationMatrixIdx));
    args.insertOrAssign(k_ManualTransformationMatrixKey, std::make_any<DynamicTableParameter::ValueType>(convertEigenMatrix(headerInfo.TransformationMatrix)));
    args.insertOrAssign(k_InterpolationTypeKey, std::make_any<ChoicesParameter::ValueType>(interpolationType));

    if(headerInfo.CenterOfRotation == std::array<float32, 3>{0.0f, 0.0f, 0.0f})
    {
      args.insertOrAssign(k_TranslateGeometryToGlobalOrigin_Key, std::make_any<BoolParameter::ValueType>(true));
    }

    // Preflight the filter
    IFilter::PreflightResult applyTransResult = applyTransformationToGeometryFilter->preflight(dataStructure, args, messageHandler, shouldCancel);
    // Copy over any preflight updated values
    preflightUpdatedValues.insert(preflightUpdatedValues.end(), applyTransResult.outputValues.begin(), applyTransResult.outputValues.end());
    // Copy over any errors/warnings

    // preflightResult.outputActions.errors().insert( preflightResult.outputActions.errors().begin(), applyTransResult.outputActions.errors().begin(), applyTransResult.outputActions.errors().end());
    // preflightResult.outputActions.warnings().insert( preflightResult.outputActions.warnings().begin(), applyTransResult.outputActions.warnings().begin(),
    // applyTransResult.outputActions.warnings().end());
  }

  return {resultOutputActions, preflightUpdatedValues};
}

//------------------------------------------------------------------------------
Result<> ITKMhaFileReader::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  auto fileNamePath = filterArgs.value<FileSystemPathParameter::ValueType>(ITKImageReader::k_FileName_Key);
  auto imageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(ITKImageReader::k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(ITKImageReader::k_CellDataName_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(ITKImageReader::k_ImageDataArrayPath_Key);
  auto saveImageTransformationAsArray = filterArgs.value<BoolParameter::ValueType>(k_SaveImageTransformationAsArray);
  auto transformationMatrixDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_TransformationMatrixDataArrayPathKey);
  auto applyImageTransformation = filterArgs.value<BoolParameter::ValueType>(k_ApplyImageTransformation);
  auto interpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationTypeKey);
  auto transposeTransform = filterArgs.value<BoolParameter::ValueType>(k_TransposeTransformMatrix);

  DataPath imageDataArrayPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataArrayName);

  Result<> mainResult;

  // Execute the ImageFileReader filter
  messageHandler(fmt::format("Reading image file '{}'...", fileNamePath.string()));
  {
    DataStructure importedDataStructure;
    const ITKImageReader imageReaderFilter;
    Arguments args;
    args.insertOrAssign(ITKImageReader::k_FileName_Key, fileNamePath);
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imageGeomPath);
    args.insertOrAssign(ITKImageReader::k_CellDataName_Key, cellDataName);
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, imageDataArrayName);

    auto executeResult = imageReaderFilter.execute(importedDataStructure, args, pipelineNode, messageHandler, shouldCancel);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
    }

    const auto& srcDataArray = importedDataStructure.getDataRefAs<IDataArray>(imageDataArrayPath);
    const auto& destDataArray = dataStructure.getDataRefAs<IDataArray>(imageDataArrayPath);
    messageHandler(fmt::format("Copying image data to destination array '{}'...", destDataArray.getName()));

    Result<> copyResult = ExecuteDataFunction(CopyImageDataFunctor{}, srcDataArray.getDataType(), importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
    if(copyResult.invalid())
    {
      return copyResult;
    }

    mainResult = MergeResults(executeResult.result, copyResult);
  }

  if(applyImageTransformation || saveImageTransformationAsArray)
  {
    messageHandler(fmt::format("Reading transformation matrix from image file '{}'...", fileNamePath.string()));
    MHAHeaderInfo headerInfo = readMhaHeader(fileNamePath.string(), transposeTransform);

    if(!headerInfo.Errors.empty())

    {
      return {{nonstd::make_unexpected(headerInfo.Errors)}};
    }

    if(saveImageTransformationAsArray)
    {
      Float32Array& transformationMatrixArray = dataStructure.getDataRefAs<Float32Array>(transformationMatrixDataArrayPath);
      // Eigen 3.4 has STL iterators.
      std::copy(headerInfo.TransformationMatrix.data(), headerInfo.TransformationMatrix.data() + 16, transformationMatrixArray.begin());
    }

    if(applyImageTransformation)
    {
      const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
      messageHandler(fmt::format("Applying transformation matrix to image geometry '{}'...", imageGeom.getName()));
      auto* filterListPtr = Application::Instance()->getFilterList();
      auto applyTransformationToGeometryFilter = filterListPtr->createFilter(k_ApplyTransformationToGeometryFilterHandle);
      if(applyTransformationToGeometryFilter == nullptr)
      {
        return {MakeErrorResult(-5001, fmt::format("Error applying transformation to geometry - Unable to instantiate filter 'Apply Transformation To Geometry'."))};
      }

      Arguments args;
      args.insertOrAssign(k_SelectedImageGeometryKey, std::make_any<GeometrySelectionParameter::ValueType>(DataPath{imageGeomPath}));
      args.insertOrAssign(k_CellAttributeMatrixPathKey, std::make_any<AttributeMatrixSelectionParameter::ValueType>(DataPath{imageGeomPath.createChildPath(cellDataName)}));
      args.insertOrAssign(k_TransformationTypeKey, std::make_any<ChoicesParameter::ValueType>(k_ManualTransformationMatrixIdx));
      args.insertOrAssign(k_ManualTransformationMatrixKey, std::make_any<DynamicTableParameter::ValueType>(convertEigenMatrix(headerInfo.TransformationMatrix)));
      args.insertOrAssign(k_InterpolationTypeKey, std::make_any<ChoicesParameter::ValueType>(interpolationType));
      if(headerInfo.CenterOfRotation == std::array<float32, 3>{0.0f, 0.0f, 0.0f})
      {
        args.insertOrAssign(k_TranslateGeometryToGlobalOrigin_Key, std::make_any<BoolParameter::ValueType>(true));
      }

      {
        const ExecuteResult result = applyTransformationToGeometryFilter->execute(dataStructure, args, pipelineNode, messageHandler, shouldCancel);
        if(result.result.invalid())
        {
          return result.result;
        }

        mainResult = MergeResults(mainResult, result.result);
      }
    }
  }

  return mainResult;
}
} // namespace nx::core
