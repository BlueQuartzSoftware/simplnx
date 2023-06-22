#include "ITKMhaFileReader.hpp"

#include <filesystem>

#include "itkMetaImageIO.h"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/Core/Application.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/DynamicTableParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeometrySelectionParameter.hpp"

namespace fs = std::filesystem;

using namespace complex;

namespace
{
const Uuid k_ComplexCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_ApplyTransformationToGeometryFilterId = *Uuid::FromString("f5bbc16b-3426-4ae0-b27b-ba7862dc40fe");
const FilterHandle k_ApplyTransformationToGeometryFilterHandle(k_ApplyTransformationToGeometryFilterId, k_ComplexCorePluginId);

static inline constexpr StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
static inline constexpr StringLiteral k_CellAttributeMatrixPath_Key = "cell_attribute_matrix_path";
static inline constexpr StringLiteral k_TransformationType_Key = "transformation_type";
static inline constexpr StringLiteral k_ManualTransformationMatrix_Key = "manual_transformation_matrix";
static inline constexpr StringLiteral k_InterpolationType_Key = "interpolation_type";
const complex::ChoicesParameter::ValueType k_ManualTransformationMatrixIdx = 2ULL;

const std::string k_NearestNeighborInterpolation("Nearest Neighbor");
const std::string k_LinearInterpolation("Linear Interpolation");
const complex::ChoicesParameter::Choices k_InterpolationChoices = {k_NearestNeighborInterpolation, k_LinearInterpolation};

const complex::ChoicesParameter::ValueType k_NearestNeighborInterpolationIdx = 0ULL;
const complex::ChoicesParameter::ValueType k_LinearInterpolationIdx = 1ULL;

Result<std::array<float32, 16>> ReadTransformMatrix(const std::string& filePath)
{
  std::array<float32, 16> arr;

  itk::MetaImageIO::Pointer metaImageIO = itk::MetaImageIO::New();

  try
  {
    metaImageIO->SetFileName(filePath);
    metaImageIO->ReadImageInformation();
  } catch(itk::ExceptionObject& e)
  {
    return {MakeErrorResult<std::array<float32, 16>>(-5000, fmt::format("Exception caught while reading image: {}", e.what()))};
  }

  MetaImage* metaImagePtr = metaImageIO->GetMetaImagePointer();
  const double* transformMatrixPtr = metaImagePtr->TransformMatrix();

  /*
   * The ITK API hands back the Transform Matrix as a raw pointer to the values.
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
  int nDims = metaImagePtr->NDims();
  if(nDims == 2)
  {
    std::copy(transformMatrixPtr, transformMatrixPtr + 2, arr.begin());
    arr[2] = 0;
    arr[3] = 0;
    std::copy(transformMatrixPtr + 2, transformMatrixPtr + 4, arr.begin() + 4);
    arr[6] = 0;
    arr[7] = 0;
    arr[8] = 0;
    arr[9] = 0;
    arr[10] = 1;
    arr[11] = 0;
    arr[12] = 0;
    arr[13] = 0;
    arr[14] = 0;
    arr[15] = 1;
  }
  else if(nDims == 3)
  {
    std::copy(transformMatrixPtr, transformMatrixPtr + 3, arr.begin());
    arr[3] = 0;
    std::copy(transformMatrixPtr + 3, transformMatrixPtr + 6, arr.begin() + 4);
    arr[7] = 0;
    std::copy(transformMatrixPtr + 6, transformMatrixPtr + 9, arr.begin() + 8);
    arr[11] = 0;
    arr[12] = 0;
    arr[13] = 0;
    arr[14] = 0;
    arr[15] = 1;
  }
  else
  {
    return {MakeErrorResult<std::array<float32, 16>>(
        -5001, fmt::format("Image data has an unsupported number of dimensions ({}).  This filter only supports 2-dimensional and 3-dimensional image data.", nDims))};
  }

  return {arr};
}

std::string GenerateTransformMatrixString(const std::array<float32, 16> transformMatrix)
{
  std::string result;

  for(size_t i = 0; i < transformMatrix.size(); ++i)
  {
    // Add a starting bracket at the beginning of each row
    if(i % 4 == 0)
    {
      result += "[";
    }

    result += std::to_string(transformMatrix[i]);

    // Add a closing bracket at the end of every 4 elements
    if(i % 4 == 3)
    {
      result += "]\n";
    }
    else
    {
      result += ", ";
    }
  }

  return result;
}

template <class T>
Result<> CopyImageData(const DataStructure& srcDataStructure, DataStructure& destDataStructure, DataPath dataArrayPath, fs::path fileNamePath)
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
} // namespace

namespace complex
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
Parameters ITKMhaFileReader::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Filter Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(ITKImageReader::k_FileName_Key, "Input MHA File", "The input .mha file that will be read.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".mha"}}, FileSystemPathParameter::PathType::InputFile, false));
  params.insertLinkableParameter(
      std::make_unique<BoolParameter>(k_SaveImageTransformationAsArray, "Save Image Transformation As Array",
                                      "When true, the transformation matrix found in the image's header metadata will be saved as a data array in the created image geometry.", false));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ApplyImageTransformation, "Apply Image Transformation To Geometry",
                                                                 "When true, the transformation matrix found in the image's header metadata will be applied to the created image geometry.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_InterpolationType_Key, "Interpolation Type", "", k_NearestNeighborInterpolationIdx, k_InterpolationChoices));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(
      std::make_unique<DataGroupCreationParameter>(ITKImageReader::k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(ITKImageReader::k_CellDataName_Key, "Cell Data Name", "The name of the created cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<ArrayCreationParameter>(ITKImageReader::k_ImageDataArrayPath_Key, "Created Image Data", "The path to the created image data array",
                                                         DataPath({"ImageDataContainer", ImageGeom::k_CellDataName, "ImageData"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_TransformationMatrixDataArrayPath_Key, "Transformation Matrix", "The path to the transformation matrix data array",
                                                         DataPath({"ImageDataContainer", "TransformationMatrix"})));

  params.linkParameters(k_SaveImageTransformationAsArray, k_TransformationMatrixDataArrayPath_Key, true);
  params.linkParameters(k_ApplyImageTransformation, k_InterpolationType_Key, true);

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
  auto imageDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(ITKImageReader::k_ImageDataArrayPath_Key);
  auto saveImageTransformationAsArray = filterArgs.value<BoolParameter::ValueType>(k_SaveImageTransformationAsArray);
  auto transformationMatrixDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_TransformationMatrixDataArrayPath_Key);
  auto applyImageTransformation = filterArgs.value<BoolParameter::ValueType>(k_ApplyImageTransformation);
  auto interpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  IFilter::PreflightResult preflightResult;

  {
    ITKImageReader imageReaderFilter;
    Arguments imageReaderArgs;
    imageReaderArgs.insertOrAssign(ITKImageReader::k_FileName_Key, fileNamePath);
    imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imageGeomPath);
    imageReaderArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, cellDataName);
    imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, imageDataArrayPath);
    preflightResult = imageReaderFilter.preflight(dataStructure, imageReaderArgs, messageHandler, shouldCancel);
    if(preflightResult.outputActions.invalid())
    {
      return preflightResult;
    }
  }

  if(applyImageTransformation || saveImageTransformationAsArray)
  {
    Result<std::array<float32, 16>> transformMatrixResult = ReadTransformMatrix(fileNamePath.string());
    if(transformMatrixResult.invalid())
    {
      auto voidResult = ConvertResult(std::move(transformMatrixResult));
      return {ConvertResultTo<OutputActions>(std::move(voidResult), {})};
    }

    std::array<float32, 16>& transformMatrix = transformMatrixResult.value();

    auto* filterList = Application::Instance()->getFilterList();
    auto applyTransformationToGeometryFilter = filterList->createFilter(k_ApplyTransformationToGeometryFilterHandle);
    if(applyTransformationToGeometryFilter == nullptr)
    {
      return {MakeErrorResult<OutputActions>(-5001, fmt::format("Error preflighting application of transformation to geometry - Unable to instantiate filter 'Apply Transformation To Geometry'."))};
    }

    Arguments args;
    // read in the exemplar shift data file
    args.insertOrAssign(k_SelectedImageGeometry_Key, std::make_any<GeometrySelectionParameter::ValueType>(DataPath{imageGeomPath}));
    args.insertOrAssign(k_CellAttributeMatrixPath_Key, std::make_any<AttributeMatrixSelectionParameter::ValueType>(DataPath{imageGeomPath.createChildPath(cellDataName)}));
    args.insertOrAssign(k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(k_ManualTransformationMatrixIdx));
    args.insertOrAssign(k_ManualTransformationMatrix_Key,
                        std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{transformMatrix[0], transformMatrix[1], transformMatrix[2], transformMatrix[3]},
                                                                                                        {transformMatrix[4], transformMatrix[5], transformMatrix[6], transformMatrix[7]},
                                                                                                        {transformMatrix[8], transformMatrix[9], transformMatrix[10], transformMatrix[11]},
                                                                                                        {transformMatrix[12], transformMatrix[13], transformMatrix[14], transformMatrix[15]}}));
    args.insertOrAssign(k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(interpolationType));

    preflightResult.outputValues.push_back({"Image Transformation Matrix", GenerateTransformMatrixString(transformMatrix)});

    if(saveImageTransformationAsArray)
    {
      auto createArrayAction = std::make_unique<CreateArrayAction>(complex::DataType::float32, std::vector<usize>{1}, std::vector<usize>{transformMatrix.size()}, transformationMatrixDataArrayPath);
      preflightResult.outputActions.value().appendAction(std::move(createArrayAction));
    }
  }

  return preflightResult;
}

//------------------------------------------------------------------------------
Result<> ITKMhaFileReader::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                       const std::atomic_bool& shouldCancel) const
{
  auto fileNamePath = filterArgs.value<FileSystemPathParameter::ValueType>(ITKImageReader::k_FileName_Key);
  auto imageGeomPath = filterArgs.value<DataGroupCreationParameter::ValueType>(ITKImageReader::k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(ITKImageReader::k_CellDataName_Key);
  auto imageDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(ITKImageReader::k_ImageDataArrayPath_Key);
  auto saveImageTransformationAsArray = filterArgs.value<BoolParameter::ValueType>(k_SaveImageTransformationAsArray);
  auto transformationMatrixDataArrayPath = filterArgs.value<ArrayCreationParameter::ValueType>(k_TransformationMatrixDataArrayPath_Key);
  auto applyImageTransformation = filterArgs.value<BoolParameter::ValueType>(k_ApplyImageTransformation);
  auto interpolationType = filterArgs.value<ChoicesParameter::ValueType>(k_InterpolationType_Key);
  Result<> mainResult;

  // Execute the ImageFileReader filter
  messageHandler(fmt::format("Reading image file '{}'...", fileNamePath.string()));
  {
    DataStructure importedDataStructure;
    ITKImageReader imageReaderFilter;
    Arguments args;
    args.insertOrAssign(ITKImageReader::k_FileName_Key, fileNamePath);
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, imageGeomPath);
    args.insertOrAssign(ITKImageReader::k_CellDataName_Key, cellDataName);
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, imageDataArrayPath);

    auto executeResult = imageReaderFilter.execute(importedDataStructure, args, pipelineNode, messageHandler, shouldCancel);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
    }

    Result<> copyResult;
    const auto& srcDataArray = importedDataStructure.getDataRefAs<IDataArray>(imageDataArrayPath);
    const auto& destDataArray = dataStructure.getDataRefAs<IDataArray>(imageDataArrayPath);
    messageHandler(fmt::format("Copying image data to destination array '{}'...", destDataArray.getName()));
    switch(srcDataArray.getDataType())
    {
    case DataType::uint8: {
      copyResult = CopyImageData<uint8>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::int8: {
      copyResult = CopyImageData<int8>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::uint16: {
      copyResult = CopyImageData<uint16>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::int16: {
      copyResult = CopyImageData<int16>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::uint32: {
      copyResult = CopyImageData<uint32>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::int32: {
      copyResult = CopyImageData<int32>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::uint64: {
      copyResult = CopyImageData<uint64>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::int64: {
      copyResult = CopyImageData<int64>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::float32: {
      copyResult = CopyImageData<float32>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::float64: {
      copyResult = CopyImageData<float64>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    case DataType::boolean: {
      copyResult = CopyImageData<bool>(importedDataStructure, dataStructure, imageDataArrayPath, fileNamePath);
      break;
    }
    default: {
      return MakeErrorResult(-5006, fmt::format("The created data array '{}' does not have a compatible DataType.", srcDataArray.getName()));
    }
    }

    mainResult = MergeResults(executeResult.result, copyResult);
  }

  if(applyImageTransformation || saveImageTransformationAsArray)
  {
    messageHandler(fmt::format("Reading transformation matrix from image file '{}'...", fileNamePath.string()));
    Result<std::array<float32, 16>> transformMatrixResult = ReadTransformMatrix(fileNamePath.string());
    if(transformMatrixResult.invalid())
    {
      return ConvertResult(std::move(transformMatrixResult));
    }

    std::array<float32, 16> transformMat = transformMatrixResult.value();

    if(saveImageTransformationAsArray)
    {
      Float32Array& transformationMatrixArray = dataStructure.getDataRefAs<Float32Array>(transformationMatrixDataArrayPath);
      std::copy(transformMat.begin(), transformMat.end(), transformationMatrixArray.begin());
    }

    if(applyImageTransformation)
    {
      const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
      messageHandler(fmt::format("Applying transformation matrix to image geometry '{}'...", imageGeom.getName()));
      auto* filterList = Application::Instance()->getFilterList();
      auto applyTransformationToGeometryFilter = filterList->createFilter(k_ApplyTransformationToGeometryFilterHandle);
      if(applyTransformationToGeometryFilter == nullptr)
      {
        return {MakeErrorResult(-5001, fmt::format("Error applying transformation to geometry - Unable to instantiate filter 'Apply Transformation To Geometry'."))};
      }

      Arguments args;
      // read in the exemplar shift data file
      args.insertOrAssign(k_SelectedImageGeometry_Key, std::make_any<GeometrySelectionParameter::ValueType>(DataPath{imageGeomPath}));
      args.insertOrAssign(k_CellAttributeMatrixPath_Key, std::make_any<AttributeMatrixSelectionParameter::ValueType>(DataPath{imageGeomPath.createChildPath(cellDataName)}));
      args.insertOrAssign(k_TransformationType_Key, std::make_any<ChoicesParameter::ValueType>(k_ManualTransformationMatrixIdx));
      args.insertOrAssign(k_ManualTransformationMatrix_Key,
                          std::make_any<DynamicTableParameter::ValueType>(DynamicTableInfo::TableDataType{{transformMat[0], transformMat[1], transformMat[2], transformMat[3]},
                                                                                                          {transformMat[4], transformMat[5], transformMat[6], transformMat[7]},
                                                                                                          {transformMat[8], transformMat[9], transformMat[10], transformMat[11]},
                                                                                                          {transformMat[12], transformMat[13], transformMat[14], transformMat[15]}}));
      args.insertOrAssign(k_InterpolationType_Key, std::make_any<ChoicesParameter::ValueType>(interpolationType));

      {
        ExecuteResult result = applyTransformationToGeometryFilter->execute(dataStructure, args, pipelineNode, messageHandler, shouldCancel);
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
} // namespace complex
