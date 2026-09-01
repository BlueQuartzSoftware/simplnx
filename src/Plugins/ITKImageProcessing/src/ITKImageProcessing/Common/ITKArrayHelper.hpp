#pragma once

#include "ITKImageProcessing/Common/ITKDream3DFilterInterruption.hpp"
#include "ITKImageProcessing/Common/ITKProgressObserver.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataStore.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Output.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <itkCastImageFilter.h>
#include <itkImage.h>
#include <itkImageIOBase.h>
#include <itkImportImageFilter.h>
#include <itkIntTypes.h>
#include <itkNumericTraits.h>
#include <itkVector.h>

#include <fmt/core.h>
#include <fmt/ranges.h>

#include <stdexcept>
#include <type_traits>
#include <vector>

namespace nx::core::ITK
{
namespace detail
{
/**
 * @struct TypeConversionValidateFunctor
 * @brief Tests whether the limited preflight conversion supports two scalar types.
 * @tparam OriginT Source scalar type.
 *
 * The conversion path accepts different unsigned integer types. It rejects
 * identity, signed, floating-point, and Boolean conversions.
 */
template <typename OriginT>
struct TypeConversionValidateFunctor
{
  template <typename DestT>
  bool operator()()
  {
    if constexpr(std::is_same_v<OriginT, DestT>)
    {
      // An identical type does not need a conversion copy.
      return false;
    }
    if constexpr(std::is_signed_v<OriginT> || std::is_signed_v<DestT>)
    {
      return false;
    }
    if constexpr(std::is_same_v<OriginT, bool> || std::is_same_v<DestT, bool>)
    {
      return false;
    }

    // Currently only works on uint8, uint16, or uint32
    return true;
  }
};

/**
 * @struct PreflightTypeConversionValidateFunctor
 * @brief Dispatches a runtime destination type to the conversion support test.
 */
struct PreflightTypeConversionValidateFunctor
{
  template <typename OriginT>
  bool operator()(const DataType& destType)
  {
    return ExecuteNeighborFunction(TypeConversionValidateFunctor<OriginT>{}, destType);
  }
};

DataType ConvertChoiceToDataType(usize choice);
} // namespace detail

/**
 * @struct ImageGeomData
 * @brief Stores the image metadata needed by compile-time ITK dispatch.
 *
 * This value snapshot lets dispatch code use geometry metadata without retaining
 * an ImageGeom reference.
 */
struct ImageGeomData
{
  SizeVec3 dims{};
  FloatVec3 origin{};
  FloatVec3 spacing{};

  ImageGeomData() = default;

  ImageGeomData(SizeVec3 inputDims, FloatVec3 inputOrigin, FloatVec3 inputSpacing)
  : dims(std::move(inputDims))
  , origin(std::move(inputOrigin))
  , spacing(std::move(inputSpacing))
  {
  }

  ImageGeomData(const ImageGeom& imageGeom)
  : ImageGeomData(imageGeom.getDimensions(), imageGeom.getOrigin(), imageGeom.getSpacing())
  {
  }
};

inline const std::set<DataType>& GetScalarPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {nx::core::DataType::int8,   nx::core::DataType::uint8, nx::core::DataType::int16,  nx::core::DataType::uint16,  nx::core::DataType::int32,
                                               nx::core::DataType::uint32, nx::core::DataType::int64, nx::core::DataType::uint64, nx::core::DataType::float32, nx::core::DataType::float64};
  return dataTypes;
}

inline const std::set<DataType>& GetIntegerScalarPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {nx::core::DataType::int8,  nx::core::DataType::uint8,  nx::core::DataType::int16, nx::core::DataType::uint16,
                                               nx::core::DataType::int32, nx::core::DataType::uint32, nx::core::DataType::int64, nx::core::DataType::uint64};
  return dataTypes;
}

inline const std::set<DataType>& GetFloatingScalarPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {nx::core::DataType::float32, nx::core::DataType::float64};
  return dataTypes;
}

inline const std::set<DataType>& GetNonLabelPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {nx::core::DataType::int8,   nx::core::DataType::uint8, nx::core::DataType::int16,  nx::core::DataType::uint16,  nx::core::DataType::int32,
                                               nx::core::DataType::uint32, nx::core::DataType::int64, nx::core::DataType::uint64, nx::core::DataType::float32, nx::core::DataType::float64};
  return dataTypes;
}

inline const std::set<DataType>& GetScalarVectorPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {};
  return dataTypes;
}

inline const std::set<DataType>& GetFloatingVectorPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {nx::core::DataType::float32, nx::core::DataType::float64};
  return dataTypes;
}

inline const std::set<DataType>& GetSignedIntegerScalarPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {
      nx::core::DataType::int8,
      nx::core::DataType::int16,
      nx::core::DataType::int32,
      nx::core::DataType::int64,
  };
  return dataTypes;
}

inline const std::set<DataType>& GetSignedScalarPixelAllowedTypes()
{
  static const std::set<DataType> dataTypes = {
      nx::core::DataType::int8, nx::core::DataType::int16, nx::core::DataType::int32, nx::core::DataType::int64, nx::core::DataType::float32, nx::core::DataType::float64,
  };
  return dataTypes;
}

namespace Constants
{
inline constexpr int32 k_ImageGeometryDimensionMismatch = -2000;
inline constexpr int32 k_ImageComponentDimensionMismatch = -2001;
inline constexpr int32 k_OutOfCoreDataNotSupported = -2002;

} // namespace Constants

/**
 * @brief Converts an ITK I/O component type to a simplnx numeric type.
 * @param component ITK component type to convert.
 * @return Matching numeric type, or std::nullopt for an unsupported component.
 */
inline constexpr std::optional<NumericType> ConvertIOComponentToNumericType(itk::ImageIOBase::IOComponentEnum component) noexcept
{
  using ComponentType = itk::ImageIOBase::IOComponentEnum;

  switch(component)
  {
  case ComponentType::UCHAR: {
    return NumericType::uint8;
  }
  case ComponentType::CHAR: {
    return NumericType::int8;
  }
  case ComponentType::USHORT: {
    return NumericType::uint16;
  }
  case ComponentType::SHORT: {
    return NumericType::int16;
  }
  case ComponentType::UINT: {
    return NumericType::uint32;
  }
  case ComponentType::INT: {
    return NumericType::int32;
  }
  case ComponentType::ULONG: {
    return NumericType::uint64;
  }
  case ComponentType::LONG: {
    return NumericType::int64;
  }
  case ComponentType::FLOAT: {
    return NumericType::float32;
  }
  case ComponentType::DOUBLE: {
    return NumericType::float64;
  }
  default: {
    return {};
  }
  }
}

/**
 * @brief Converts an ITK I/O component type to a simplnx data type.
 * @param component ITK component type to convert.
 * @return Matching data type, or std::nullopt for an unsupported component.
 */
inline constexpr std::optional<DataType> ConvertIOComponentToDataType(itk::ImageIOBase::IOComponentEnum component) noexcept
{
  using ComponentType = itk::ImageIOBase::IOComponentEnum;

  switch(component)
  {
  case ComponentType::UCHAR: {
    return DataType::uint8;
  }
  case ComponentType::CHAR: {
    return DataType::int8;
  }
  case ComponentType::USHORT: {
    return DataType::uint16;
  }
  case ComponentType::SHORT: {
    return DataType::int16;
  }
  case ComponentType::UINT: {
    return DataType::uint32;
  }
  case ComponentType::INT: {
    return DataType::int32;
  }
  case ComponentType::ULONG: {
    return DataType::uint64;
  }
  case ComponentType::LONG: {
    return DataType::int64;
  }
  case ComponentType::FLOAT: {
    return DataType::float32;
  }
  case ComponentType::DOUBLE: {
    return DataType::float64;
  }
  default: {
    return {};
  }
  }
}

/**
 * @brief Copies up to three vector components into an ITK fixed-size value.
 * @tparam OutputType ITK size, index, point, or similar fixed-size type.
 * @tparam ComponentType Destination component type.
 * @tparam InputType Three-component simplnx vector type.
 * @param inputVec3 Source vector.
 * @param dimension Number of components to copy in the range [0, 3].
 * @return Output value with copied components.
 * @pre OutputType has at least dimension components.
 */
template <class OutputType, class ComponentType, class InputType>
OutputType CastVec3ToITK(const InputType& inputVec3, uint32 dimension)
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
 * @brief Validates that an array has one of the permitted data types.
 * @param types Permitted data types.
 * @param dataStructure Data structure that contains the array.
 * @param path Array path to validate.
 * @return Success, or an error when the path is not an array or its type is not permitted.
 */
Result<> CheckImageType(const std::vector<DataType>& types, const DataStructure& dataStructure, const DataPath& path);

/**
 * @brief
 * @tparam T
 */
template <class T>
using UnderlyingType_t = typename itk::NumericTraits<T>::ValueType;

template <class PixelT>
std::vector<usize> GetComponentDimensions()
{
  return {itk::NumericTraits<PixelT>::GetLength()};
}

/**
 * @brief Gives an ITK image a zero-copy view of an in-memory data store.
 *
 * @tparam PixelT ITK pixel type.
 * @tparam Dimensions Image dimensionality (2 or 3).
 * @param dataStore In-memory store whose buffer supplies the pixels.
 * @param imageGeom Supplies dimensions, origin, and spacing.
 * @return ITK image that refers to the data-store buffer.
 * @pre Dimensions is 2 or 3, and the geometry dimensions fit the store.
 * @pre dataStore outlives the returned image and all filters that use it.
 *
 * The data store retains buffer ownership. ITK must not release the imported
 * pointer.
 */
template <class PixelT, uint32 Dimensions>
typename itk::Image<PixelT, Dimensions>::Pointer WrapDataStoreInImage(DataStore<UnderlyingType_t<PixelT>>& dataStore, const ImageGeomData& imageGeom)
{
  using T = ITK::UnderlyingType_t<PixelT>;

  static_assert(std::is_standard_layout_v<PixelT>, "nx::core::ITK::WrapDataStoreInImage: PixelT must be standard layout");
  // static_assert(std::is_trivial_v<PixelT>, "nx::core::ITK::WrapDataStoreInImage: PixelT must be trivial");
  static_assert(std::is_arithmetic_v<T>, "nx::core::ITK::WrapDataStoreInImage: The underlying type T of PixelT must be arithmetic");
  static_assert(sizeof(PixelT) % sizeof(T) == 0, "nx::core::ITK::WrapDataStoreInImage: The size of PixelT must be evenly divisible by T");

  using FilterType = itk::ImportImageFilter<PixelT, Dimensions>;

  SizeVec3 geomDims = imageGeom.dims;
  FloatVec3 geomOrigin = imageGeom.origin;
  FloatVec3 geomSpacing = imageGeom.spacing;

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

template <class PixelT, uint32 Dimensions>
typename itk::Image<PixelT, Dimensions>::Pointer WrapDataStoreInImage(DataStore<UnderlyingType_t<PixelT>>& dataStore, const ImageGeom& imageGeom)
{
  return WrapDataStoreInImage<PixelT, Dimensions>(dataStore, ImageGeomData(imageGeom));
}

/**
 * @brief Transfers an ITK image buffer to an in-memory data store.
 *
 * @tparam PixelT ITK pixel type.
 * @tparam Dimension Image dimensionality (2 or 3).
 * @param image Source image whose buffer transfers.
 * @param dataStore Destination store that takes sole buffer ownership.
 * @pre The image is disconnected from its ITK pipeline.
 * @pre ITK allocated the buffer with the global array-new allocator.
 *
 * The method disables pixel-container ownership before it installs the buffer
 * in dataStore. The image does not manage the buffer after this operation.
 */
template <class PixelT, uint32 Dimension>
void ConvertImageToDataStore(itk::Image<PixelT, Dimension>& image, DataStore<UnderlyingType_t<PixelT>>& dataStore)
{
  using ImageType = itk::Image<PixelT, Dimension>;
  using T = UnderlyingType_t<PixelT>;
  typename ImageType::SizeType imageSize = image.GetLargestPossibleRegion().GetSize();
  ShapeType tDims(imageSize.rbegin(), imageSize.rend());
  ShapeType cDims = GetComponentDimensions<PixelT>();
  if constexpr(Dimension == 2)
  {
    tDims.insert(tDims.begin(), 1);
  }
  typename ImageType::PixelContainer* pixelContainer = image.GetPixelContainer();
  // ITK uses the global array-new allocator, so DataStore can own this buffer.
  auto* bufferPtr = reinterpret_cast<T*>(pixelContainer->GetBufferPointer());
  pixelContainer->ContainerManageMemoryOff();
  std::unique_ptr<T[]> newDataPtr(bufferPtr);
  dataStore = DataStore<T>(std::move(newDataPtr), std::move(tDims), std::move(cDims));
}

/**
 * @struct ArrayComponentOptions
 * @brief Selects scalar, vector, and color component dispatch at compile time.
 * @tparam UseScalarV True to enable one-component scalar pixels.
 * @tparam UseVectorV True to enable supported itk::Vector component counts.
 * @tparam UseRgbRgbaV True to enable RGB and RGBA pixels.
 *
 * Vector and color modes cannot both be active because a three-component array
 * would have two meanings.
 */
template <bool UseScalarV, bool UseVectorV, bool UseRgbRgbaV>
struct ArrayComponentOptions
{
  static inline constexpr bool UsingScalar = UseScalarV;
  static inline constexpr bool UsingVector = UseVectorV;
  static inline constexpr bool UsingRgbRgba = UseRgbRgbaV;

  static_assert(!(UsingVector && UsingRgbRgba), "ITKArrayHelper: Vector and RGB/RGBA support cannot both be enabled at the same time");
};

/**
 * @struct ArrayTypeOptions
 * @brief Selects scalar storage types for compile-time ITK dispatch.
 * @tparam UseInt8V True to enable int8.
 * @tparam UseUInt8V True to enable uint8.
 * @tparam UseInt16V True to enable int16.
 * @tparam UseUInt16V True to enable uint16.
 * @tparam UseInt32V True to enable int32.
 * @tparam UseUInt32V True to enable uint32.
 * @tparam UseInt64V True to enable int64.
 * @tparam UseUInt64V True to enable uint64.
 * @tparam UseFloat32V True to enable float32.
 * @tparam UseFloat64V True to enable float64.
 */
template <bool UseInt8V, bool UseUInt8V, bool UseInt16V, bool UseUInt16V, bool UseInt32V, bool UseUInt32V, bool UseInt64V, bool UseUInt64V, bool UseFloat32V, bool UseFloat64V>
struct ArrayTypeOptions
{
  static inline constexpr bool UsingInt8 = UseInt8V;
  static inline constexpr bool UsingUInt8 = UseUInt8V;
  static inline constexpr bool UsingInt16 = UseInt16V;
  static inline constexpr bool UsingUInt16 = UseUInt16V;
  static inline constexpr bool UsingInt32 = UseInt32V;
  static inline constexpr bool UsingUInt32 = UseUInt32V;
  static inline constexpr bool UsingInt64 = UseInt64V;
  static inline constexpr bool UsingUInt64 = UseUInt64V;
  static inline constexpr bool UsingFloat32 = UseFloat32V;
  static inline constexpr bool UsingFloat64 = UseFloat64V;
};

/**
 * @struct ArrayOptions
 * @brief Combines component and scalar-type dispatch policies.
 * @tparam ComponentOptionsT ArrayComponentOptions policy.
 * @tparam TypeOptionsT ArrayTypeOptions policy.
 */
template <class ComponentOptionsT, class TypeOptionsT>
struct ArrayOptions
{
  using ComponentOptions = ComponentOptionsT;
  using TypeOptions = TypeOptionsT;
};

using ArrayUseAllTypes = ArrayTypeOptions<true, true, true, true, true, true, true, true, true, true>;
using ArrayUseIntegerTypes = ArrayTypeOptions<true, true, true, true, true, true, true, true, false, false>;
using ArrayUseFloatingTypes = ArrayTypeOptions<false, false, false, false, false, false, false, false, true, true>;
using ArrayUseSignedTypes = ArrayTypeOptions<true, false, true, false, true, false, true, false, true, true>;
using ArrayUseUnsignedTypes = ArrayTypeOptions<false, true, false, true, false, true, false, true, false, false>;

using ArrayUseScalarOnly = ArrayComponentOptions<true, false, false>;
using ArrayUseVectorOnly = ArrayComponentOptions<false, true, false>;
using ArrayUseRgbRgbaOnly = ArrayComponentOptions<false, false, true>;
using ArrayUseScalarVector = ArrayComponentOptions<true, true, false>;

using ScalarPixelIdTypeList = ArrayOptions<ArrayUseScalarOnly, ArrayUseAllTypes>;
using VectorPixelIdTypeList = ArrayOptions<ArrayUseVectorOnly, ArrayUseAllTypes>;
using ScalarVectorPixelIdTypeList = ArrayOptions<ArrayUseScalarVector, ArrayUseAllTypes>;

using IntegerScalarPixelIdTypeList = ArrayOptions<ArrayUseScalarOnly, ArrayUseIntegerTypes>;
using FloatingScalarPixelIdTypeList = ArrayOptions<ArrayUseScalarOnly, ArrayUseFloatingTypes>;

using IntegerVectorPixelIdTypeList = ArrayOptions<ArrayUseVectorOnly, ArrayUseIntegerTypes>;
using FloatingVectorPixelIdTypeList = ArrayOptions<ArrayUseVectorOnly, ArrayUseFloatingTypes>;

using SignedIntegerScalarPixelIdTypeList = ArrayOptions<ArrayUseScalarOnly, ArrayUseSignedTypes>;

using SignedScalarPixelIdTypeList = ArrayOptions<ArrayUseScalarOnly, ArrayUseSignedTypes>;

namespace detail
{
/**
 * @brief Dispatches an ITK functor for a supported component representation.
 * @tparam InputT Input scalar type.
 * @tparam OutputT Output scalar type.
 * @tparam Dimensions Image rank.
 * @tparam ComponentOptionsT Enabled component representations.
 * @tparam ResultT Functor result value type.
 * @tparam FunctorT ITK operation template.
 * @tparam ArgsT Forwarded operation argument types.
 * @param nComp Input component count.
 * @param errorCode Error code for an unsupported component count.
 * @param args Arguments passed to the selected operation.
 * @return Selected operation result, or an unsupported-component error.
 *
 * Vector dispatch supports 2, 3, 10, 11, and 36 components. Color dispatch
 * supports RGB and RGBA representations.
 */
template <class InputT, class OutputT, usize Dimensions, class ComponentOptionsT, class ResultT, template <class, class, uint32> class FunctorT, class... ArgsT>
Result<ResultT> ArraySwitchFuncComponentImpl(usize nComp, int32 errorCode, ArgsT&&... args)
{
  if constexpr(ComponentOptionsT::UsingScalar)
  {
    if(nComp == 1)
    {
      return FunctorT<InputT, OutputT, Dimensions>()(std::forward<ArgsT>(args)...);
    }
  }

  if constexpr(ComponentOptionsT::UsingVector)
  {
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
  }

  if constexpr(ComponentOptionsT::UsingRgbRgba)
  {
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
  }

  return MakeErrorResult<ResultT>(errorCode,
                                  fmt::format("Vector dimension not supported. cDims[0] = {} Try converting the selected input image to an image with scalar components using 'ITK::RGB to Luminance "
                                              "ImageFilter' or 'Convert Rgb To GrayScale' filters",
                                              nComp));
}

/**
 * @brief Selects two- or three-dimensional ITK dispatch from geometry metadata.
 * @tparam InputT Input scalar type.
 * @tparam OutputT Output scalar type.
 * @tparam ComponentOptionsT Enabled component representations.
 * @tparam ResultT Functor result value type.
 * @tparam FunctorT ITK operation template.
 * @tparam ArgsT Forwarded operation argument types.
 * @param dataStore Supplies the component shape.
 * @param imageGeom Supplies image dimensions.
 * @param errorCode Error code for unsupported components.
 * @param args Arguments passed to the selected operation.
 * @return Selected operation result or dispatch error.
 * @pre dataStore has at least one component dimension.
 *
 * A Z dimension of one selects a two-dimensional ITK image. Other Z dimensions
 * select a three-dimensional image.
 */
template <class InputT, class OutputT, class ComponentOptionsT, class ResultT, template <class, class, uint32> class FunctorT, class... ArgsT>
Result<ResultT> ArraySwitchFuncDimsImpl(const IDataStore& dataStore, const ImageGeomData& imageGeom, int32 errorCode, ArgsT&&... args)
{
  auto tDims = imageGeom.dims;
  auto cDims = dataStore.getComponentShape();

  usize nComp = cDims[0];

  // If the image dimensions are X Y 1 i.e. 2D
  if(tDims.getZ() == 1)
  {
    return ArraySwitchFuncComponentImpl<InputT, OutputT, 2, ComponentOptionsT, ResultT, FunctorT>(nComp, errorCode, args...);
  }

  return ArraySwitchFuncComponentImpl<InputT, OutputT, 3, ComponentOptionsT, ResultT, FunctorT>(nComp, errorCode, args...);
}

/**
 * @brief Validates one dispatched ITK input and describes its output array.
 * @tparam InputPixelT Dispatched ITK input pixel type.
 * @tparam OutputPixelT Dispatched ITK output pixel type.
 * @tparam Dimension ITK image rank.
 * @param dataStructure Contains the geometry and input array.
 * @param inputArrayPath Input array path.
 * @param imageGeomPath Input ImageGeom path.
 * @param outputArrayPath Output array path.
 * @return Output creation action or a dimension error.
 *
 * The output requests the input store format. An empty format delegates storage
 * selection to the data-structure resolver.
 */
template <class InputPixelT, class OutputPixelT, uint32 Dimension>
Result<OutputActions> DataCheckImpl(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath)
{
  using InputValueType = ITK::UnderlyingType_t<InputPixelT>;
  using OutputValueType = ITK::UnderlyingType_t<OutputPixelT>;

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  const auto& dataArray = dataStructure.getDataRefAs<DataArray<InputValueType>>(inputArrayPath);

  const IDataStore& dataStore = dataArray.getIDataStoreRef();

  if(!nx::core::DoDimensionsMatch(dataStore, imageGeom) && !nx::core::DoTuplesMatch(dataStore, imageGeom))
  {

    std::string errMessage = fmt::format("DataArray '{}' dimensions ({}) do not match Image Geometry '{}' dimensions ({}).", inputArrayPath.toString(),
                                         StringUtilities::formatTupleShape3D(dataStore.getTupleShape()), imageGeomPath.toString(), StringUtilities::formatDimensions3D(imageGeom.getDimensions()));
    return MakeErrorResult<OutputActions>(nx::core::ITK::Constants::k_ImageGeometryDimensionMismatch, errMessage);
  }

  ShapeType cDims = dataStore.getComponentShape();
  std::vector<usize> inputPixelDims = ITK::GetComponentDimensions<InputPixelT>();

  if(cDims != inputPixelDims)
  {
    const std::string errMessage =
        fmt::format("DataArray component dimensions of '{}' do not match output image component dimensions of '{}'", fmt::join(inputPixelDims, ", "), fmt::join(cDims, ", "));
    return MakeErrorResult<OutputActions>(nx::core::ITK::Constants::k_ImageComponentDimensionMismatch, errMessage);
  }

  OutputActions outputActions;

  DataType outputType = GetDataType<OutputValueType>();
  SizeVec3 imageDims = imageGeom.getDimensions();
  ShapeType tDims(std::make_reverse_iterator(imageDims.end()), std::make_reverse_iterator(imageDims.begin()));
  std::vector<usize> outputPixelDims = ITK::GetComponentDimensions<OutputPixelT>();

  outputActions.appendAction(std::make_unique<CreateArrayAction>(outputType, tDims, outputPixelDims, outputArrayPath, dataStore.getDataFormat()));

  return {std::move(outputActions)};
}

/**
 * @struct DataCheckImplFunctor
 * @brief Adapts DataCheckImpl() to the array-dispatch interface.
 * @tparam InputT Dispatched input pixel type.
 * @tparam OutputT Dispatched output pixel type.
 * @tparam Dimension ITK image rank.
 */
template <class InputT, class OutputT, uint32 Dimension>
struct DataCheckImplFunctor
{
  Result<OutputActions> operator()(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath) const
  {
    return DataCheckImpl<InputT, OutputT, Dimension>(dataStructure, inputArrayPath, imageGeomPath, outputArrayPath);
  }
};

/**
 * @struct ITKFilterFunctorResult
 * @brief Selects void when an ITK filter-creation functor has no Measurements type.
 * @tparam T Filter-creation functor type.
 */
template <class T, class = void>
struct ITKFilterFunctorResult
{
  using type = void;
};

template <class T>
using Measurements_t = typename T::Measurements;

/**
 * @struct ITKFilterFunctorResult
 * @brief Selects a filter-creation functor's Measurements type when it exists.
 * @tparam T Filter-creation functor type.
 */
template <class T>
struct ITKFilterFunctorResult<T, std::void_t<Measurements_t<T>>>
{
  static_assert(!std::is_same_v<Measurements_t<T>, void>);
  using type = Measurements_t<T>;
};

template <class T>
using ITKFilterFunctorResult_t = typename ITKFilterFunctorResult<T>::type;

template <class T>
inline constexpr bool HasMeasurements_v = !std::is_same_v<ITKFilterFunctorResult_t<T>, void>;

/**
 * @struct HasIntermediateTypeHelper
 * @brief Selects void when a filter-creation functor has no IntermediateType.
 * @tparam T Filter-creation functor type.
 */
template <class T, class = void>
struct HasIntermediateTypeHelper
{
  using type = void;
};

template <class T>
using IntermediateType_t = typename T::IntermediateType;

/**
 * @struct HasIntermediateTypeHelper
 * @brief Selects a filter-creation functor's IntermediateType when it exists.
 * @tparam T Filter-creation functor type.
 */
template <class T>
struct HasIntermediateTypeHelper<T, std::void_t<IntermediateType_t<T>>>
{
  static_assert(!std::is_same_v<IntermediateType_t<T>, void>);
  using type = IntermediateType_t<T>;
};

template <class T>
using HasInterMediateTypeHelper_t = typename HasIntermediateTypeHelper<T>::type;

template <class T>
inline constexpr bool HasIntermediateType_v = !std::is_same_v<HasInterMediateTypeHelper_t<T>, void>;

/**
 * @struct ITKFilterFunctor
 * @brief Executes an ITK filter against contiguous simplnx buffers.
 * @tparam InputT ITK input pixel type.
 * @tparam OutputT ITK output pixel type.
 * @tparam Dimension ITK image rank.
 *
 * The input store supplies a zero-copy ITK view. The output ITK buffer transfers
 * to the output store. Both IDataStore references must have concrete DataStore
 * dynamic types. The cancellation observer asks ITK to abort during a progress
 * event; ITK controls when that event occurs and whether Update() throws.
 */
template <class InputT, class OutputT, uint32 Dimension>
struct ITKFilterFunctor
{
  template <class FilterCreationFunctorT>
  Result<ITKFilterFunctorResult_t<FilterCreationFunctorT>> execute(IDataStore& inputDataStore, const ImageGeom& imageGeom, IDataStore& outputDataStore, const std::atomic_bool& shouldCancel,
                                                                   const itk::ProgressObserver::Pointer progressObserver, const FilterCreationFunctorT& filterCreationFunctor) const
  {
    using InputImageType = itk::Image<InputT, Dimension>;
    using OutputImageType = itk::Image<OutputT, Dimension>;

    auto& typedInputDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<InputT>>&>(inputDataStore);
    typename InputImageType::Pointer inputImage = ITK::WrapDataStoreInImage<InputT, Dimension>(typedInputDataStore, imageGeom);
    auto filter = filterCreationFunctor.template createFilter<InputImageType, OutputImageType, Dimension>();
    if(progressObserver != nullptr)
    {
      filter->AddObserver(itk::ProgressEvent(), progressObserver);
    }
    itk::Dream3DFilterInterruption::Pointer interruption = itk::Dream3DFilterInterruption::New(shouldCancel);
    filter->AddObserver(itk::ProgressEvent(), interruption);
    filter->SetInput(inputImage);
    filter->Update();

    typename OutputImageType::Pointer outputImage = filter->GetOutput();
    outputImage->DisconnectPipeline();

    auto& typedOutputDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<OutputT>>&>(outputDataStore);
    ITK::ConvertImageToDataStore(*outputImage, typedOutputDataStore);

    if constexpr(HasMeasurements_v<FilterCreationFunctorT>)
    {
      return {filterCreationFunctor.template getMeasurements<InputImageType, OutputImageType, Dimension>(*filter)};
    }
    else
    {
      return {};
    }
  }

  template <class FilterCreationFunctorT>
  Result<ITKFilterFunctorResult_t<FilterCreationFunctorT>> executeWithCast(IDataStore& inputDataStore, const ImageGeom& imageGeom, IDataStore& outputDataStore, const std::atomic_bool& shouldCancel,
                                                                           const itk::ProgressObserver::Pointer progressObserver, const FilterCreationFunctorT& filterCreationFunctor) const
  {
    using InputImageType = itk::Image<InputT, Dimension>;
    using OutputImageType = itk::Image<OutputT, Dimension>;

    using IntermediatePixelType = IntermediateType_t<FilterCreationFunctorT>;
    using IntermediateImageType = itk::Image<IntermediatePixelType, Dimension>;

    auto& typedInputDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<InputT>>&>(inputDataStore);
    typename InputImageType::Pointer inputImage = ITK::WrapDataStoreInImage<InputT, Dimension>(typedInputDataStore, imageGeom);

    using CastImageToIntermediateFilterType = itk::CastImageFilter<InputImageType, IntermediateImageType>;
    auto castImageToIntermediateFilter = CastImageToIntermediateFilterType::New();
    castImageToIntermediateFilter->SetInput(inputImage);

    auto filter = filterCreationFunctor.template createFilter<IntermediateImageType, IntermediateImageType, Dimension>();
    if(progressObserver != nullptr)
    {
      filter->AddObserver(itk::ProgressEvent(), progressObserver);
    }
    itk::Dream3DFilterInterruption::Pointer interruption = itk::Dream3DFilterInterruption::New(shouldCancel);
    filter->AddObserver(itk::ProgressEvent(), interruption);
    filter->SetInput(castImageToIntermediateFilter->GetOutput());

    using CastImageFromIntermediateFilterType = itk::CastImageFilter<IntermediateImageType, OutputImageType>;
    auto castImageFromIntermediateFilter = CastImageFromIntermediateFilterType::New();
    castImageFromIntermediateFilter->SetInput(filter->GetOutput());
    castImageFromIntermediateFilter->Update();

    typename OutputImageType::Pointer outputImage = castImageFromIntermediateFilter->GetOutput();
    outputImage->DisconnectPipeline();

    auto& typedOutputDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<OutputT>>&>(outputDataStore);
    ITK::ConvertImageToDataStore(*outputImage, typedOutputDataStore);

    if constexpr(HasMeasurements_v<FilterCreationFunctorT>)
    {
      return {filterCreationFunctor.template getMeasurements<InputImageType, OutputImageType, Dimension>(*filter)};
    }
    else
    {
      return {};
    }
  }

  template <class FilterCreationFunctorT>
  Result<ITKFilterFunctorResult_t<FilterCreationFunctorT>> operator()(IDataStore& inputDataStore, const ImageGeom& imageGeom, IDataStore& outputDataStore, const std::atomic_bool& shouldCancel,
                                                                      const itk::ProgressObserver::Pointer progressObserver, const FilterCreationFunctorT& filterCreationFunctor) const
  {
    if constexpr(HasIntermediateType_v<FilterCreationFunctorT>)
    {
      return executeWithCast<FilterCreationFunctorT>(inputDataStore, imageGeom, outputDataStore, shouldCancel, progressObserver, filterCreationFunctor);
    }
    else
    {
      return execute<FilterCreationFunctorT>(inputDataStore, imageGeom, outputDataStore, shouldCancel, progressObserver, filterCreationFunctor);
    }
  }
};

template <class OutputT, class DefaultOutputT>
using TrueOutputT = std::conditional_t<std::is_same_v<OutputT, void>, DefaultOutputT, OutputT>;

template <class T>
using DefaultOutput_t = std::void_t<T>;
} // namespace detail

/**
 * @brief Dispatches an operation from runtime store type, image rank, and component count.
 * @tparam FunctorT ITK operation template.
 * @tparam ArrayOptionsT Enabled scalar types and component representations.
 * @tparam ResultT Operation result value type.
 * @tparam OutputT Maps the input scalar type to an output scalar type.
 * @tparam ArgsT Forwarded operation argument types.
 * @param dataStore Supplies runtime scalar type and component shape.
 * @param imageGeom Supplies image dimensions.
 * @param errorCode Error code for an unsupported component representation.
 * @param args Arguments passed to the selected operation.
 * @return Selected operation result, or a dispatch error.
 */
template <template <class, class, uint32> class FunctorT, class ArrayOptionsT, class ResultT = void, template <class> class OutputT = detail::DefaultOutput_t, class... ArgsT>
Result<ResultT> ArraySwitchFunc(const IDataStore& dataStore, const ImageGeomData& imageGeom, int32 errorCode, ArgsT&&... args)
{
  DataType type = dataStore.getDataType();

  using TypeOptionsT = typename ArrayOptionsT::TypeOptions;
  using ComponentOptionsT = typename ArrayOptionsT::ComponentOptions;

  if constexpr(TypeOptionsT::UsingInt8)
  {
    if(type == DataType::int8)
    {
      return detail::ArraySwitchFuncDimsImpl<int8, detail::TrueOutputT<OutputT<int8>, int8>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingUInt8)
  {
    if(type == DataType::uint8)
    {
      return detail::ArraySwitchFuncDimsImpl<uint8, detail::TrueOutputT<OutputT<uint8>, uint8>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingInt16)
  {
    if(type == DataType::int16)
    {
      return detail::ArraySwitchFuncDimsImpl<int16, detail::TrueOutputT<OutputT<int16>, int16>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingUInt16)
  {
    if(type == DataType::uint16)
    {
      return detail::ArraySwitchFuncDimsImpl<uint16, detail::TrueOutputT<OutputT<uint16>, uint16>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingInt32)
  {
    if(type == DataType::int32)
    {
      return detail::ArraySwitchFuncDimsImpl<int32, detail::TrueOutputT<OutputT<int32>, int32>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingUInt32)
  {
    if(type == DataType::uint32)
    {
      return detail::ArraySwitchFuncDimsImpl<uint32, detail::TrueOutputT<OutputT<uint32>, uint32>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingInt64)
  {
    if(type == DataType::int64)
    {
      return detail::ArraySwitchFuncDimsImpl<int64, detail::TrueOutputT<OutputT<int64>, int64>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingUInt64)
  {
    if(type == DataType::uint64)
    {
      return detail::ArraySwitchFuncDimsImpl<uint64, detail::TrueOutputT<OutputT<uint64>, uint64>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingFloat32)
  {
    if(type == DataType::float32)
    {
      return detail::ArraySwitchFuncDimsImpl<float32, detail::TrueOutputT<OutputT<float32>, float32>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }
  if constexpr(TypeOptionsT::UsingFloat64)
  {
    if(type == DataType::float64)
    {
      return detail::ArraySwitchFuncDimsImpl<float64, detail::TrueOutputT<OutputT<float64>, float64>, ComponentOptionsT, ResultT, FunctorT>(dataStore, imageGeom, errorCode, args...);
    }
  }

  return MakeErrorResult<ResultT>(-1000, "Invalid DataType while attempting to execute");
}

template <template <class, class, uint32> class FunctorT, class ArrayOptionsT, class ResultT = void, template <class> class OutputT = detail::DefaultOutput_t, class... ArgsT>
Result<ResultT> ArraySwitchFunc(const IDataStore& dataStore, const ImageGeom& imageGeom, int32 errorCode, ArgsT&&... args)
{
  return ArraySwitchFunc<FunctorT, ArrayOptionsT, ResultT, OutputT>(dataStore, ImageGeomData(imageGeom), errorCode, args...);
}

/**
 * @brief Validates an ITK input and creates the output-array action.
 * @tparam ArrayOptionsT Enabled scalar types and component representations.
 * @tparam OutputT Maps the input scalar type to an output scalar type.
 * @param dataStructure Contains the input array and image geometry.
 * @param inputArrayPath Input array path.
 * @param imageGeomPath Input ImageGeom path.
 * @param outputArrayPath Output array path.
 * @return Output action or a type, shape, or storage error.
 *
 * ITK needs a contiguous input buffer. The method rejects a store whose physical
 * store type is OutOfCore instead of inferring residency from a format name.
 */
template <class ArrayOptionsT, template <class> class OutputT = detail::DefaultOutput_t>
Result<OutputActions> DataCheck(const DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath)
{
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  const auto& inputDataStore = inputArray.getIDataStoreRef();

  // ITK imports a contiguous pointer and cannot consume a disk-backed store.
  if(inputArray.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    return MakeErrorResult<OutputActions>(Constants::k_OutOfCoreDataNotSupported,
                                          fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", inputArrayPath.toString()));
  }

  return ArraySwitchFunc<detail::DataCheckImplFunctor, ArrayOptionsT, OutputActions, OutputT>(inputDataStore, imageGeom, -1, dataStructure, inputArrayPath, imageGeomPath, outputArrayPath);
}

/**
 * @brief Executes a dispatched ITK filter against simplnx arrays.
 * @tparam ArrayOptionsT Enabled scalar types and component representations.
 * @tparam OutputT Maps the input scalar type to an output scalar type.
 * @tparam FilterCreationFunctorT Creates and optionally reads the ITK filter.
 * @param dataStructure Contains the input, output, and geometry objects.
 * @param inputArrayPath Input array path.
 * @param imageGeomPath Input ImageGeom path.
 * @param outputArrayPath Output array path.
 * @param filterCreationFunctor Creates the selected ITK filter.
 * @param shouldCancel Supplies cancellation state to an ITK progress observer.
 * @param progressObserver Optional ITK progress observer.
 * @return Filter measurements, success, or an ITK and dispatch error.
 * @pre The input and output stores have concrete in-memory DataStore dynamic types.
 * @pre The referenced arrays, geometry, cancellation flag, and observer outlive this call.
 *
 * The method rejects an OutOfCore input store. It does not check the output
 * store type before ITKFilterFunctor casts it to DataStore. Only ITK exceptions
 * become Result errors; other C++ exceptions propagate.
 */
template <class ArrayOptionsT, template <class> class OutputT = detail::DefaultOutput_t, class FilterCreationFunctorT>
Result<detail::ITKFilterFunctorResult_t<FilterCreationFunctorT>> Execute(DataStructure& dataStructure, const DataPath& inputArrayPath, const DataPath& imageGeomPath, const DataPath& outputArrayPath,
                                                                         FilterCreationFunctorT&& filterCreationFunctor, const std::atomic_bool& shouldCancel,
                                                                         const itk::ProgressObserver::Pointer progressObserver = nullptr)
{
  auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  auto& inputArray = dataStructure.getDataRefAs<IDataArray>(inputArrayPath);
  auto& outputArray = dataStructure.getDataRefAs<IDataArray>(outputArrayPath);
  auto& inputDataStore = inputArray.getIDataStoreRef();
  auto& outputDataStore = outputArray.getIDataStoreRef();

  using ResultT = detail::ITKFilterFunctorResult_t<FilterCreationFunctorT>;

  // ITK imports a contiguous pointer and cannot consume a disk-backed store.
  if(inputArray.getStoreType() == IDataStore::StoreType::OutOfCore)
  {
    return MakeErrorResult(Constants::k_OutOfCoreDataNotSupported, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", inputArrayPath.toString()));
  }

  try
  {
    return ArraySwitchFunc<detail::ITKFilterFunctor, ArrayOptionsT, ResultT, OutputT>(inputDataStore, imageGeom, -1, inputDataStore, imageGeom, outputDataStore, shouldCancel, progressObserver,
                                                                                      filterCreationFunctor);
  } catch(const itk::ExceptionObject& exception)
  {
    return MakeErrorResult<ResultT>(-222, exception.GetDescription());
  }
}
} // namespace nx::core::ITK
