#include "ITKImageReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

#include <filesystem>

#include <itkImageFileReader.h>

namespace fs = std::filesystem;

using namespace complex;

namespace
{
//------------------------------------------------------------------------------
std::optional<NumericType> ConvertComponentToNumericType(itk::ImageIOBase::IOComponentEnum component) noexcept
{
  using ComponentType = itk::ImageIOBase::IOComponentEnum;

  switch(component)
  {
  case ComponentType::UCHAR:
    return NumericType::uint8;
  case ComponentType::CHAR:
    return NumericType::int8;
  case ComponentType::USHORT:
    return NumericType::uint16;
  case ComponentType::SHORT:
    return NumericType::int16;
  case ComponentType::UINT:
    return NumericType::uint32;
  case ComponentType::INT:
    return NumericType::int32;
  case ComponentType::ULONG:
    return NumericType::uint64;
  case ComponentType::LONG:
    return NumericType::int64;
  case ComponentType::FLOAT:
    return NumericType::float32;
  case ComponentType::DOUBLE:
    return NumericType::float64;
  default:
    return {};
  }
}

//------------------------------------------------------------------------------
template <class T>
void ReadImageIntoDataArray(DataStructure& dataStructure, const DataPath& arrayPath, itk::ImageIOBase& imageIO)
{
  auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(arrayPath);
  auto& dataStore = dataArray.template getIDataStoreRefAs<DataStore<T>>();
  auto imageSize = imageIO.GetImageSizeInBytes();
  usize arraySize = dataStore.getSize() * sizeof(T);
  if(arraySize != imageSize)
  {
    throw std::runtime_error("ITKImageReader: Image size not equal to array size");
  }
  imageIO.Read(dataStore.data());
}

//------------------------------------------------------------------------------
Result<> ReadImageExecute(const std::string& fileName, const DataPath& imageGeomPath, const DataPath& arrayPath, DataStructure& dataStructure)
{
  try
  {
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(fileName.c_str(), itk::ImageIOFactory::ReadMode);
    if(imageIO == nullptr)
    {
      return MakeErrorResult(-5, fmt::format("ITK could not read the given file \"%1\". Format is likely unsupported.", fileName));
    }

    imageIO->SetFileName(fileName);
    imageIO->ReadImageInformation();

    itk::ImageIOBase::IOComponentEnum component = imageIO->GetComponentType();

    std::optional<NumericType> numericType = ConvertComponentToNumericType(component);
    if(!numericType.has_value())
    {
      return MakeErrorResult(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
    }

    switch(*numericType)
    {
    case NumericType::uint8: {
      ReadImageIntoDataArray<uint8>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::int8: {
      ReadImageIntoDataArray<int8>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::uint16: {
      ReadImageIntoDataArray<uint16>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::int16: {
      ReadImageIntoDataArray<int16>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::uint32: {
      ReadImageIntoDataArray<uint32>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::int32: {
      ReadImageIntoDataArray<int32>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::uint64: {
      ReadImageIntoDataArray<uint64>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::int64: {
      ReadImageIntoDataArray<int64>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::float32: {
      ReadImageIntoDataArray<float32>(dataStructure, arrayPath, *imageIO);
      break;
    }
    case NumericType::float64: {
      ReadImageIntoDataArray<float64>(dataStructure, arrayPath, *imageIO);
      break;
    }
    }

  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult(-55557, fmt::format("ITK exception was thrown while processing input file: {}", err.what()));
  }

  return {};
}

//------------------------------------------------------------------------------
Result<OutputActions> ReadImagePreflight(const std::string& fileName, DataPath imageGeomPath, DataPath arrayPath)
{
  OutputActions actions;

  try
  {
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(fileName.c_str(), itk::ImageIOFactory::ReadMode);
    if(imageIO == nullptr)
    {
      return MakeErrorResult<OutputActions>(-5, fmt::format("ITK could not read the given file \"{}\". Format is likely unsupported.", fileName));
    }

    imageIO->SetFileName(fileName);
    imageIO->ReadImageInformation();

    itk::ImageIOBase::IOComponentEnum component = imageIO->GetComponentType();

    std::optional<NumericType> numericType = ConvertComponentToNumericType(component);
    if(!numericType.has_value())
    {
      return MakeErrorResult<OutputActions>(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
    }

    uint32 nDims = imageIO->GetNumberOfDimensions();

    std::vector<usize> dims = {1, 1, 1};
    std::vector<float32> origin = {0.0f, 0.0f, 0.0f};
    std::vector<float32> spacing = {1.0f, 1.0f, 1.0f};

    for(uint32 i = 0; i < nDims; i++)
    {
      dims[i] = static_cast<usize>(imageIO->GetDimensions(i));
      origin[i] = static_cast<float32>(imageIO->GetOrigin(i));
      spacing[i] = static_cast<float32>(imageIO->GetSpacing(i));
    }

    uint32 nComponents = imageIO->GetNumberOfComponents();

    // DataArray dimensions are stored slowest to fastest, the opposite of ImageGeometry
    std::vector<usize> arrayDims(dims.crbegin(), dims.crend());

    std::vector<usize> cDims = {nComponents};

    actions.actions.push_back(std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(dims), std::move(origin), std::move(spacing)));
    actions.actions.push_back(std::make_unique<CreateArrayAction>(*numericType, std::move(arrayDims), std::move(cDims), std::move(arrayPath)));
  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult<OutputActions>(-55557, fmt::format("ITK exception was thrown while processing input file: {}", err.what()));
  }

  return {std::move(actions)};
}

} // namespace

namespace complex
{
//------------------------------------------------------------------------------
std::string ITKImageReader::name() const
{
  return FilterTraits<ITKImageReader>::name;
}

//------------------------------------------------------------------------------
std::string ITKImageReader::className() const
{
  return FilterTraits<ITKImageReader>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImageReader::uuid() const
{
  return FilterTraits<ITKImageReader>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImageReader::humanName() const
{
  return "ITK::Image Reader";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageReader::defaultTags() const
{
  return {"io", "input", "read", "import"};
}

//------------------------------------------------------------------------------
Parameters ITKImageReader::parameters() const
{
  Parameters params;
  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<FileSystemPathParameter>(k_FileName_Key, "File", "Input image file", fs::path(""), FileSystemPathParameter::PathType::InputFile));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Image Geometry", "", DataPath{}));
  params.insertSeparator(Parameters::Separator{"Cell Data"});
  params.insert(std::make_unique<ArrayCreationParameter>(k_ImageDataArrayPath_Key, "Image Data", "", DataPath{}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageReader::clone() const
{
  return std::make_unique<ITKImageReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageReader::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler) const
{
  auto fileName = filterArgs.value<fs::path>(k_FileName_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  std::string fileNameString = fileName.string();

  return {ReadImagePreflight(fileNameString, imageGeometryPath, imageDataArrayPath)};
}

//------------------------------------------------------------------------------
Result<> ITKImageReader::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler) const
{
  auto fileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  std::string fileNameString = fileName.string();

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  imageGeom.getLinkedGeometryData().addCellData(imageDataArrayPath);

  return ReadImageExecute(fileNameString, imageGeometryPath, imageDataArrayPath, dataStructure);
}
} // namespace complex
