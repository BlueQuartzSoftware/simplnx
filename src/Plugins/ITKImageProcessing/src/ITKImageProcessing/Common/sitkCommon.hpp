#pragma once

#include <itkFlatStructuringElement.h>
/**
 * @brief On MSVC the above header (itkFlatStructuringelement.h) eventually brings in
 * `fileapi.h` which then defines 'CreateFile' as a macro. This interferes with
 * API in the `complex` library for reading HDF5 files. So we are going to `undef`
 * that macro here.
 */
#if defined(_MSC_VER) && defined(CreateFile)
#undef CreateFile
#endif

// Copied from sitkKernel.h (SimpleITK)
// Copied from sitkCreateKernel.h (SimpleITK)
namespace itk
{
namespace simple
{
enum PixelIDTypeList
{
  BasicPixelIDTypeList = 1,       // uint8, int8, uint16, int16, uint32, int32, float, double
  IntegerPixelIDTypeList = 2,     // uint8, int8, uint16, int16, uint32, int32
  NonLabelPixelIDTypeList = 3,    //  BasicPixelIDTypeList, ComplexPixelIDTypeList, VectorPixelIDTypeList
  RealPixelIDTypeList = 4,        // float, double
  RealVectorPixelIDTypeList = 5,  // VectorPixelID< float >, VectorPixelID< double >
  ScalarPixelIDTypeList = 6,      // uint8, int8, uint16, int16, uint32, int32, float, double
  SignedPixelIDTypeList = 7,      // int8, int16, int32, float, double
  VectorPixelIDTypeList = 8,      // Vector: uint8, int8, uint16, int16, uint32, int32, float, double
  SignedVectorPixelIDTypeList = 9 // Vector: int8, int16, int32, float, double
};
enum PixelIDValueEnum
{
  sitkUnknown = -1,
  sitkUInt8,          // = PixelIDToPixelIDValue< BasicPixelID<uint8_t> >::Result,   ///< Unsigned 8 bit integer
  sitkInt8,           // = PixelIDToPixelIDValue< BasicPixelID<int8_t> >::Result,     ///< Signed 8 bit integer
  sitkUInt16,         // = PixelIDToPixelIDValue< BasicPixelID<uint16_t> >::Result, ///< Unsigned 16 bit integer
  sitkInt16,          // = PixelIDToPixelIDValue< BasicPixelID<int16_t> >::Result,   ///< Signed 16 bit integer
  sitkUInt32,         // = PixelIDToPixelIDValue< BasicPixelID<uint32_t> >::Result, ///< Unsigned 32 bit integer
  sitkInt32,          // = PixelIDToPixelIDValue< BasicPixelID<int32_t> >::Result,   ///< Signed 32 bit integer
  sitkUInt64,         // = PixelIDToPixelIDValue< BasicPixelID<uint64_t> >::Result, ///< Unsigned 64 bit integer
  sitkInt64,          // = PixelIDToPixelIDValue< BasicPixelID<int64_t> >::Result,   ///< Signed 64 bit integer
  sitkFloat32,        // = PixelIDToPixelIDValue< BasicPixelID<float> >::Result,   ///< 32 bit float
  sitkFloat64,        // = PixelIDToPixelIDValue< BasicPixelID<double> >::Result,  ///< 64 bit float
  sitkComplexFloat32, // = PixelIDToPixelIDValue< BasicPixelID<std::complex<float> > >::Result,  ///< complex number of 32 bit float
  sitkComplexFloat64, // = PixelIDToPixelIDValue< BasicPixelID<std::complex<double> > >::Result,  ///< complex number of 64 bit float
  sitkVectorUInt8,    // = PixelIDToPixelIDValue< VectorPixelID<uint8_t> >::Result, ///< Multi-component of unsigned 8 bit integer
  sitkVectorInt8,     // = PixelIDToPixelIDValue< VectorPixelID<int8_t> >::Result, ///< Multi-component of signed 8 bit integer
  sitkVectorUInt16,   // = PixelIDToPixelIDValue< VectorPixelID<uint16_t> >::Result, ///< Multi-component of unsigned 16 bit integer
  sitkVectorInt16,    // = PixelIDToPixelIDValue< VectorPixelID<int16_t> >::Result, ///< Multi-component of signed 16 bit integer
  sitkVectorUInt32,   // = PixelIDToPixelIDValue< VectorPixelID<uint32_t> >::Result, ///< Multi-component of unsigned 32 bit integer
  sitkVectorInt32,    // = PixelIDToPixelIDValue< VectorPixelID<int32_t> >::Result, ///< Multi-component of signed 32 bit integer
  sitkVectorUInt64,   // = PixelIDToPixelIDValue< VectorPixelID<uint64_t> >::Result, ///< Multi-component of unsigned 64 bit integer
  sitkVectorInt64,    // = PixelIDToPixelIDValue< VectorPixelID<int64_t> >::Result, ///< Multi-component of signed 64 bit integer
  sitkVectorFloat32,  // = PixelIDToPixelIDValue< VectorPixelID<float> >::Result, ///< Multi-component of 32 bit float
  sitkVectorFloat64,  // = PixelIDToPixelIDValue< VectorPixelID<double> >::Result,  ///< Multi-component of 64 bit float
  sitkLabelUInt8,     // = PixelIDToPixelIDValue< LabelPixelID<uint8_t> >::Result, ///< RLE label of unsigned 8 bit integers
  sitkLabelUInt16,    // = PixelIDToPixelIDValue< LabelPixelID<uint16_t> >::Result, ///< RLE label of unsigned 16 bit integers
  sitkLabelUInt32,    // = PixelIDToPixelIDValue< LabelPixelID<uint32_t> >::Result, ///< RLE label of unsigned 32 bit integers
  sitkLabelUInt64,    // = PixelIDToPixelIDValue< LabelPixelID<uint64_t> >::Result, ///< RLE label of unsigned 64 bit integers
};
enum KernelEnum
{
  sitkAnnulus = 0,
  sitkBall = 1,
  sitkBox = 2,
  sitkCross = 3
};

enum SeedEnum
{
  /// A sentinel value used for "seed" parameters to indicate it
  /// should be initialized by the wall clock for pseudo-random behavior.
  sitkWallClock = 0
};

template <typename InputImageType, typename ContainerType>
typename InputImageType::RadiusType CastToRadiusType(const ContainerType& incoming)
{
  using RadiusType = typename InputImageType::RadiusType;
  typename InputImageType::RadiusType elementRadius;
  if(RadiusType::Dimension > 0)
  {
    elementRadius[0] = static_cast<typename RadiusType::SizeValueType>(incoming[0]);
    if(RadiusType::Dimension > 1)
    {
      elementRadius[1] = static_cast<typename RadiusType::SizeValueType>(incoming[1]);
      if(RadiusType::Dimension > 2)
      {
        elementRadius[2] = static_cast<typename RadiusType::SizeValueType>(incoming[2]);
      }
    }
  }
  return elementRadius;
}

template <unsigned int Dimension>
itk::FlatStructuringElement<Dimension> CreateKernel(KernelEnum kernelType, const std::vector<uint32_t>& size)
{
  using StructuringElementType = itk::FlatStructuringElement<Dimension>;
  using RadiusType = typename StructuringElementType::RadiusType;

  RadiusType elementRadius;
  if(RadiusType::Dimension > 0)
  {
    elementRadius[0] = static_cast<typename RadiusType::SizeValueType>(size[0]);
    if(RadiusType::Dimension > 1)
    {
      elementRadius[1] = static_cast<typename RadiusType::SizeValueType>(size[1]);
      if(RadiusType::Dimension > 2)
      {
        elementRadius[2] = static_cast<typename RadiusType::SizeValueType>(size[2]);
      }
    }
  }

  StructuringElementType structuringElement;
  switch(kernelType)
  {
  case KernelEnum::sitkAnnulus:
    structuringElement = StructuringElementType::Annulus(elementRadius, false);
    break;
  case KernelEnum::sitkBall:
    structuringElement = StructuringElementType::Ball(elementRadius, false);
    break;
  case KernelEnum::sitkBox:
    structuringElement = StructuringElementType::Box(elementRadius);
    break;
  case KernelEnum::sitkCross:
    structuringElement = StructuringElementType::Cross(elementRadius);
    break;
  default:
    break;
  }
  return structuringElement;
}
template <unsigned int Dimension>
itk::FlatStructuringElement<Dimension> CreateKernel(KernelEnum kernelType, const std::vector<float>& sizef)
{
  std::vector<uint32_t> size;
  for(const auto& f : sizef)
  {
    size.push_back(static_cast<uint32_t>(f));
  }
  using StructuringElementType = itk::FlatStructuringElement<Dimension>;
  using RadiusType = typename StructuringElementType::RadiusType;
  RadiusType elementRadius;
  if(RadiusType::Dimension > 0)
  {
    elementRadius[0] = static_cast<typename RadiusType::SizeValueType>(size[0]);
    if(RadiusType::Dimension > 1)
    {
      elementRadius[1] = static_cast<typename RadiusType::SizeValueType>(size[1]);
      if(RadiusType::Dimension > 2)
      {
        elementRadius[2] = static_cast<typename RadiusType::SizeValueType>(size[2]);
      }
    }
  }

  StructuringElementType structuringElement;
  switch(kernelType)
  {
  case 0:
    structuringElement = StructuringElementType::Annulus(elementRadius, false);
    break;
  case 1:
    structuringElement = StructuringElementType::Ball(elementRadius, false);
    break;
  case 2:
    structuringElement = StructuringElementType::Box(elementRadius);
    break;
  case 3:
    structuringElement = StructuringElementType::Cross(elementRadius);
    break;
  default:
    break;
  }
  return structuringElement;
}

} // end namespace simple
} // end namespace itk
