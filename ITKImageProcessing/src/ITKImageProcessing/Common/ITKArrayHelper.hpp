#pragma once

#include "complex/Common/Result.hpp"
#include "complex/Common/Types.hpp"
#include "complex/Common/TypesUtility.hpp"
#include "complex/DataStructure/DataStore.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/Geometry/ImageGeom.hpp"
#include "complex/DataStructure/IDataStore.hpp"
#include "complex/Filter/Actions/CreateArrayAction.hpp"
#include "complex/Filter/Output.hpp"

#include <itkImage.h>
#include <itkImportImageFilter.h>
#include <itkNumericTraits.h>
#include <itkVector.h>

#include <fmt/core.h>

#include <stdexcept>
#include <type_traits>
#include <vector>

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_int8
#define COMPLEX_ITK_ARRAY_HELPER_USE_int8 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_uint8
#define COMPLEX_ITK_ARRAY_HELPER_USE_uint8 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_int16
#define COMPLEX_ITK_ARRAY_HELPER_USE_int16 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_uint16
#define COMPLEX_ITK_ARRAY_HELPER_USE_uint16 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_int32
#define COMPLEX_ITK_ARRAY_HELPER_USE_int32 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_uint32
#define COMPLEX_ITK_ARRAY_HELPER_USE_uint32 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_int64
#define COMPLEX_ITK_ARRAY_HELPER_USE_int64 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_uint64
#define COMPLEX_ITK_ARRAY_HELPER_USE_uint64 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_float32
#define COMPLEX_ITK_ARRAY_HELPER_USE_float32 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_float64
#define COMPLEX_ITK_ARRAY_HELPER_USE_float64 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_Scalar
#define COMPLEX_ITK_ARRAY_HELPER_USE_Scalar 1
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_Vector
#define COMPLEX_ITK_ARRAY_HELPER_USE_Vector 0
#endif

#ifndef COMPLEX_ITK_ARRAY_HELPER_USE_RGB_RGBA
#define COMPLEX_ITK_ARRAY_HELPER_USE_RGB_RGBA 0
#endif

/*
 * The original implementation seemed to short circuit to vector if both were activate, but it's
 * unknown if that was the intended behavior.
 */
static_assert(!(COMPLEX_ITK_ARRAY_HELPER_USE_Vector == 1 && COMPLEX_ITK_ARRAY_HELPER_USE_RGB_RGBA == 1), "ITKArrayHelper: Vector and RGB/RGBA support cannot both be enabled at the same time");

namespace complex
{
namespace ITK
{
/**
 * @brief CastVec3ToITK Input type should be FloatVec3Type or IntVec3Type, Output
   type should be some kind of ITK "array" (itk::Size, itk::Index,...)
 */
template <typename InputType, typename OutputType, typename ComponentType>
OutputType CastVec3ToITK(const InputType& inputVec3, unsigned int dimension)
{
  OutputType output;
  if(dimension > 0)
  {
    output[0] = static_cast<ComponentType>(inputVec3[0]);
    if(dimension > 1)
    {
      output[1] = static_cast<ComponentType>(inputVec3[1]);
      if(dimension > 2)
      {
        output[2] = static_cast<ComponentType>(inputVec3[2]);
      }
    }
  }
  return output;
}

/**
 * @brief
 * @tparam T
 */
template <class T>
using UnderlyingType_t = typename itk::NumericTraits<T>::ValueType;

bool DoDimensionsMatch(const IDataStore& dataStore, const ImageGeom& imageGeom);

template <class PixelT>
std::vector<usize> GetComponentDimensions()
{
  return {itk::NumericTraits<PixelT>::GetLength()};
}

template <class PixelT, uint32 Dimensions>
typename itk::Image<PixelT, Dimensions>::Pointer WrapDataStoreInImage(DataStore<UnderlyingType_t<PixelT>>& dataStore, const ImageGeom& imageGeom)
{
  using T = ITK::UnderlyingType_t<PixelT>;

  static_assert(std::is_standard_layout_v<PixelT>, "complex::ITK::WrapDataStoreInImage: PixelT must be standard layout");
  static_assert(std::is_trivial_v<PixelT>, "complex::ITK::WrapDataStoreInImage: PixelT must be trivial");
  static_assert(std::is_arithmetic_v<T>, "complex::ITK::WrapDataStoreInImage: The underlying type T of PixelT must be arithmetic");
  static_assert(sizeof(PixelT) % sizeof(T) == 0, "complex::ITK::WrapDataStoreInImage: The size of PixelT must be evenly divisible by T");

  using FilterType = itk::ImportImageFilter<PixelT, Dimensions>;

  SizeVec3 geomDims = imageGeom.getDimensions();
  FloatVec3 geomOrigin = imageGeom.getOrigin();
  FloatVec3 geomSpacing = imageGeom.getSpacing();

  typename FilterType::SizeType imageSize{};
  typename FilterType::OriginType imageOrigin{};
  typename FilterType::SpacingType imageSpacing{};

  for(uint32 i = 0; i < Dimensions; i++)
  {
    imageSize[i] = geomDims[i];
    imageSpacing[i] = geomSpacing[i];
    imageOrigin[i] = geomOrigin[i];
  }

  typename FilterType::IndexType imageIndex{};

  typename FilterType::RegionType imageRegion{};

  imageRegion.SetSize(imageSize);
  imageRegion.SetIndex(imageIndex);

  typename FilterType::DirectionType imageDirection = FilterType::DirectionType::GetIdentity();

  auto importFilter = FilterType::New();
  importFilter->SetRegion(imageRegion);
  importFilter->SetOrigin(imageOrigin);
  importFilter->SetSpacing(imageSpacing);
  importFilter->SetDirection(imageDirection);
  importFilter->SetImportPointer(reinterpret_cast<PixelT*>(dataStore.data()), dataStore.getSize(), false);
  importFilter->Update();

  return importFilter->GetOutput();
}

template <class PixelT, uint32 Dimension>
DataStore<UnderlyingType_t<PixelT>> ConvertImageToDataStore(itk::Image<PixelT, Dimension>& image)
{
  using ImageType = itk::Image<PixelT, Dimension>;
  using T = UnderlyingType_t<PixelT>;
  typename ImageType::SizeType imageSize = image.GetLargestPossibleRegion().GetSize();
  std::vector<usize> tDims(imageSize.rbegin(), imageSize.rend());
  std::vector<usize> cDims = GetComponentDimensions<PixelT>();
  typename ImageType::PixelContainer* pixelContainer = image.GetPixelContainer();
  // ITK use the global new allocator
  auto* bufferPtr = pixelContainer->GetBufferPointer();
  pixelContainer->ContainerManageMemoryOff();
  std::unique_ptr<T[]> newData(bufferPtr);
  DataStore<T> dataStore(std::move(newData), std::move(tDims), std::move(cDims));
  return dataStore;
}

namespace detail
{
template <class InputT, class OutputT, usize Dimensions, class ResultT, template <class, class, uint32> class FunctorT, class... ArgsT>
Result<ResultT> ArraySwitchFuncComponentImpl(usize nComp, int32 errorCode, ArgsT&&... args)
{
#if COMPLEX_ITK_ARRAY_HELPER_USE_Scalar == 1
  if(nComp == 1)
  {
    return FunctorT<InputT, OutputT, Dimensions>()(std::forward<ArgsT>(args)...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_Vector == 1
  if(nComp == 2)
  {
    using InputPixelType = itk::Vector<InputT, 2>;
    using OutputPixelType = itk::Vector<OutputT, 2>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
  if(nComp == 3)
  {
    using InputPixelType = itk::Vector<InputT, 3>;
    using OutputPixelType = itk::Vector<OutputT, 3>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
  if(nComp == 10)
  {
    using InputPixelType = itk::Vector<InputT, 10>;
    using OutputPixelType = itk::Vector<OutputT, 10>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
  if(nComp == 11)
  {
    using InputPixelType = itk::Vector<InputT, 11>;
    using OutputPixelType = itk::Vector<OutputT, 11>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
  if(nComp == 36)
  {
    using InputPixelType = itk::Vector<InputT, 36>;
    using OutputPixelType = itk::Vector<OutputT, 36>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_RGB_RGBA == 1
  if(nComp == 3)
  {
    using InputPixelType = itk::RGBPixel<InputT>;
    using OutputPixelType = itk::RGBPixel<OutputT>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
  if(nComp == 4)
  {
    using InputPixelType = itk::RGBAPixel<InputT>;
    using OutputPixelType = itk::RGBAPixel<OutputT>;
    return FunctorT<InputPixelType, OutputPixelType, Dimensions>()(std::forward<ArgsT>(args)...);
  }
#endif

  return MakeErrorResult<ResultT>(errorCode,
                                  fmt::format("Vector dimension not supported. cDims[0] = {} Try converting the selected input image to an image with scalar components using 'ITK::RGB to Luminance "
                                              "ImageFilter' or 'Convert Rgb To GrayScale' filters",
                                              nComp));
}

template <class InputT, class OutputT, class ResultT, template <class, class, uint32> class FunctorT, class... ArgsT>
Result<ResultT> ArraySwitchFuncDimsImpl(const IDataStore& dataStore, const ImageGeom& imageGeom, int32 errorCode, ArgsT&&... args)
{
  auto tDims = imageGeom.getDimensions();
  auto cDims = dataStore.getComponentShape();

  usize nComp = cDims[0];

  // If the image dimensions are X Y 1 i.e. 2D
  if(tDims.getZ() == 1)
  {
    return ArraySwitchFuncComponentImpl<InputT, OutputT, 2, ResultT, FunctorT>(nComp, errorCode, args...);
  }
  else
  {
    return ArraySwitchFuncComponentImpl<InputT, OutputT, 3, ResultT, FunctorT>(nComp, errorCode, args...);
  }
}

template <class InputPixelT, class OutputPixelT, uint32 Dimension>
Result<OutputActions> DataCheckImpl(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath)
{
  using InputValueType = ITK::UnderlyingType_t<InputPixelT>;
  using OutputValueType = ITK::UnderlyingType_t<OutputPixelT>;

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  const auto& dataArray = dataStructure.getDataRefAs<DataArray<InputValueType>>(inputArrayPath);

  const IDataStore& dataStore = dataArray.getIDataStoreRef();

  if(!ITK::DoDimensionsMatch(dataStore, imageGeom))
  {
    return MakeErrorResult<OutputActions>(-1, "DataArray dimensions don't match ImageGeom");
  }

  std::vector<usize> cDims = dataStore.getComponentShape();
  std::vector<usize> inputPixelDims = ITK::GetComponentDimensions<InputPixelT>();

  if(cDims != inputPixelDims)
  {
    return MakeErrorResult<OutputActions>(-2, "DataArray component dimensions don't match output image components");
  }

  OutputActions outputActions;

  NumericType outputType = GetNumericType<OutputValueType>();
  SizeVec3 imageDims = imageGeom.getDimensions();
  std::vector<usize> tDims(std::make_reverse_iterator(imageDims.end()), std::make_reverse_iterator(imageDims.begin()));
  std::vector<usize> outputPixelDims = ITK::GetComponentDimensions<OutputPixelT>();

  outputActions.actions.push_back(std::make_unique<CreateArrayAction>(outputType, tDims, outputPixelDims, outputArrayPath));

  return {std::move(outputActions)};
}

template <class InputT, class OutputT, uint32 Dimension>
struct DataCheckImplFunctor
{
  Result<OutputActions> operator()(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath) const
  {
    return DataCheckImpl<InputT, OutputT, Dimension>(dataStructure, inputArrayPath, imageGeomPath, outputArrayPath);
  }
};

template <class InputT, class OutputT, uint32 Dimension>
struct ITKFilterFunctor
{
  template <class FilterCreationFunctorT>
  Result<> operator()(IDataStore& inputDataStore, const ImageGeom& imageGeom, IDataStore& outputDataStore, const FilterCreationFunctorT& filterCreationFunctor) const
  {
    using InputImageType = itk::Image<InputT, Dimension>;
    using OutputImageType = itk::Image<OutputT, Dimension>;
    auto& typedInputDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<InputT>>&>(inputDataStore);
    typename InputImageType::Pointer inputImage = ITK::WrapDataStoreInImage<InputT, Dimension>(typedInputDataStore, imageGeom);
    auto filter = filterCreationFunctor.template operator()<InputImageType, OutputImageType, Dimension>();
    filter->SetInput(inputImage);
    filter->Update();

    typename OutputImageType::Pointer outputImage = filter->GetOutput();
    outputImage->DisconnectPipeline();
    auto& typedOuputDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<OutputT>>&>(inputDataStore);
    auto imageDataStore = ITK::ConvertImageToDataStore(*outputImage);
    typedOuputDataStore = std::move(imageDataStore);

    return {};
  }
};
} // namespace detail

template <template <class, class, uint32> class FunctorT, class ResultT = void, class... ArgsT>
Result<ResultT> ArraySwitchFunc(const IDataStore& dataStore, const ImageGeom& imageGeom, int32 errorCode, ArgsT&&... args)
{
  DataType type = dataStore.getDataType();

  switch(type)
  {
#if COMPLEX_ITK_ARRAY_HELPER_USE_int8 == 1
  case DataType::int8: {
    return detail::ArraySwitchFuncDimsImpl<int8, int8, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_uint8 == 1
  case DataType::uint8: {
    return detail::ArraySwitchFuncDimsImpl<uint8, uint8, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_int16 == 1
  case DataType::int16: {
    return detail::ArraySwitchFuncDimsImpl<int16, int16, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_uint16 == 1
  case DataType::uint16: {
    return detail::ArraySwitchFuncDimsImpl<uint16, uint16, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_int32 == 1
  case DataType::int32: {
    return detail::ArraySwitchFuncDimsImpl<int32, int32, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_uint32 == 1
  case DataType::uint32: {
    return detail::ArraySwitchFuncDimsImpl<uint32, uint32, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_int64 == 1
  case DataType::int64: {
    return detail::ArraySwitchFuncDimsImpl<int64, int64, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_uint64 == 1
  case DataType::uint64: {
    return detail::ArraySwitchFuncDimsImpl<uint64, uint64, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_float32 == 1
  case DataType::float32: {
    return detail::ArraySwitchFuncDimsImpl<float32, float32, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
#if COMPLEX_ITK_ARRAY_HELPER_USE_float64 == 1
  case DataType::float64: {
    return detail::ArraySwitchFuncDimsImpl<float64, float64, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
  }
#endif
  default: {
    return MakeErrorResult<ResultT>(-1000, "Invalid DataType while attempting to execute");
  }
  }
}

inline Result<OutputActions> DataCheck(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& dataArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const auto& dataStore = dataArray.getIDataStoreRef();

  return ArraySwitchFunc<detail::DataCheckImplFunctor, OutputActions>(dataStore, imageGeom, -1, dataStructure, inputArrayPath, imageGeomPath, outputArrayPath);
}

template <class FilterCreationFunctorT>
Result<> Execute(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath, FilterCreationFunctorT filterCreationFunctor)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  auto& outputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  auto& inputDataStore = inputArray.getIDataStoreRef();
  auto& outputDataStore = outputArray.getIDataStoreRef();

  try
  {
    return ArraySwitchFunc<detail::ITKFilterFunctor>(inputDataStore, imageGeom, -1, inputDataStore, imageGeom, outputDataStore, filterCreationFunctor);
  } catch(const itk::ExceptionObject& exception)
  {
    return MakeErrorResult(-2, exception.GetDescription());
  }
}
} // namespace ITK
} // namespace complex
