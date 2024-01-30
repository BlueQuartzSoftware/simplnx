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

#include "simplnx/Common/Types.hpp"

#include <fmt/format.h>

#include <memory>
#include <vector>

using namespace nx::core;

namespace
{
using ShapeType = std::vector<usize>;

template <class T>
std::string ComputeMD5HashTyped(const IDataArray& outputDataArray)
{
  const auto& dataArray = dynamic_cast<const DataArray<T>&>(outputDataArray);
  const T* dataPtr = dataArray.template getIDataStoreRefAs<DataStore<T>>().data();
  usize arraySize = dataArray.getSize();
  MD5 md5;
  md5.update(reinterpret_cast<const uint8*>(dataPtr), arraySize * sizeof(T));
  md5.finalize();
  return md5.hexdigest();
}

template <class PixelType>
float64 ComputeDiff(const itk::Vector<PixelType, 2>& p1, const itk::Vector<PixelType, 2>& p2)
{
  float64 diff = static_cast<float64>((p1 - p2).GetNorm());
  return diff;
}

template <class PixelType>
float64 ComputeDiff(const itk::Vector<PixelType, 3>& p1, const itk::Vector<PixelType, 3>& p2)
{
  float64 diff = static_cast<float64>((p1 - p2).GetNorm());
  return diff;
}

template <class PixelType>
float64 ComputeDiff(const itk::RGBAPixel<PixelType>& p1, const itk::RGBAPixel<PixelType>& p2)
{
  float64 diff = static_cast<float64>((p1 - p2).GetScalarValue());
  return diff;
}

template <class PixelType>
float64 ComputeDiff(const itk::RGBPixel<PixelType>& p1, const itk::RGBPixel<PixelType>& p2)
{
  float64 diff = static_cast<float64>((p1 - p2).GetScalarValue());
  return diff;
}

template <class PixelType>
float64 ComputeDiff(PixelType p1, PixelType p2)
{
  return static_cast<float64>(p1 - p2);
}

template <class PixelType, uint32 Dimensions>
Result<> CompareImagesImpl(const ImageGeom& inputImageGeom, const IDataArray& outputDataArray, const ImageGeom& baselineImageGeom, const IDataArray& baselineDataArray, float64 tolerance)
{
  using DataArrayType = DataArray<PixelType>;

  const auto& outputArray = dynamic_cast<const DataArrayType&>(outputDataArray);
  const auto& baselineArray = dynamic_cast<const DataArrayType&>(baselineDataArray);

  float64 largestError = 0.0;

  usize numElements = outputArray.getSize();
  for(usize idx = 0; idx < numElements; idx++)
  {
    PixelType outputValue = outputArray[idx];
    PixelType baselineValue = baselineArray[idx];
    float64 diff = ComputeDiff(outputValue, baselineValue);
    diff = (diff > 0 ? diff : -diff);
    largestError = std::max(diff, largestError);
  }
  if(largestError > tolerance)
  {
    return MakeErrorResult(-20, fmt::format("Comparing output image and baseline image produced too large of an error. Tolerance was: {}. Error was {}", tolerance, largestError));
  }
  return {};
}

template <class PixelType>
Result<> CompareImagesTyped(const ImageGeom& inputImageGeom, const IDataArray& outputDataArray, const ImageGeom& baselineImageGeom, const IDataArray& baselineDataArray, float64 tolerance)
{
  SizeVec3 inputDims = inputImageGeom.getDimensions();
  if(inputDims[2] == 1)
  {
    // 2D image
    return CompareImagesImpl<PixelType, 2>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }

  {
    // 3D image
    return CompareImagesImpl<PixelType, 3>(inputImageGeom, outputDataArray, baselineImageGeom, baselineDataArray, tolerance);
  }
}
} // namespace

namespace nx::core
{
namespace ITKTestBase
{
//------------------------------------------------------------------------------
std::string ComputeMd5Hash(DataStructure& dataStructure, const DataPath& outputDataPath)
{
  const auto& outputDataArray = dataStructure.getDataRefAs<IDataArray>(outputDataPath);
  DataType outputDataType = outputDataArray.getDataType();

  switch(outputDataType)
  {
  case DataType::float32: {
    return ComputeMD5HashTyped<float32>(outputDataArray);
  }
  case DataType::float64: {
    return ComputeMD5HashTyped<float64>(outputDataArray);
  }
  case DataType::int8: {
    return ComputeMD5HashTyped<int8>(outputDataArray);
  }
  case DataType::uint8: {
    return ComputeMD5HashTyped<uint8>(outputDataArray);
  }
  case DataType::int16: {
    return ComputeMD5HashTyped<int16>(outputDataArray);
  }
  case DataType::uint16: {
    return ComputeMD5HashTyped<uint16>(outputDataArray);
  }
  case DataType::int32: {
    return ComputeMD5HashTyped<int32>(outputDataArray);
  }
  case DataType::uint32: {
    return ComputeMD5HashTyped<uint32>(outputDataArray);
  }
  case DataType::int64: {
    return ComputeMD5HashTyped<int64>(outputDataArray);
  }
  case DataType::uint64: {
    return ComputeMD5HashTyped<uint64>(outputDataArray);
  }
  case DataType::boolean: {
    [[fallthrough]];
  }
  default: {
    return {};
  }
  }
}

//------------------------------------------------------------------------------
Result<> ReadImage(DataStructure& dataStructure, const fs::path& filePath, const DataPath& geometryPath, const DataPath& cellDataPath, const DataPath& imagePath)
{
  ITKImageReader filter;
  Arguments args;
  args.insertOrAssign(ITKImageReader::k_FileName_Key, filePath);
  args.insertOrAssign(ITKImageReader::k_ImageGeometryPath_Key, geometryPath);
  args.insertOrAssign(ITKImageReader::k_CellDataName_Key, cellDataPath.getTargetName());
  args.insertOrAssign(ITKImageReader::k_ImageDataArrayPath_Key, imagePath);
  args.insertOrAssign(ITKImageReader::k_ChangeOrigin_Key, false);
  args.insertOrAssign(ITKImageReader::k_ChangeSpacing_Key, false);
  auto executeResult = filter.execute(dataStructure, args);
  return executeResult.result;
}

//------------------------------------------------------------------------------
Result<> WriteImage(DataStructure& dataStructure, const fs::path& filePath, const DataPath& geometryPath, const DataPath& imagePath)
{
  ITKImageWriter filter;
  Arguments args;

  args.insertOrAssign(ITKImageWriter::k_ImageGeomPath_Key, std::make_any<DataPath>(geometryPath));
  args.insertOrAssign(ITKImageWriter::k_ImageArrayPath_Key, std::make_any<DataPath>(imagePath));
  args.insertOrAssign(ITKImageWriter::k_FileName_Key, std::make_any<fs::path>(filePath));
  args.insertOrAssign(ITKImageWriter::k_IndexOffset_Key, std::make_any<uint64>(0));
  args.insertOrAssign(ITKImageWriter::k_Plane_Key, std::make_any<uint64>(ITKImageWriter::k_XYPlane));

  auto executeResult = filter.execute(dataStructure, args);

  return executeResult.result;
}

//------------------------------------------------------------------------------
Result<> CompareImages(DataStructure& dataStructure, const DataPath& baselineGeometryPath, const DataPath& baselineDataPath, const DataPath& inputGeometryPath, const DataPath& outputDataPath,
                       float64 tolerance)
{
  const auto* baselineImageGeom = dataStructure.getDataAs<ImageGeom>(baselineGeometryPath);
  if(baselineImageGeom == nullptr)
  {
    return MakeErrorResult(-10, fmt::format("Could not get ImageGeometry for Baseline"));
  }
  SizeVec3 baselineDims = baselineImageGeom->getDimensions();

  const auto* baselineDataArray = dataStructure.getDataAs<IDataArray>(baselineDataPath);
  if(baselineDataArray == nullptr)
  {
    return MakeErrorResult(-11, fmt::format("Could not get DataArray for Baseline"));
  }
  DataType baseLineDataType = baselineDataArray->getDataType();

  const auto* inputImageGeom = dataStructure.getDataAs<ImageGeom>(inputGeometryPath);
  if(inputImageGeom == nullptr)
  {
    return MakeErrorResult(-12, fmt::format("Could not get ImageGeometry for Output"));
  }
  SizeVec3 outputDims = inputImageGeom->getDimensions();

  const auto* outputDataArray = dataStructure.getDataAs<IDataArray>(outputDataPath);
  if(outputDataArray == nullptr)
  {
    return MakeErrorResult(-13, fmt::format("Could not get DataArray for Output"));
  }
  DataType outputDataType = outputDataArray->getDataType();
  // Make sure the data types are the same
  if(baseLineDataType != outputDataType)
  {
    return MakeErrorResult(-14, fmt::format("DataTypes do not match. Output: {} Baseline: {}", fmt::underlying(outputDataType), fmt::underlying(baseLineDataType)));
  }
  // Make sure the geometry dimensions are the same
  if(baselineDims != outputDims)
  {
    return MakeErrorResult(-15, fmt::format("Image Dimensions do not match. Output: {} Baseline: {}", fmt::join(outputDims, ", "), fmt::join(baselineDims, ", ")));
  }
  // Make sure the tuple shape is the same
  ShapeType baselineTupleShape = baselineDataArray->getIDataStoreRef().getTupleShape();
  ShapeType outputTupleShape = outputDataArray->getIDataStoreRef().getTupleShape();
  if(baselineTupleShape != outputTupleShape)
  {
    return MakeErrorResult(-16, fmt::format("Tuple Shape does not Match. Output: {} Baseline: {}", fmt::join(outputTupleShape, ", "), fmt::join(baselineTupleShape, ", ")));
  }

  // Make sure the component shape is the same
  ShapeType baselineComponentShape = baselineDataArray->getIDataStoreRef().getComponentShape();
  ShapeType outputComponentShape = outputDataArray->getIDataStoreRef().getComponentShape();
  if(baselineComponentShape != outputComponentShape)
  {
    return MakeErrorResult(-18, fmt::format("Component Shape does not Match. Output: {} Baseline: {}", fmt::join(outputComponentShape, ", "), fmt::join(baselineComponentShape, ", ")));
  }

  switch(outputDataType)
  {
  case DataType::float32: {
    return CompareImagesTyped<float32>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::float64: {
    return CompareImagesTyped<float64>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::int8: {
    return CompareImagesTyped<int8>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::uint8: {
    return CompareImagesTyped<uint8>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::int16: {
    return CompareImagesTyped<int16>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::uint16: {
    return CompareImagesTyped<uint16>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::int32: {
    return CompareImagesTyped<int32>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::uint32: {
    return CompareImagesTyped<uint32>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::int64: {
    return CompareImagesTyped<int64>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::uint64: {
    return CompareImagesTyped<uint64>(*inputImageGeom, *outputDataArray, *baselineImageGeom, *baselineDataArray, tolerance);
  }
  case DataType::boolean: {
    [[fallthrough]];
  }
  default: {
    return MakeErrorResult(-100, "");
  }
  }
}

//------------------------------------------------------------------------------
void RemoveFiles(const fs::path& dirPath, const std::string& filePattern)
{
  for(const auto& entry : fs::directory_iterator(dirPath))
  {
    std::string fileName = entry.path().filename().string();
    if(fileName.find(filePattern) == 0)
    {
      fs::remove(entry.path());
    }
  }
}
} // namespace ITKTestBase
} // namespace nx::core
