#include "ITKImageWriter.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "ITKImageProcessing/ITKImageProcessingPlugin.hpp"

#include <itkImageFileWriter.h>
#include <itkImageSeriesWriter.h>
#include <itkImportImageFilter.h>
#include <itkNumericSeriesFileNames.h>

#include <fmt/core.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>

#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <sstream>

namespace fs = std::filesystem;

using namespace nx::core;

namespace cxITKImageWriter
{
using ArrayOptionsType = ITK::ScalarVectorPixelIdTypeList;

bool Is2DFormat(const fs::path& fileName)
{
  fs::path ext = fileName.extension();
  auto supported2DExtensions = ITKImageProcessingPlugin::GetList2DSupportedFileExtensions();
  auto iter = std::find(supported2DExtensions.cbegin(), supported2DExtensions.cend(), ext);
  return iter != supported2DExtensions.cend();
}

template <typename PixelT, uint32 Dimensions>
Result<> WriteAsOneFile(itk::Image<PixelT, Dimensions>& image, const fs::path& filePath /*, const IFilter::MessageHandler& messanger*/)
{
  AtomicFile atomicFile(filePath.string());
  auto tempPath = atomicFile.tempFilePath();
  try
  {
    using ImageType = itk::Image<PixelT, Dimensions>;
    using FileWriterType = itk::ImageFileWriter<ImageType>;
    auto writer = FileWriterType::New();

    // messanger(fmt::format("Saving {}", fileName));

    writer->SetInput(&image);
    writer->SetFileName(tempPath.string());
    writer->UseCompressionOn();
    writer->Update();
  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult(-21011, fmt::format("ITK exception was thrown while writing output file: {}", err.GetDescription()));
  }

  if(!atomicFile.commit())
  {
    return atomicFile.getResult();
  }
  return {};
}

template <typename PixelT, uint32 Dimensions>
Result<> WriteAs2DStack(itk::Image<PixelT, Dimensions>& image, uint32 z_size, const fs::path& filePath, uint64 indexOffset)
{
  // Create list of AtomicFiles
  std::vector<std::unique_ptr<AtomicFile>> atomicFiles = {};

  for(uint64 index = indexOffset; index < (z_size - 1); index++)
  {
    atomicFiles.emplace_back(
        std::make_unique<AtomicFile>((fs::absolute(fmt::format("{}/{}{:03d}{}", filePath.parent_path().string(), filePath.stem().string(), index, filePath.extension().string())))));
  }
  {
    std::vector<std::string> fileNames = {};
    for(const auto& atomicFile : atomicFiles)
    {
      fileNames.emplace_back(atomicFile->tempFilePath().string());
    }

    // generate all the files in that new directory
    try
    {
      using InputImageType = itk::Image<PixelT, Dimensions>;
      using OutputImageType = itk::Image<PixelT, Dimensions - 1>;
      using SeriesWriterType = itk::ImageSeriesWriter<InputImageType, OutputImageType>;
      auto writer = SeriesWriterType::New();
      writer->SetInput(&image);
      writer->SetFileNames(fileNames);
      writer->UseCompressionOn();
      writer->Update();
    } catch(const itk::ExceptionObject& err)
    {
      return MakeErrorResult(-21011, fmt::format("ITK exception was thrown while writing output file: {}", err.GetDescription()));
    }
  }

  for(const auto& atomicFile : atomicFiles)
  {
    if(!atomicFile->commit())
    {
      return atomicFile->getResult();
    }
  }

  return {};
}

template <class PixelT, uint32 Dimensions>
Result<> WriteImage(IDataStore& dataStore, const ITK::ImageGeomData& imageGeom, const fs::path& filePath, uint64 indexOffset)
{
  using ImageType = itk::Image<PixelT, Dimensions>;

  auto& typedDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<PixelT>>&>(dataStore);

  typename itk::Image<PixelT, Dimensions>::Pointer image = ITK::WrapDataStoreInImage<PixelT, Dimensions>(typedDataStore, imageGeom);
  if(Is2DFormat(filePath) && Dimensions == 3)
  {
    typename ImageType::SizeType size = image->GetLargestPossibleRegion().GetSize();
    if(size[2] < 2)
    {
      return MakeErrorResult(-21012, "Image is 2D, not 3D.");
    }

    return WriteAs2DStack<PixelT, Dimensions>(*image, size[2], filePath, indexOffset);
  }
  else
  {
    return WriteAsOneFile<PixelT, Dimensions>(*image, filePath);
  }
}

template <class InputT, class OutputT, uint32 Dimensions>
struct WriteImageFunctor
{
  Result<> operator()(IDataStore& dataStore, const ITK::ImageGeomData& imageGeom, const fs::path& filePath, uint64 indexOffset) const
  {
    return WriteImage<InputT, Dimensions>(dataStore, imageGeom, filePath, indexOffset);
  }
};

template <class T>
void CopyTupleTyped(const IDataStore& currentData, IDataStore& sliceData, usize nComp, usize index, usize indexNew)
{
  const auto& currentDataTyped = dynamic_cast<const DataStore<T>&>(currentData);
  auto& sliceDataTyped = dynamic_cast<DataStore<T>&>(sliceData);

  const T* sourcePtr = currentDataTyped.data() + (nComp * index);
  T* destPtr = sliceDataTyped.data() + (nComp * indexNew);
  std::memcpy(destPtr, sourcePtr, currentData.getTypeSize() * nComp);
}

void CopyTuple(usize index, usize axisA, usize dB, usize axisB, usize nComp, const IDataStore& currentData, IDataStore& sliceData)
{
  usize indexNew = (axisA * dB) + axisB;

  DataType type = currentData.getDataType();

  switch(type)
  {
  case DataType::int8: {
    CopyTupleTyped<int8>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::uint8: {
    CopyTupleTyped<uint8>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::int16: {
    CopyTupleTyped<int16>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::uint16: {
    CopyTupleTyped<uint16>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::int32: {
    CopyTupleTyped<int32>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::uint32: {
    CopyTupleTyped<int32>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::int64: {
    CopyTupleTyped<int64>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::uint64: {
    CopyTupleTyped<int64>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::float32: {
    CopyTupleTyped<float32>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  case DataType::float64: {
    CopyTupleTyped<float64>(currentData, sliceData, nComp, index, indexNew);
    break;
  }
  default: {
    throw std::runtime_error("ITKImageWriter: Invalid DataType while attempting to copy tuples");
  }
  }
}

Result<> SaveImageData(const fs::path& filePath, IDataStore& sliceData, const ITK::ImageGeomData& imageGeom, usize slice, usize maxSlice, uint64 indexOffset)
{
  std::stringstream ss;
  ss << fs::absolute(filePath).parent_path().string() << "/" << filePath.stem().string();

  // If the parent path does not exist then try to create it.
  if(!fs::exists(fs::absolute(filePath).parent_path()))
  {
    if(!fs::create_directories(fs::absolute(filePath).parent_path()))
    {
      return {MakeErrorResult(-19000, fmt::format("Error Creating output path for image '{}'", fs::absolute(filePath).string()))};
    }
  }

  int32 totalDigits = static_cast<int32>(std::log10(maxSlice) + 1);

  if(maxSlice != 1)
  {
    ss << "_" << std::setw(totalDigits) << std::setfill('0') << slice;
  }
  ss << filePath.extension().string();

  auto fileName = fs::path(ss.str());

  return ITK::ArraySwitchFunc<WriteImageFunctor, ArrayOptionsType>(sliceData, imageGeom, -21010, sliceData, imageGeom, fileName, indexOffset);
}
} // namespace cxITKImageWriter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKImageWriter::name() const
{
  return FilterTraits<ITKImageWriter>::name;
}

//------------------------------------------------------------------------------
std::string ITKImageWriter::className() const
{
  return FilterTraits<ITKImageWriter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImageWriter::uuid() const
{
  return FilterTraits<ITKImageWriter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImageWriter::humanName() const
{
  return "Write Image (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageWriter::defaultTags() const
{
  return {className(), "io", "output", "write", "export"};
}

//------------------------------------------------------------------------------
Parameters ITKImageWriter::parameters() const
{
  Parameters params;

  using ExtensionListType = std::unordered_set<std::string>;
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane", "Selection for plane normal for writing the images (XY, XZ, or YZ)", 0, ChoicesParameter::Choices{"XY", "XZ", "YZ"}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_FileName_Key, "Output File", "Path to the output file to write.", fs::path(), ExtensionListType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<UInt64Parameter>(k_IndexOffset_Key, "Index Offset", "This is the starting index when writing mulitple images", 0));

  params.insertSeparator(Parameters::Separator{"Required Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageArrayPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes()));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageWriter::clone() const
{
  return std::make_unique<ITKImageWriter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageWriter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto filePath = filterArgs.value<fs::path>(k_FileName_Key);
  auto indexOffset = filterArgs.value<uint64>(k_IndexOffset_Key);
  auto imageArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);

  // Stored fastest to slowest i.e. X Y Z
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  // Stored slowest to fastest i.e. Z Y X
  const auto& imageArray = dataStructure.getDataRefAs<IDataArray>(imageArrayPath);

  const IDataStore& imageArrayStore = imageArray.getIDataStoreRef();

  if(!ITK::DoDimensionsMatch(imageArrayStore, imageGeom))
  {
    return {MakeErrorResult<OutputActions>(-1, "Image Array dimensions must match ImageGeometry")};
  }

  return {};
}

//------------------------------------------------------------------------------
Result<> ITKImageWriter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  auto plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto filePath = filterArgs.value<fs::path>(k_FileName_Key);
  auto indexOffset = filterArgs.value<uint64>(k_IndexOffset_Key);
  auto imageArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(imageArrayPath);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageArrayPath.toString()));
  }

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  // Stored fastest to slowest i.e. X Y Z
  SizeVec3 dims = imageGeom.getDimensions();

  const auto& imageArray = dataStructure.getDataRefAs<IDataArray>(imageArrayPath);
  usize nComp = imageArray.getNumberOfComponents();
  const IDataStore& currentData = imageArray.getIDataStoreRef();

  if(currentData.getStoreType() != IDataStore::StoreType::InMemory)
  {
    return {MakeErrorResult(-1, "DataArray must be in memory")};
  }

  std::unique_ptr<IDataStore> sliceData = currentData.createNewInstance();

  ITK::ImageGeomData newImageGeom(imageGeom);

  switch(plane)
  {
  case k_XYPlane: {
    usize dA = dims.getX();
    usize dB = dims.getY();

    newImageGeom.dims = {dims.getX(), dims.getY(), 1};

    for(usize slice = 0; slice < dims.getZ(); ++slice)
    {
      for(usize axisA = 0; axisA < dA; ++axisA)
      {
        for(usize axisB = 0; axisB < dB; ++axisB)
        {
          usize index = (slice * dA * dB) + (axisA * dB) + axisB;
          cxITKImageWriter::CopyTuple(index, axisA, dB, axisB, nComp, currentData, *sliceData);
        }
      }
      Result<> result = cxITKImageWriter::SaveImageData(filePath, *sliceData, newImageGeom, slice + indexOffset, dims.getZ(), indexOffset);
      if(result.invalid())
      {
        return result;
      }
    }
    break;
  }
  case k_XZPlane: {
    usize dA = dims.getZ();
    usize dB = dims.getX();

    newImageGeom.dims = {dims.getX(), dims.getZ(), 1};

    for(usize slice = 0; slice < dims.getY(); ++slice)
    {
      for(usize axisA = 0; axisA < dA; ++axisA)
      {
        for(usize axisB = 0; axisB < dB; ++axisB)
        {
          usize index = (dims.getY() * axisA * dB) + (slice * dB) + axisB;
          cxITKImageWriter::CopyTuple(index, axisA, dB, axisB, nComp, currentData, *sliceData);
        }
      }
      Result<> result = cxITKImageWriter::SaveImageData(filePath, *sliceData, newImageGeom, slice + indexOffset, dims.getY(), indexOffset);
      if(result.invalid())
      {
        return result;
      }
    }
    break;
  }
  case k_YZPlane: {
    usize dA = dims.getZ();
    usize dB = dims.getY();

    newImageGeom.dims = {dims.getY(), dims.getZ(), 1};

    for(usize slice = 0; slice < dims.getX(); ++slice)
    {
      for(usize axisA = 0; axisA < dA; ++axisA)
      {
        for(usize axisB = 0; axisB < dB; ++axisB)
        {
          usize index = (dims.getX() * axisA * dB) + (axisB * dims.getX()) + slice;
          cxITKImageWriter::CopyTuple(index, axisA, dB, axisB, nComp, currentData, *sliceData);
        }
      }
      Result<> result = cxITKImageWriter::SaveImageData(filePath, *sliceData, newImageGeom, slice + indexOffset, dims.getX(), indexOffset);
      if(result.invalid())
      {
        return result;
      }
    }
    break;
  }
  }

  return {};
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_PlaneKey = "Plane";
constexpr StringLiteral k_FileNameKey = "FileName";
constexpr StringLiteral k_IndexOffsetKey = "IndexOffset";
constexpr StringLiteral k_ImageArrayPathKey = "ImageArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKImageWriter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKImageWriter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_PlaneKey, k_Plane_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_FileNameKey, k_FileName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_IndexOffsetKey, k_IndexOffset_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_ImageArrayPathKey, k_ImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ImageArrayPathKey, k_ImageArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
