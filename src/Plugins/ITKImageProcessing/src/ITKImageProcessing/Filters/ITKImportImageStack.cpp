#include "ITKImportImageStack.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

#include <itkImageFileReader.h>
#include <itkImageIOBase.h>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace RotateSampleRefFrame
{
// Parameter Keys
static inline constexpr nx::core::StringLiteral k_RotationRepresentation_Key = "rotation_representation";
static inline constexpr nx::core::StringLiteral k_RotationAxisAngle_Key = "rotation_axis";
static inline constexpr nx::core::StringLiteral k_RotationMatrix_Key = "rotation_matrix";
static inline constexpr nx::core::StringLiteral k_SelectedImageGeometry_Key = "selected_image_geometry";
static inline constexpr nx::core::StringLiteral k_CreatedImageGeometry_Key = "created_image_geometry";
static inline constexpr nx::core::StringLiteral k_RotateSliceBySlice_Key = "rotate_slice_by_slice";
static inline constexpr nx::core::StringLiteral k_RemoveOriginalGeometry_Key = "remove_original_geometry";

// static inline constexpr nx::core::StringLiteral k_RotatedGeometryName = ".RotatedGeometry";

enum class RotationRepresentation : uint64_t
{
  AxisAngle = 0,
  RotationMatrix = 1
};

} // namespace RotateSampleRefFrame

namespace
{
const ChoicesParameter::Choices k_SliceOperationChoices = {"None", "Flip about X axis", "Flip about Y axis"};
const ChoicesParameter::ValueType k_NoImageTransform = 0;
const ChoicesParameter::ValueType k_FlipAboutXAxis = 1;
const ChoicesParameter::ValueType k_FlipAboutYAxis = 2;

const Uuid k_SimplnxCorePluginId = *Uuid::FromString("05cc618b-781f-4ac0-b9ac-43f26ce1854f");
const Uuid k_RotateSampleRefFrameFilterId = *Uuid::FromString("d2451dc1-a5a1-4ac2-a64d-7991669dcffc");
const FilterHandle k_RotateSampleRefFrameFilterHandle(k_RotateSampleRefFrameFilterId, k_SimplnxCorePluginId);

// Make sure we can instantiate the RotateSampleRefFrame Filter
std::unique_ptr<IFilter> CreateRotateSampleRefFrameFilter()
{
  auto* filterListPtr = Application::Instance()->getFilterList();
  auto filter = filterListPtr->createFilter(k_RotateSampleRefFrameFilterHandle);
  return filter;
}

template <class T>
void FlipAboutYAxis(DataArray<T>& dataArray, Vec3<usize>& dims)
{
  auto& tempDataStore = dataArray.getDataStoreRef();

  usize numComp = tempDataStore.getNumberOfComponents();
  std::vector<T> currentRowBuffer(dims[0] * dataArray.getNumberOfComponents());

  // We could _in theory_ parallelize over the rows, not sure how the out-of-core
  // would handle that though.
  for(usize row = 0; row < dims[1]; row++)
  {
    // Copy the current row into a temp buffer
    auto startIter = tempDataStore.begin() + (dims[0] * numComp * row);
    auto endIter = startIter + dims[0] * numComp;
    std::copy(startIter, endIter, currentRowBuffer.begin());

    // Starting at the last tuple in the buffer
    usize bufferIndex = (dims[0] - 1) * numComp;
    usize dataStoreIndex = row * dims[0] * numComp;

    for(usize tupleIdx = 0; tupleIdx < dims[0]; tupleIdx++)
    {
      for(usize cIdx = 0; cIdx < numComp; cIdx++)
      {
        tempDataStore.setValue(dataStoreIndex, currentRowBuffer[bufferIndex + cIdx]);
        dataStoreIndex++;
      }
      bufferIndex = bufferIndex - numComp;
    }
  }
}

template <class T>
void FlipAboutXAxis(DataArray<T>& dataArray, Vec3<usize>& dims)
{
  auto& tempDataStore = dataArray.getDataStoreRef();
  usize numComp = tempDataStore.getNumberOfComponents();
  size_t rowLCV = (dims[1] % 2 == 1) ? (dims[1] - 1 / 2) : dims[1] / 2;
  usize bottomRow = dims[1] - 1;

  for(usize row = 0; row < rowLCV; row++)
  {
    // Copy the "top" row into a temp buffer
    auto topStartIter = 0 + (dims[0] * numComp * row);
    auto topEndIter = topStartIter + dims[0] * numComp;
    auto bottomStartIter = 0 + (dims[0] * numComp * bottomRow);

    // Copy from bottom to top and then temp to bottom
    for(usize eleIndex = topStartIter; eleIndex < topEndIter; eleIndex++)
    {
      T value = tempDataStore.getValue(eleIndex);
      tempDataStore[eleIndex] = tempDataStore[bottomStartIter];
      tempDataStore[bottomStartIter] = value;
      bottomStartIter++;
    }
    bottomRow--;
  }
}

} // namespace

namespace cxITKImportImageStack
{
template <class T>
Result<> ReadImageStack(DataStructure& dataStructure, const DataPath& imageGeomPath, const std::string& cellDataName, const DataPath& imageDataPath, const std::vector<std::string>& files,
                        ChoicesParameter::ValueType transformType, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel)
{
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  SizeVec3 dims = imageGeom.getDimensions();
  const usize tuplesPerSlice = dims[0] * dims[1];

  auto& outputData = dataStructure.getDataRefAs<DataArray<T>>(imageDataPath);
  auto& outputDataStore = outputData.getDataStoreRef();

  imageGeom.getLinkedGeometryData().addCellData(imageDataPath);

  // Variables for the progress Reporting
  usize slice = 0;

  // Loop over all the files importing them one by one and copying the data into the data array
  for(const auto& filePath : files)
  {
    messageHandler(IFilter::Message::Type::Info, fmt::format("Importing: {}", filePath));

    DataStructure importedDataStructure;

    {
      // Create a sub-filter to read each image, although for preflight we are going to read the first image in the
      // list and hope the rest are correct.
      const ITKImageReader imageReader;

      Arguments args;
      args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
      args.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<std::string>(cellDataName));
      args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(imageDataPath));
      args.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(filePath));

      auto executeResult = imageReader.execute(importedDataStructure, args);
      if(executeResult.result.invalid())
      {
        return executeResult.result;
      }
    }
    // Check the ImageGeometry of the imported Image matches the destination
    const auto& importedImageGeom = importedDataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    SizeVec3 importedDims = importedImageGeom.getDimensions();
    if(dims[0] != importedDims[0] || dims[1] != importedDims[1])
    {
      return MakeErrorResult(-64510, fmt::format("Slice {} image dimensions are different than the first slice.\n  First Slice Dims are:  {} x {}\n  Current Slice Dims are:{} x {}\n", slice,
                                                 importedDims[0], importedDims[1], dims[0], dims[1]));
    }

    // Compute the Tuple Index we are at:
    const usize tupleIndex = (slice * dims[0] * dims[1]);
    // get the current Slice data...
    auto& tempData = importedDataStructure.getDataRefAs<DataArray<T>>(imageDataPath);
    auto& tempDataStore = tempData.getDataStoreRef();

    if(transformType == k_FlipAboutYAxis)
    {
      FlipAboutYAxis<T>(tempData, dims);
    }
    else if(transformType == k_FlipAboutXAxis)
    {
      FlipAboutXAxis<T>(tempData, dims);
    }

    // Copy that into the output array...
    if(!outputDataStore.copyFrom(tupleIndex, tempDataStore, 0, tuplesPerSlice))
    {
      return MakeErrorResult(-64511, fmt::format("Error copying source image data into destination array.\n  Slice:{}\n  TupleIndex:{}\n  MaxTupleIndex:{}", slice, tupleIndex, outputData.getSize()));
    }

    slice++;

    // Check to see if the filter got canceled.
    if(shouldCancel)
    {
      return {};
    }
  }

  return {};
}
} // namespace cxITKImportImageStack

namespace nx::core
{

//------------------------------------------------------------------------------
std::string ITKImportImageStack::name() const
{
  return FilterTraits<ITKImportImageStack>::name;
}

//------------------------------------------------------------------------------
std::string ITKImportImageStack::className() const
{
  return FilterTraits<ITKImportImageStack>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImportImageStack::uuid() const
{
  return FilterTraits<ITKImportImageStack>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImportImageStack::humanName() const
{
  return "Read Images [3D Stack] (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportImageStack::defaultTags() const
{
  return {className(), "IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportImageStack::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "The origin of the 3D volume", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "The spacing of the 3D volume", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "y", "Z"}));
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_ImageTransformChoice_Key, "Optional Slice Operations",
                                                                    "Operation that is performed on each slice. 0=None, 1=Flip about X, 2=Flip about Y", 0, k_SliceOperationChoices));

  params.insertSeparator(Parameters::Separator{"File List"});
  params.insert(
      std::make_unique<GeneratedFileListParameter>(k_InputFileListInfo_Key, "Input File List", "The list of 2D image files to be read in to a 3D volume", GeneratedFileListParameter::ValueType{}));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Cell Data Name", "The name of the created cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageDataArrayPath_Key, "Created Image Data", "The path to the created image data array", "ImageData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImportImageStack::clone() const
{
  return std::make_unique<ITKImportImageStack>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImportImageStack::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                            const std::atomic_bool& shouldCancel) const
{
  auto inputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  auto origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageTransformValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageTransformChoice_Key);

  const DataPath imageDataPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataName);

  if(imageTransformValue != k_NoImageTransform)
  {
    const auto rotateSampleRefFrameFilter = CreateRotateSampleRefFrameFilter();
    if(nullptr == rotateSampleRefFrameFilter)
    {
      return MakePreflightErrorResult(-23500, "ITKImageImageStack requires the use of the RotateSampleRefFrame filter to perform any image manipulation.");
    }
  }

  std::vector<std::string> files = inputFileListInfo.generate();

  if(files.empty())
  {
    return {MakeErrorResult<OutputActions>(-1, "GeneratedFileList must not be empty")};
  }

  // Create a subfilter to read each image, although for preflight we are going to read the first image in the
  // list and hope the rest are correct.
  Arguments imageReaderArgs;
  imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
  imageReaderArgs.insertOrAssign(ITKImageReader::k_CellDataName_Key, std::make_any<std::string>(cellDataName));
  imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(imageDataPath));
  imageReaderArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(files.at(0)));

  const ITKImageReader imageReader;
  PreflightResult imageReaderResult = imageReader.preflight(dataStructure, imageReaderArgs, messageHandler);
  if(imageReaderResult.outputActions.invalid())
  {
    return imageReaderResult;
  }

  // The first output actions should be the geometry creation
  // A better solution might be to extract the preflight code into a common function for both filters
  const IDataAction* action0Ptr = imageReaderResult.outputActions.value().actions.at(0).get();
  const auto* createImageGeomActionPtr = dynamic_cast<const CreateImageGeometryAction*>(action0Ptr);
  if(createImageGeomActionPtr == nullptr)
  {
    throw std::runtime_error("ITKImportImageStack: Expected CreateImageGeometryAction at index 0");
  }

  // The second action should be the array creation
  const IDataAction* action1Ptr = imageReaderResult.outputActions.value().actions.at(1).get();
  const auto* createArrayActionPtr = dynamic_cast<const CreateArrayAction*>(action1Ptr);
  if(createArrayActionPtr == nullptr)
  {
    throw std::runtime_error("ITKImportImageStack: Expected CreateArrayAction at index 1");
  }

  // X Y Z
  auto dims = createImageGeomActionPtr->dims();
  dims.back() = files.size();

  // Z Y X
  const std::vector<usize> arrayDims(dims.crbegin(), dims.crend());

  OutputActions outputActions;
  outputActions.appendAction(std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(dims), std::move(origin), std::move(spacing), cellDataName));
  outputActions.appendAction(std::make_unique<CreateArrayAction>(createArrayActionPtr->type(), arrayDims, createArrayActionPtr->componentDims(), imageDataPath));

  return {std::move(outputActions)};
}

//------------------------------------------------------------------------------
Result<> ITKImportImageStack::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                          const std::atomic_bool& shouldCancel) const
{
  auto inputFileListInfo = filterArgs.value<GeneratedFileListParameter::ValueType>(k_InputFileListInfo_Key);
  auto origin = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat32Parameter::ValueType>(k_Spacing_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageTransformValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageTransformChoice_Key);

  const DataPath imageDataPath = imageGeomPath.createChildPath(cellDataName).createChildPath(imageDataName);

  std::vector<std::string> files = inputFileListInfo.generate();

  const std::string& firstFile = files.at(0);

  const itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(firstFile.c_str(), itk::ImageIOFactory::IOFileModeEnum::ReadMode);
  imageIO->SetFileName(firstFile.c_str());
  imageIO->ReadImageInformation();

  const itk::ImageIOBase::IOComponentEnum component = imageIO->GetComponentType();

  std::optional<NumericType> numericType = ITK::ConvertIOComponentToNumericType(component);
  if(!numericType.has_value())
  {
    return MakeErrorResult(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
  }
  Result<> readResult;
  switch(*numericType)
  {
  case NumericType::uint8: {
    readResult = cxITKImportImageStack::ReadImageStack<uint8>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::int8: {
    readResult = cxITKImportImageStack::ReadImageStack<int8>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::uint16: {
    readResult = cxITKImportImageStack::ReadImageStack<uint16>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::int16: {
    readResult = cxITKImportImageStack::ReadImageStack<int16>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::uint32: {
    readResult = cxITKImportImageStack::ReadImageStack<uint32>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::int32: {
    readResult = cxITKImportImageStack::ReadImageStack<int32>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::uint64: {
    readResult = cxITKImportImageStack::ReadImageStack<uint64>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::int64: {
    readResult = cxITKImportImageStack::ReadImageStack<int64>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::float32: {
    readResult = cxITKImportImageStack::ReadImageStack<float32>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  case NumericType::float64: {
    readResult = cxITKImportImageStack::ReadImageStack<float64>(dataStructure, imageGeomPath, cellDataName, imageDataPath, files, imageTransformValue, messageHandler, shouldCancel);
    break;
  }
  default: {
    throw std::runtime_error("Unsupported array type");
  }
  }

  return readResult;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_InputFileListInfoKey = "InputFileListInfo";
constexpr StringLiteral k_OriginKey = "Origin";
constexpr StringLiteral k_SpacingKey = "Spacing";
constexpr StringLiteral k_ImageTransformChoiceKey = "ImageTransformChoice";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_ImageDataArrayNameKey = "ImageDataArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKImportImageStack::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKImportImageStack().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FileListInfoFilterParameterConverter>(args, json, SIMPL::k_InputFileListInfoKey, k_InputFileListInfo_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_OriginKey, k_Origin_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::FloatVec3FilterParameterConverter>(args, json, SIMPL::k_SpacingKey, k_Spacing_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_ImageTransformChoiceKey, k_ImageTransformChoice_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellDataName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ImageDataArrayNameKey, k_ImageDataArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
