#include "ITKImageReader.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Actions/CreateImageGeometryAction.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/DataGroupCreationParameter.hpp"
#include "complex/Parameters/DataObjectNameParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include <filesystem>

#include <itkImageFileReader.h>

namespace fs = std::filesystem;

using namespace complex;

namespace cxItkImageReader
{
// This functor is a dummy that will return a valid Result<> if the ImageIOBase is a supported type, dimension, etc.
struct PreflightFunctor
{
  //------------------------------------------------------------------------------
  template <class PixelT, uint32 Dimension>
  Result<> operator()() const
  {
    return {};
  }
};

struct ReadImageIntoArrayFunctor
{
  //------------------------------------------------------------------------------
  template <class PixelT, uint32 Dimension>
  Result<> operator()(DataStructure& dataStructure, const DataPath& arrayPath, const std::string& filePath) const
  {
    using ImageType = itk::Image<PixelT, Dimension>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    using T = ITK::UnderlyingType_t<PixelT>;

    auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(arrayPath);
    auto& dataStore = dataArray.template getIDataStoreRefAs<DataStore<T>>();

    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(filePath);

    reader->Update();
    typename ImageType::Pointer outputImage = reader->GetOutput();

    auto imageStore = ITK::ConvertImageToDataStore(*outputImage);

    dataStore = std::move(imageStore);

    return {};
  }
};

//------------------------------------------------------------------------------
template <class T, usize Dimension, class FunctorT, class... ArgsT>
Result<> ReadImageByPixelType(const itk::ImageIOBase& imageIO, ArgsT&&... args)
{
  const uint32 numComponents = imageIO.GetNumberOfComponents();

  switch(numComponents)
  {
  case 1: {
    return FunctorT().template operator()<itk::Vector<T, 1>, Dimension>(std::forward<ArgsT>(args)...);
  }
  case 2: {
    return FunctorT().template operator()<itk::Vector<T, 2>, Dimension>(std::forward<ArgsT>(args)...);
  }
  case 3: {
    return FunctorT().template operator()<itk::Vector<T, 3>, Dimension>(std::forward<ArgsT>(args)...);
  }
  case 4: {
    return FunctorT().template operator()<itk::Vector<T, 4>, Dimension>(std::forward<ArgsT>(args)...);
  }
  case 36: {
    return FunctorT().template operator()<itk::Vector<T, 36>, Dimension>(std::forward<ArgsT>(args)...);
  }
  default: {
    return MakeErrorResult(-4, fmt::format("Unsupported number of components: {} in image file. 1,2,3,4,36 are the only supported number of components", numComponents));
  }
  }
}

//------------------------------------------------------------------------------
template <class T, class FunctorT, class... ArgsT>
Result<> ReadImageByDimension(const itk::ImageIOBase& imageIO, ArgsT&&... args)
{
  uint32 dimensions = imageIO.GetNumberOfDimensions();
  switch(dimensions)
  {
  case 1: {
    return ReadImageByPixelType<T, 1, FunctorT>(imageIO, args...);
  }
  case 2: {
    return ReadImageByPixelType<T, 2, FunctorT>(imageIO, args...);
  }
  case 3: {
    return ReadImageByPixelType<T, 3, FunctorT>(imageIO, args...);
  }
  default: {
    return MakeErrorResult(-1, fmt::format("Unsupported number of dimensions: {}", dimensions));
  }
  }
}

//------------------------------------------------------------------------------
template <class FunctorT, class... ArgsT>
Result<> ReadImageExecute(const std::string& fileName, ArgsT&&... args)
{
  try
  {
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(fileName.c_str(), itk::ImageIOFactory::ReadMode);
    if(imageIO == nullptr)
    {
      return MakeErrorResult(-5, fmt::format("ITK could not read the given file \"{}\". Format is likely unsupported.", fileName));
    }

    imageIO->SetFileName(fileName);
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
      return ReadImageByDimension<uint8, FunctorT>(*imageIO, args...);
    }
    case NumericType::int8: {
      return ReadImageByDimension<int8, FunctorT>(*imageIO, args...);
    }
    case NumericType::uint16: {
      return ReadImageByDimension<uint16, FunctorT>(*imageIO, args...);
    }
    case NumericType::int16: {
      return ReadImageByDimension<int16, FunctorT>(*imageIO, args...);
    }
    case NumericType::uint32: {
      return ReadImageByDimension<uint32, FunctorT>(*imageIO, args...);
    }
    case NumericType::int32: {
      return ReadImageByDimension<int32, FunctorT>(*imageIO, args...);
    }
    case NumericType::uint64: {
      return ReadImageByDimension<uint64, FunctorT>(*imageIO, args...);
    }
    case NumericType::int64: {
      return ReadImageByDimension<int64, FunctorT>(*imageIO, args...);
    }
    case NumericType::float32: {
      return ReadImageByDimension<float32, FunctorT>(*imageIO, args...);
    }
    case NumericType::float64: {
      return ReadImageByDimension<float64, FunctorT>(*imageIO, args...);
    }
    default: {
      throw std::runtime_error("");
    }
    }
  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult(-55557, fmt::format("ITK exception was thrown while processing input file: {}", err.what()));
  }
}

//------------------------------------------------------------------------------
Result<OutputActions> ReadImagePreflight(const std::string& fileName, DataPath imageGeomPath, std::string cellDataName, DataPath arrayPath)
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

    std::optional<DataType> numericType = ITK::ConvertIOComponentToDataType(component);
    if(!numericType.has_value())
    {
      return MakeErrorResult<OutputActions>(-4, fmt::format("Unsupported pixel component: {}", imageIO->GetComponentTypeAsString(component)));
    }

    uint32 nDims = imageIO->GetNumberOfDimensions();

    std::vector<size_t> dims = {1, 1, 1};
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

    actions.appendAction(std::make_unique<CreateImageGeometryAction>(std::move(imageGeomPath), std::move(dims), std::move(origin), std::move(spacing), cellDataName));
    actions.appendAction(std::make_unique<CreateArrayAction>(*numericType, std::move(arrayDims), std::move(cDims), std::move(arrayPath)));

  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult<OutputActions>(-55557, fmt::format("ITK exception was thrown while processing input file: {}", err.what()));
  }

  return {std::move(actions)};
}

} // namespace cxItkImageReader

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
  return "Read Image (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageReader::defaultTags() const
{
  return {className(), "io", "input", "read", "import"};
}

//------------------------------------------------------------------------------
Parameters ITKImageReader::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<FileSystemPathParameter>(k_FileName_Key, "File", "Input image file", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".png"}, {".tiff"}, {".tif"}, {".bmp"}, {".jpeg"}, {".jpg"}, {".nrrd"}, {".mha"}},
                                                          FileSystemPathParameter::PathType::InputFile, false));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Cell Data Name", "The name of the created cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ImageDataArrayPath_Key, "Created Image Data", "The path to the created image data array",
                                                         DataPath({"ImageDataContainer", ImageGeom::k_CellDataName, "ImageData"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageReader::clone() const
{
  return std::make_unique<ITKImageReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageReader::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto fileName = filterArgs.value<fs::path>(k_FileName_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<std::string>(k_CellDataName_Key);
  auto imageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  std::string fileNameString = fileName.string();

  Result<> check = cxItkImageReader::ReadImageExecute<cxItkImageReader::PreflightFunctor>(fileNameString);
  if(check.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(check), {})};
  }

  return {cxItkImageReader::ReadImagePreflight(fileNameString, imageGeometryPath, cellDataName, imageDataArrayPath)};
}

//------------------------------------------------------------------------------
Result<> ITKImageReader::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  auto fileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(imageDataArrayPath);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageDataArrayPath.toString()));
  }

  std::string fileNameString = fileName.string();

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  imageGeom.getLinkedGeometryData().addCellData(imageDataArrayPath);

  return cxItkImageReader::ReadImageExecute<cxItkImageReader::ReadImageIntoArrayFunctor>(fileNameString, dataStructure, imageDataArrayPath, fileNameString);
}
} // namespace complex
