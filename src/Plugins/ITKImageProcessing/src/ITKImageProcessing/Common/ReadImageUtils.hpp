#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include <itkImageFileReader.h>

using namespace nx::core;

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
    itk::ImageIOBase::Pointer imageIO = itk::ImageIOFactory::CreateImageIO(fileName.c_str(), itk::CommonEnums::IOFileMode::ReadMode);
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

struct ImageReaderOptions
{
  bool OverrideOrigin = false;
  bool OriginAtCenterOfGeometry = false;
  bool OverrideSpacing = false;
  FloatVec3 Origin;
  FloatVec3 Spacing;
};

//------------------------------------------------------------------------------
/**
 * @brief
 * @param fileName
 * @param imageGeomPath
 * @param cellDataName
 * @param arrayName
 * @return
 */
Result<OutputActions> ReadImagePreflight(const std::string& fileName, DataPath imageGeomPath, const std::string& cellDataName, const std::string& arrayName,
                                         const ImageReaderOptions& imageReaderOptions);

} // namespace cxItkImageReader
