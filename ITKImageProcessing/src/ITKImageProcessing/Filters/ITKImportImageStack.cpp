#include "ITKImportImageStack.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Filters/ITKImageReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

#include <itkImageFileReader.h>
#include <itkImageIOBase.h>

#include <filesystem>

namespace fs = std::filesystem;

using namespace complex;

namespace
{
template <class T>
Result<> ReadImageStack(DataStructure& dataStructure, const DataPath& imageGeomPath, const DataPath& imageDataPath, const std::vector<std::string>& files,
                        const IFilter::MessageHandler& messageHandler)
{
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  SizeVec3 dims = imageGeom.getDimensions();
  usize tuplesPerSlice = dims[0] * dims[1];

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

    // Create a subfilter to read each image, although for preflight we are going to read the first image in the
    // list and hope the rest are correct.
    ITKImageReader imageReader;

    Arguments args;
    args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
    args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(imageDataPath));
    args.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(filePath));

    auto executeResult = imageReader.execute(importedDataStructure, args);
    if(executeResult.result.invalid())
    {
      return executeResult.result;
    }

    // Check the ImageGeometry of the imported Image matches the destination
    const auto& importedImageGeom = importedDataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
    SizeVec3 importedDims = importedImageGeom.getDimensions();
    if(dims[0] != importedDims[0] || dims[1] != importedDims[1])
    {
      return MakeErrorResult(-64510, fmt::format("Slice {} image dimensions are different than the first slice.\n  First Slice Dims are:  {} x {}\n  Current Slice Dims are:{} x {}\n"));
    }

    // Compute the Tuple Index we are at:
    usize tupleIndex = (slice * dims[0] * dims[1]);
    // get the current Slice data...
    const auto& tempData = importedDataStructure.getDataRefAs<DataArray<T>>(imageDataPath);
    const auto& tempDataStore = tempData.getDataStoreRef();

    // Copy that into the output array...
    if(!outputDataStore.copyFrom(tupleIndex, tempDataStore, 0, tuplesPerSlice))
    {
      return MakeErrorResult(-64511, fmt::format("Error copying source image data into destination array.    Slice:{}    TupleIndex:{}    MaxTupleIndex:{}", slice, tupleIndex, outputData.getSize()));
    }

    slice++;

    // Check to see if the filter got canceled.
    // if(filter->getCancel())
    //{
    //  return;
    //}
  }

  return {};
}
} // namespace

namespace complex
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
  return "ITK::Import Images (3D Stack)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImportImageStack::defaultTags() const
{
  return {"IO", "Input", "Read", "Import"};
}

//------------------------------------------------------------------------------
Parameters ITKImportImageStack::parameters() const
{
  Parameters params;

  params.insert(std::make_unique<GeneratedFileListParameter>(k_InputFileListInfo_Key, "Input File List", "", GeneratedFileListParameter::ValueType{}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Origin_Key, "Origin", "", std::vector<float32>{0.0F, 0.0F, 0.0F}, std::vector<std::string>{"X", "y", "Z"}));
  params.insert(std::make_unique<VectorFloat32Parameter>(k_Spacing_Key, "Spacing", "", std::vector<float32>{1.0F, 1.0F, 1.0F}, std::vector<std::string>{"X", "y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Created Data Structure Items"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ImageDataArrayPath_Key, "Created Image Data", "", DataPath({"ImageDataContainer", "ImageData"})));

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
  auto imageDataPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  std::vector<std::string> files = inputFileListInfo.generate();

  if(files.empty())
  {
    return {MakeErrorResult<OutputActions>(-1, "GeneratedFileList must not be empty")};
  }

  // Create a subfilter to read each image, although for preflight we are going to read the first image in the
  // list and hope the rest are correct.
  Arguments imageReaderArgs;
  imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, std::make_any<DataPath>(imageGeomPath));
  imageReaderArgs.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, std::make_any<DataPath>(imageDataPath));
  imageReaderArgs.insertOrAssign(ITKImageReader::k_FileName_Key, std::make_any<fs::path>(files.at(0)));

  ITKImageReader imageReader;
  PreflightResult imageReaderResult = imageReader.preflight(dataStructure, imageReaderArgs, messageHandler);
  if(imageReaderResult.outputActions.invalid())
  {
    return imageReaderResult;
  }

  // The first output actions should be the geometry creation
  // A better solution might be to extract the preflight code into a common function for both filters
  const IDataAction* action0 = imageReaderResult.outputActions.value().actions.at(0).get();
  const auto* createImageGeomAction = dynamic_cast<const CreateImageGeometryAction*>(action0);
  if(createImageGeomAction == nullptr)
  {
    throw std::runtime_error("ITKImportImageStack: Expected CreateImageGeometryAction at index 0");
  }

  // The third action should be the array creation
  const IDataAction* action1 = imageReaderResult.outputActions.value().actions.at(2).get();
  const auto* createArrayAction = dynamic_cast<const CreateArrayAction*>(action1);
  if(createArrayAction == nullptr)
  {
    throw std::runtime_error("ITKImportImageStack: Expected CreateArrayAction at index 2");
  }

  // X Y Z
  auto dims = createImageGeomAction->dims();
  dims.back() = files.size();

  // Z Y X
  std::vector<usize> arrayDims(dims.crbegin(), dims.crend());

  OutputActions outputActions;
  outputActions.actions.push_back(std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(dims), std::move(origin), std::move(spacing)));
  outputActions.actions.push_back(std::make_unique<CreateArrayAction>(createArrayAction->type(), arrayDims, createArrayAction->componentDims(), imageDataPath));

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
  auto imageDataPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  std::vector<std::string> files = inputFileListInfo.generate();

  const std::string& firstFile = files.at(0);

  itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(firstFile.c_str(), itk::ImageIOFactory::IOFileModeEnum::ReadMode);
  imageIO->SetFileName(firstFile.c_str());
  imageIO->ReadImageInformation();

  itk::ImageIOBase::IOComponentEnum component = imageIO->GetComponentType();

  std::optional<NumericType> numericType = ITK::ConvertIOComponentToNumericType(component);
  if(!numericType.has_value())
  {
    return MakeErrorResult(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
  }

  switch(*numericType)
  {
  case NumericType::uint8: {
    return ReadImageStack<uint8>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::int8: {
    return ReadImageStack<int8>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::uint16: {
    return ReadImageStack<uint16>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::int16: {
    return ReadImageStack<int16>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::uint32: {
    return ReadImageStack<uint32>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::int32: {
    return ReadImageStack<int32>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::uint64: {
    return ReadImageStack<uint64>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::int64: {
    return ReadImageStack<int64>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::float32: {
    return ReadImageStack<float32>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  case NumericType::float64: {
    return ReadImageStack<float64>(dataStructure, imageGeomPath, imageDataPath, files, messageHandler);
  }
  default: {
    throw std::runtime_error("Unsupported array type");
  }
  }

  return {};
}
} // namespace complex
