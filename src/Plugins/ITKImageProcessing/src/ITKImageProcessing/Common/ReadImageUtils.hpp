#pragma once

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Parameters/CropGeometryParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include <itkImageFileReader.h>
#include <itkRegionOfInterestImageFilter.h>

using namespace nx::core;

namespace cxItkImageReaderFilter
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
  template <class ImageType>
  Result<typename ImageType::Pointer> CropImage(typename ImageType::Pointer inputImage, const CropGeometryParameter::ValueType& croppingOptions, const std::optional<std::vector<float64>>& spacing,
                                                const std::optional<std::vector<float64>>& origin) const
  {
    using IndexType = typename ImageType::IndexType;
    using PointType = typename ImageType::PointType;
    using SpacingType = typename ImageType::SpacingType;
    using PointValueType = typename PointType::ValueType;
    using SpacingValueType = typename SpacingType::ValueType;

    constexpr unsigned int Dimension = ImageType::ImageDimension;

    std::vector<bool> cropFlags = {croppingOptions.cropX, croppingOptions.cropY, croppingOptions.cropZ};

    const auto& region = inputImage->GetLargestPossibleRegion();
    const auto& size = region.GetSize();

    // Make local, modifiable copies of origin/spacing
    PointType imageOrigin = inputImage->GetOrigin();
    SpacingType imageSpacing = inputImage->GetSpacing();

    // Override origin from std::vector<double>, if provided
    if(origin.has_value())
    {
      const auto& originVec = origin.value();
      for(unsigned int d = 0; d < Dimension && d < originVec.size(); ++d)
      {
        imageOrigin[d] = static_cast<PointValueType>(originVec[d]);
      }
    }

    // Override spacing from std::vector<double>, if provided
    if(spacing.has_value())
    {
      const auto& spacingVec = spacing.value();
      for(unsigned int d = 0; d < Dimension && d < spacingVec.size(); ++d)
      {
        imageSpacing[d] = static_cast<SpacingValueType>(spacingVec[d]);
      }
    }

    switch(croppingOptions.type)
    {
    case CropGeometryParameter::CropValues::TypeEnum::VoxelSubvolume: {
      IndexType minIdx;
      IndexType maxIdx;
      for(unsigned int d = 0; d < Dimension; ++d)
      {
        switch(d)
        {
        case 0:
          minIdx[d] = cropFlags[d] ? static_cast<typename IndexType::IndexValueType>(croppingOptions.xBoundVoxels[0]) : static_cast<typename IndexType::IndexValueType>(0);
          maxIdx[d] = cropFlags[d] ? static_cast<typename IndexType::IndexValueType>(croppingOptions.xBoundVoxels[1]) : static_cast<typename IndexType::IndexValueType>(size[d] - 1);
          break;
        case 1:
          minIdx[d] = cropFlags[d] ? static_cast<typename IndexType::IndexValueType>(croppingOptions.yBoundVoxels[0]) : static_cast<typename IndexType::IndexValueType>(0);
          maxIdx[d] = cropFlags[d] ? static_cast<typename IndexType::IndexValueType>(croppingOptions.yBoundVoxels[1]) : static_cast<typename IndexType::IndexValueType>(size[d] - 1);
          break;
        case 2:
          minIdx[d] = cropFlags[d] ? static_cast<typename IndexType::IndexValueType>(croppingOptions.zBoundVoxels[0]) : static_cast<typename IndexType::IndexValueType>(0);
          maxIdx[d] = cropFlags[d] ? static_cast<typename IndexType::IndexValueType>(croppingOptions.zBoundVoxels[1]) : static_cast<typename IndexType::IndexValueType>(size[d] - 1);
          break;
        default:
          break;
        }
      }
      return CropImageByVoxelBounds<ImageType>(inputImage, minIdx, maxIdx);
    }
    case CropGeometryParameter::CropValues::TypeEnum::PhysicalSubvolume: {
      PointType minPt;
      PointType maxPt;
      for(unsigned int d = 0; d < Dimension; ++d)
      {
        switch(d)
        {
        case 0:
          minPt[d] = cropFlags[d] ? static_cast<PointValueType>(croppingOptions.xBoundPhysical[0]) : static_cast<PointValueType>(imageOrigin[d]);
          maxPt[d] = cropFlags[d] ? static_cast<PointValueType>(croppingOptions.xBoundPhysical[1]) : static_cast<PointValueType>(imageSpacing[d] * static_cast<float64>(size[d]) + imageOrigin[d]);
          break;
        case 1:
          minPt[d] = cropFlags[d] ? static_cast<PointValueType>(croppingOptions.yBoundPhysical[0]) : static_cast<PointValueType>(imageOrigin[d]);
          maxPt[d] = cropFlags[d] ? static_cast<PointValueType>(croppingOptions.yBoundPhysical[1]) : static_cast<PointValueType>(imageSpacing[d] * static_cast<float64>(size[d]) + imageOrigin[d]);
          break;
        case 2:
          minPt[d] = cropFlags[d] ? static_cast<PointValueType>(croppingOptions.zBoundPhysical[0]) : static_cast<PointValueType>(imageOrigin[d]);
          maxPt[d] = cropFlags[d] ? static_cast<PointValueType>(croppingOptions.zBoundPhysical[1]) : static_cast<PointValueType>(imageSpacing[d] * static_cast<float64>(size[d]) + imageOrigin[d]);
          break;
        default:
          break;
        }
      }
      return CropImageByPhysicalBounds<ImageType>(inputImage, minPt, maxPt, imageSpacing, imageOrigin);
    }
    case CropGeometryParameter::CropValues::TypeEnum::NoCropping: {
      // Do nothing
      return {inputImage};
    }
    }

    // Fallback to original image if an unknown cropping type is encountered
    return {inputImage};
  }

  //------------------------------------------------------------------------------
  template <class ImageType>
  Result<typename ImageType::Pointer> CropImageByVoxelBounds(typename ImageType::Pointer inputImage, const typename ImageType::IndexType& minIndex, const typename ImageType::IndexType& maxIndex) const
  {
    using SizeType = typename ImageType::SizeType;
    using RegionType = typename ImageType::RegionType;

    constexpr unsigned int Dimension = ImageType::ImageDimension;

    // Compute size = max - min + 1 in each dimension
    SizeType size;
    for(unsigned int d = 0; d < Dimension; ++d)
    {
      if(maxIndex[d] < minIndex[d])
      {
        return MakeErrorResult<typename ImageType::Pointer>(-3001, fmt::format("CropImageByVoxelBounds: maxIndex[{}] < minIndex[{}]", d, d));
      }
      size[d] = static_cast<typename SizeType::SizeValueType>(maxIndex[d] - minIndex[d] + 1);
    }

    RegionType region;
    region.SetIndex(minIndex);
    region.SetSize(size);

    using ROIFilterType = itk::RegionOfInterestImageFilter<ImageType, ImageType>;
    auto roi = ROIFilterType::New();
    roi->SetInput(inputImage);
    roi->SetRegionOfInterest(region);
    roi->Update();

    return {roi->GetOutput()};
  }

  //------------------------------------------------------------------------------
  template <class ImageType>
  Result<typename ImageType::Pointer> CropImageByPhysicalBounds(typename ImageType::Pointer inputImage, const typename ImageType::PointType& minPoint,
                                                                const typename ImageType::PointType& maxPoint, const typename ImageType::SpacingType& spacing, const typename ImageType::PointType& origin) const
  {
    using IndexType = typename ImageType::IndexType;

    constexpr unsigned int Dimension = ImageType::ImageDimension;

    //    IndexType minIndex;
    //    IndexType maxIndex;

    //    bool insideMin = inputImage->TransformPhysicalPointToIndex(minPoint, minIndex);
    //    bool insideMax = inputImage->TransformPhysicalPointToIndex(maxPoint, maxIndex);
    //
    //    if(!insideMin || !insideMax)
    //    {
    //      return MakeErrorResult<typename ImageType::Pointer>(-3002, "CropImageByPhysicalBounds: physical bounds are outside image extent");
    //    }

    using IndexType = typename ImageType::IndexType;

    const auto& region = inputImage->GetLargestPossibleRegion();
    const auto& size = region.GetSize();

    IndexType minIndex{};
    IndexType maxIndex{};

    for(usize d = 0; d < Dimension; ++d)
    {
      const float64 minPhys = minPoint[d];
      const float64 maxPhys = maxPoint[d];

      const float64 minIdxCont = (minPhys - origin[d]) / spacing[d];
      const float64 maxIdxCont = (maxPhys - origin[d]) / spacing[d];

      auto minI = static_cast<long>(std::floor(minIdxCont));
      auto maxI = static_cast<long>(std::ceil(maxIdxCont));

      minI = std::max<long>(0, minI);
      maxI = std::min<long>(static_cast<long>(size[d]) - 1, maxI);

      if(maxI < minI)
      {
        return MakeErrorResult<typename ImageType::Pointer>(-4001, "CropImageByPhysicalBounds: physical bounds are outside image extent");
      }

      minIndex[d] = minI;
      maxIndex[d] = maxI;
    }

    // Ensure minIndex <= maxIndex in each dimension
    for(unsigned int d = 0; d < Dimension; ++d)
    {
      if(minIndex[d] > maxIndex[d])
      {
        std::swap(minIndex[d], maxIndex[d]);
      }
    }

    return CropImageByVoxelBounds<ImageType>(inputImage, minIndex, maxIndex);
  }

  //------------------------------------------------------------------------------
  template <class PixelT, uint32 Dimension>
  Result<> operator()(DataStructure& dataStructure, const DataPath& arrayPath, const std::string& filePath, const CropGeometryParameter::ValueType& croppingOptions,
                      const std::optional<std::vector<float64>>& spacing, const std::optional<std::vector<float64>>& origin) const
  {
    using ImageType = itk::Image<PixelT, Dimension>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    using T = ITK::UnderlyingType_t<PixelT>;

    auto& dataArray = dataStructure.getDataRefAs<DataArray<T>>(arrayPath);
    auto& dataStore = dataArray.template getIDataStoreRefAs<AbstractDataStore<T>>();

    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(filePath);

    reader->Update();
    typename ImageType::Pointer outputImage = reader->GetOutput();
    Result<typename ImageType::Pointer> result = CropImage<ImageType>(outputImage, croppingOptions, spacing, origin);
    if(result.invalid())
    {
      return ConvertResult(std::move(result));
    }
    outputImage = result.value();

    ITK::ConvertImageToDataStore(*outputImage, dataStore);

    return {};
  }

  //------------------------------------------------------------------------------
  template <class PixelT, uint32 Dimension>
  Result<> operator()(DataStructure& dataStructure, const std::string& filePath, const DataPath& arrayPath, const DataType& dataType, const CropGeometryParameter::ValueType& croppingOptions,
                      const std::optional<std::vector<float64>>& spacing = {}, const std::optional<std::vector<float64>>& origin = {}) const
  {
    using ImageType = itk::Image<PixelT, Dimension>;
    using ReaderType = itk::ImageFileReader<ImageType>;

    if(!ExecuteNeighborFunction(ITK::detail::TypeConversionValidateFunctor<typename itk::NumericTraits<PixelT>::ValueType>{}, dataType))
    {
      // Not valid for conversion executing overload
      return operator()<PixelT, Dimension>(dataStructure, arrayPath, filePath, croppingOptions, spacing, origin);
    }

    typename ReaderType::Pointer reader = ReaderType::New();
    reader->SetFileName(filePath);

    reader->Update();
    typename ImageType::Pointer outputImage = reader->GetOutput();
    Result<typename ImageType::Pointer> result = CropImage<ImageType>(outputImage, croppingOptions, spacing, origin);
    if(result.invalid())
    {
      return ConvertResult(std::move(result));
    }
    outputImage = result.value();

    return ExecuteNeighborFunction(ITK::ConvertImageToDatastoreFunctor{}, dataType, dataStructure, arrayPath, *outputImage);
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
      throw std::runtime_error(fmt::format("ReadImageExecute::Unknown Numeric Type:'{}'", to_underlying(*numericType)));
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
  bool ChangeDataType = false;
  DataType ImageDataType = DataType::uint8;
  CropGeometryParameter::ValueType CroppingOptions;
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

} // namespace cxItkImageReaderFilter
