#include "ITKTestBase.hpp"

#include "ITKImageProcessing/Filters/ITKImageReader.hpp"
#include "ITKImageProcessing/Filters/ITKImageWriter.hpp"
#include "MD5.hpp"

#include <itkImportImageFilter.h>
#include <itkNumericTraits.h>
#include <itkNumericTraitsRGBAPixel.h>
#include <itkNumericTraitsRGBPixel.h>
#include <itkNumericTraitsVectorPixel.h>
#include <itkTestingHashImageFilter.h>

#include "complex/Common/Types.hpp"

#include <fmt/format.h>

#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace complex
{
namespace ITKTestBase
{

using DatObjectPtr = std::shared_ptr<DataObject>;
using ImageGeomPtr = std::shared_ptr<ImageGeom>;
using IDataArrayPtr = std::shared_ptr<IDataArray>;
using ShapeType = typename std::vector<size_t>;

template <typename T>
std::string ComputeMD5Hash(IDataArrayPtr& outputDataArray)
{
  using DataArrayType = DataArray<T>;
  using DataArrayPtrType = std::shared_ptr<DataArrayType>;
  using DataStoreType = DataStore<T>;
  DataArrayPtrType dataArray = std::dynamic_pointer_cast<DataArrayType>(outputDataArray);
  T* dataPtr = dataArray->template getIDataStoreAs<DataStoreType>()->data();
  size_t arraySize = dataArray->getSize();
  MD5 md5;
  md5.update(reinterpret_cast<uint8_t*>(dataPtr), arraySize * sizeof(T));
  md5.finalize();
  return md5.hexdigest();
}

std::string ComputeMd5Hash(DataStructure& ds, const DataPath& outputDataPath)
{
  IDataArrayPtr outputDataArray = ds.getSharedDataAs<IDataArray>(outputDataPath);
  complex::DataType outputDataType = outputDataArray->getIDataStore()->getDataType();

  switch(outputDataType)
  {
  case complex::DataType::float32: {
    return ComputeMD5Hash<float>(outputDataArray);
  }
  case complex::DataType::float64: {
    return ComputeMD5Hash<double>(outputDataArray);
  }
  case complex::DataType::int8: {
    return ComputeMD5Hash<int8_t>(outputDataArray);
  }
  case complex::DataType::uint8: {
    return ComputeMD5Hash<uint8_t>(outputDataArray);
  }
  case complex::DataType::int16: {
    return ComputeMD5Hash<int16_t>(outputDataArray);
  }
  case complex::DataType::uint16: {
    return ComputeMD5Hash<uint16_t>(outputDataArray);
  }
  case complex::DataType::int32: {
    return ComputeMD5Hash<int32_t>(outputDataArray);
  }
  case complex::DataType::uint32: {
    return ComputeMD5Hash<uint32_t>(outputDataArray);
  }
  case complex::DataType::int64: {
    return ComputeMD5Hash<int64_t>(outputDataArray);
  }
  case complex::DataType::uint64: {
    return ComputeMD5Hash<uint64_t>(outputDataArray);
  }
  case complex::DataType::boolean:
  case complex::DataType::error: {
    return {};
  }
  }

  return {};
}

Result<> ReadImage(DataStructure& ds, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath)
{
  ITKImageReader filter;
  Arguments args;
  args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
  args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, geometryPath);
  args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, imagePath);
  auto executeResult = filter.execute(ds, args);
  return executeResult.result;
}

int32_t WriteImage(DataStructure& ds, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath)
{
  ITKImageWriter filter;
  Arguments args;

  args.insertOrAssign(ITKImageWriter::k_ImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(ITKImageWriter::k_ImageArrayPath_Key, std::make_any<DataPath>(imagePath));
  args.insertOrAssign(ITKImageWriter::k_FileName_Key, std::make_any<fs::path>(filePath));
  args.insertOrAssign(ITKImageWriter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriter::k_Plane_Key, std::make_any<uint64>(ITKImageWriter::k_XYPlane));

  auto preflightResult = filter.preflight(ds, args);
  if(preflightResult.outputActions.invalid())
  {
    for(const auto& error : preflightResult.outputActions.errors())
    {
      std::cout << error.code << ": " << error.message << std::endl;
    }
    return -1;
  }
  auto executeResult = filter.execute(ds, args);
  if(executeResult.result.invalid())
  {
    for(const auto& error : executeResult.result.errors())
    {
      std::cout << error.code << ": " << error.message << std::endl;
    }
    return -2;
  }

  return 0;
}

template <typename PixelType>
double ComputeDiff(itk::Vector<PixelType, 2> p1, itk::Vector<PixelType, 2> p2)
{
  double diff = static_cast<double>((p1 - p2).GetNorm());
  return diff;
}

template <typename PixelType>
double ComputeDiff(itk::Vector<PixelType, 3> p1, itk::Vector<PixelType, 3> p2)
{
  double diff = static_cast<double>((p1 - p2).GetNorm());
  return diff;
}

template <typename PixelType>
double ComputeDiff(itk::RGBAPixel<PixelType> p1, itk::RGBAPixel<PixelType> p2)
{
  double diff = static_cast<double>((p1 - p2).GetScalarValue());
  return diff;
}

template <typename PixelType>
double ComputeDiff(itk::RGBPixel<PixelType> p1, itk::RGBPixel<PixelType> p2)
{
  double diff = static_cast<double>((p1 - p2).GetScalarValue());
  return diff;
}

template <typename PixelType>
double ComputeDiff(PixelType p1, PixelType p2)
{
  return static_cast<double>(p1 - p2);
}

template <typename PixelType, unsigned int Dimensions>
Result<> CompareImages(ImageGeomPtr& inputImageGeom, IDataArrayPtr& outputDataArray, ImageGeomPtr& baselineImageGeom, IDataArrayPtr& baselineDataArray, double tolerance)
{
  using DataArrayType = DataArray<PixelType>;
  using DataArrayTypePtr = std::shared_ptr<DataArrayType>;

  DataArrayTypePtr outputArray = std::dynamic_pointer_cast<DataArrayType>(outputDataArray);
  DataArrayTypePtr baselineArray = std::dynamic_pointer_cast<DataArrayType>(baselineDataArray);

  double largest_error = 0.0;
  double diff;

  size_t numElements = outputArray->getSize();
  for(size_t idx = 0; idx < numElements; idx++)
  {
    PixelType outputValue = (*outputArray)[idx];
    PixelType baselineValue = (*baselineArray)[idx];
    diff = ComputeDiff(outputValue, baselineValue);
    diff = (diff > 0 ? diff : -diff);
    largest_error = std::max(diff, largest_error);
  }
  if(largest_error > tolerance)
  {
    return MakeErrorResult(-20, fmt::format("Comparing output image and baseline image produced too large of an error. Tolerance was: {}. Error was {}", tolerance, largest_error));
  }
  return {};
}

template <typename PixelType, unsigned int Dimensions>
Result<> CompareOutputToBaseline(ImageGeomPtr& inputImageGeom, IDataArrayPtr& outputDataArray, ImageGeomPtr& baselineImageGeom, IDataArrayPtr& baselineDataArray, double tolerance)
{
  ShapeType cDims = outputDataArray->getIDataStore()->getComponentShape();
  // Vector images
  if(cDims.size() > 1)
  {
    if(cDims.size() == 2)
    {
      return CompareOutputToBaseline<itk::Vector<PixelType, 2>, Dimensions>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
    }
    if(cDims.size() == 3)
    {
      return CompareOutputToBaseline<itk::Vector<PixelType, 3>, Dimensions>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
    }
    return MakeErrorResult(-22, fmt::format("Vector Size of {} is not supported.", cDims.size()));
  }
  {
    // Scalar images
    if(cDims[0] == 1)
    {
      return CompareOutputToBaseline<PixelType, Dimensions>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
    }
    // RGB images
    if(cDims[0] == 3)
    {
      return CompareOutputToBaseline<itk::RGBPixel<PixelType>, Dimensions>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
    }
    // RGBA images
    if(cDims[0] == 4)
    {
      return CompareOutputToBaseline<itk::RGBAPixel<PixelType>, Dimensions>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
    }
    return MakeErrorResult(-23, fmt::format("Number of components {} not supported.", cDims[0]));
  }
}

template <typename PixelType>
Result<> CompareImages(ImageGeomPtr& inputImageGeom, IDataArrayPtr& outputDataArray, ImageGeomPtr& baselineImageGeom, IDataArrayPtr& baselineDataArray, double tolerance)
{
  SizeVec3 inputDims = inputImageGeom->getDimensions();
  if(inputDims[2] == 1)
  {
    /* 2D image */
    return CompareImages<PixelType, 2>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }

  {
    /* 3D */
    return CompareImages<PixelType, 3>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
}

Result<> CompareImages(DataStructure& ds, const DataPath& baselineGeometryPath, const DataPath& baselineDataPath, const DataPath& inputGeometryPath, const DataPath& outputDataPath, double tolerance)
{
  DatObjectPtr sharedDo = ds.getSharedData(baselineGeometryPath);
  ImageGeomPtr baselineImageGeom = std::dynamic_pointer_cast<ImageGeom>(sharedDo);
  if(baselineImageGeom.get() == nullptr)
  {
    return MakeErrorResult(-10, fmt::format("Could not cast DataObject to ImageGeometry for Baseline"));
  }
  complex::SizeVec3 baselineDims = baselineImageGeom->getDimensions();

  sharedDo = ds.getSharedData(baselineDataPath);
  IDataArrayPtr baselineDataArray = std::dynamic_pointer_cast<IDataArray>(sharedDo);
  if(baselineDataArray.get() == nullptr)
  {
    return MakeErrorResult(-11, fmt::format("Could not cast DataObject to DataArray for Baseline"));
  }
  complex::DataType baseLineDataType = baselineDataArray->getIDataStore()->getDataType();

  sharedDo = ds.getSharedData(inputGeometryPath);
  ImageGeomPtr inputImageGeom = std::dynamic_pointer_cast<ImageGeom>(sharedDo);
  if(inputImageGeom.get() == nullptr)
  {
    return MakeErrorResult(-12, fmt::format("Could not cast DataObject to ImageGeometry for Output"));
  }
  complex::SizeVec3 outputDims = inputImageGeom->getDimensions();

  sharedDo = ds.getSharedData(outputDataPath);
  IDataArrayPtr outputDataArray = std::dynamic_pointer_cast<IDataArray>(sharedDo);
  if(outputDataArray.get() == nullptr)
  {
    return MakeErrorResult(-13, fmt::format("Could not cast DataObject to DataArray for Output"));
  }
  complex::DataType outputDataType = outputDataArray->getIDataStore()->getDataType();
  // Make sure the data types are the same
  if(baseLineDataType != outputDataType)
  {
    return MakeErrorResult(-14, fmt::format("DataTypes do not match. Output: {} Baseline: {}", outputDataType, baseLineDataType));
  }
  // Make sure the geometry dimensions are the same
  if(baselineDims != outputDims)
  {
    return MakeErrorResult(-15, fmt::format("Image Dimensions do not match. Output: {} Baseline: {}", fmt::join(outputDims, ", "), fmt::join(baselineDims, ", ")));
  }
  // Make sure the tuple shape is the same
  ShapeType baselineTupleShape = baselineDataArray->getIDataStore()->getTupleShape();
  ShapeType outputTupleShape = outputDataArray->getIDataStore()->getTupleShape();
  if(baselineTupleShape != outputTupleShape)
  {
    return MakeErrorResult(-16, fmt::format("Tuple Shape does not Match. Output: {} Baseline: {}", fmt::join(outputTupleShape, ", "), fmt::join(baselineTupleShape, ", ")));
  }

  // Make sure the component shape is the same
  ShapeType baselineComponentShape = baselineDataArray->getIDataStore()->getComponentShape();
  ShapeType outputComponentShape = outputDataArray->getIDataStore()->getComponentShape();
  if(baselineComponentShape != outputComponentShape)
  {
    return MakeErrorResult(-18, fmt::format("Component Shape does not Match. Output: {} Baseline: {}", fmt::join(outputComponentShape, ", "), fmt::join(baselineComponentShape, ", ")));
  }

  switch(outputDataType)
  {
  case complex::DataType::float32: {
    return CompareImages<float>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::float64: {
    return CompareImages<double>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::int8: {
    return CompareImages<int8_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::uint8: {
    return CompareImages<uint8_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::int16: {
    return CompareImages<int16_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::uint16: {
    return CompareImages<uint16_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::int32: {
    return CompareImages<int32_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::uint32: {
    return CompareImages<uint32_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::int64: {
    return CompareImages<int64_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::uint64: {
    return CompareImages<uint64_t>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
  case complex::DataType::boolean: {
    [[fallthrough]];
  }
  case complex::DataType::error: {
    [[fallthrough]];
  }
  default: {
    return MakeErrorResult(-100, fmt::format(""));
  }
  }
}

void RemoveFiles(fs::path& dirPath, const std::string& filePattern)
{
  for(auto& p : fs::directory_iterator(dirPath))
  {
    std::string file_name = p.path().filename().string();
    if(file_name.find(filePattern) == 0)
    {
      // std::cout << "Removing: " << file_name  <<std::endl;
      fs::remove(p.path());
    }
  }
}

} // namespace ITKTestBase
} // namespace complex
