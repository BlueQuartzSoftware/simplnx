#include "ITKImageWriterFilter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"

#include "simplnx/Common/AtomicFile.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/ImageIO/ImageIOUtilities.hpp"
#include "simplnx/Utilities/StringUtilities.hpp"

#include <itkImageFileWriter.h>

#include <fmt/core.h>

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <iomanip>
#include <sstream>

#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace fs = std::filesystem;

using namespace nx::core;

namespace cxITKImageWriterFilter
{
using ArrayOptionsType = ITK::ScalarVectorPixelIdTypeList;
using RgbRgbaArrayOptionsType = ITK::ArrayOptions<ITK::ArrayComponentOptions<true, false, true>, ITK::ArrayUseAllTypes>;

// Rejects 0 - 31 ASCII control characters
bool IsValidFillCharacter(char fillCharacter)
{
  return !(fillCharacter >= 0 && fillCharacter <= 31) && fillCharacter != '{' && fillCharacter != '}' && fillCharacter != '\\' && fillCharacter != '/' && fillCharacter != ':' &&
         fillCharacter != '*' && fillCharacter != '?' && fillCharacter != '"' && fillCharacter != '<' && fillCharacter != '>' && fillCharacter != '|';
}

template <typename PixelT, uint32 Dimensions>
Result<> WriteAsOneFile(itk::Image<PixelT, Dimensions>& image, const fs::path& filePath)
{
  auto atomicFileResult = AtomicFile::Create(filePath);
  if(atomicFileResult.invalid())
  {
    return ConvertResult(std::move(atomicFileResult));
  }
  AtomicFile atomicFile = std::move(atomicFileResult.value());
  std::string tempPath = atomicFile.tempFilePath().string();
  try
  {
    using ImageType = itk::Image<PixelT, Dimensions>;
    using FileWriterType = itk::ImageFileWriter<ImageType>;
    auto writer = FileWriterType::New();

    writer->SetInput(&image);
    writer->SetFileName(tempPath);
    writer->UseCompressionOn();
    writer->Update();
  } catch(const itk::ExceptionObject& err)
  {
    return MakeErrorResult(-21011, fmt::format("ITK exception was thrown while writing output file: {}", err.GetDescription()));
  }

  Result<> commitResult = atomicFile.commit();
  if(commitResult.invalid())
  {
    return commitResult;
  }
  return {};
}

template <class PixelT, uint32 Dimensions>
Result<> WriteImage(IDataStore& dataStore, const ITK::ImageGeomData& imageGeom, const fs::path& filePath)
{
  auto& typedDataStore = dynamic_cast<DataStore<ITK::UnderlyingType_t<PixelT>>&>(dataStore);

  typename itk::Image<PixelT, Dimensions>::Pointer image = ITK::WrapDataStoreInImage<PixelT, Dimensions>(typedDataStore, imageGeom);
  return WriteAsOneFile<PixelT, Dimensions>(*image, filePath);
}

template <class InputT, class OutputT, uint32 Dimensions>
struct WriteImageFunctor
{
  Result<> operator()(IDataStore& dataStore, const ITK::ImageGeomData& imageGeom, const fs::path& filePath) const
  {
    return WriteImage<InputT, Dimensions>(dataStore, imageGeom, filePath);
  }
};

fs::path GenerateOutputFilePath(const fs::path& filePath, usize slice, usize maxSlice, int32 totalDigits, const std::string& fillChar)
{
  std::stringstream ss;
  ss << fs::absolute(filePath).parent_path().string() << "/" << filePath.stem().string();

  if(maxSlice != 1)
  {
    ss << "_" << std::setw(totalDigits) << std::setfill(fillChar[0]) << slice;
  }
  ss << filePath.extension().string();

  return fs::path(ss.str());
}

Result<> SaveImageData(const fs::path& fileName, IDataStore& sliceData, const ITK::ImageGeomData& imageGeom)
{
  // If the parent path does not exist then try to create it.
  if(!fs::exists(fileName.parent_path()))
  {
    if(!fs::create_directories(fileName.parent_path()))
    {
      return MakeErrorResult(-19000, fmt::format("Error Creating output path for image '{}'", fileName.string()));
    }
  }

  if(sliceData.getNumberOfComponents() == 4)
  {
    return ITK::ArraySwitchFunc<WriteImageFunctor, RgbRgbaArrayOptionsType>(sliceData, imageGeom, -21010, sliceData, imageGeom, fileName);
  }
  return ITK::ArraySwitchFunc<WriteImageFunctor, ArrayOptionsType>(sliceData, imageGeom, -21010, sliceData, imageGeom, fileName);
}

struct CopyXYSlicesFunctor
{
  template <class T>
  Result<> operator()(usize dA, usize dB, usize slice, const IDataStore& currentData, IDataStore& sliceData) const
  {
    const auto& currentDataTyped = dynamic_cast<const AbstractDataStore<T>&>(currentData);
    auto& sliceDataTyped = dynamic_cast<AbstractDataStore<T>&>(sliceData);

    for(usize axisA = 0; axisA < dA; ++axisA)
    {
      for(usize axisB = 0; axisB < dB; ++axisB)
      {
        usize index = (slice * dA * dB) + (axisA * dB) + axisB;
        usize indexNew = (axisA * dB) + axisB;
        if(Result<> copyResult = sliceDataTyped.copyFrom(indexNew, currentDataTyped, index, 1); copyResult.invalid())
        {
          return copyResult;
        }
      }
    }

    return {};
  }
};

struct CopyXZSlicesFunctor
{
  template <class T>
  Result<> operator()(usize dA, usize dB, usize slice, const SizeVec3& dims, const IDataStore& currentData, IDataStore& sliceData) const
  {
    const auto& currentDataTyped = dynamic_cast<const AbstractDataStore<T>&>(currentData);
    auto& sliceDataTyped = dynamic_cast<AbstractDataStore<T>&>(sliceData);

    for(usize axisA = 0; axisA < dA; ++axisA)
    {
      for(usize axisB = 0; axisB < dB; ++axisB)
      {
        usize index = (dims.getY() * axisA * dB) + (slice * dB) + axisB;
        usize indexNew = (axisA * dB) + axisB;
        if(Result<> copyResult = sliceDataTyped.copyFrom(indexNew, currentDataTyped, index, 1); copyResult.invalid())
        {
          return copyResult;
        }
      }
    }
    return {};
  }
};

struct CopyYZSlicesFunctor
{
  template <class T>
  Result<> operator()(usize dA, usize dB, usize slice, const SizeVec3& dims, const IDataStore& currentData, IDataStore& sliceData) const
  {
    const auto& currentDataTyped = dynamic_cast<const AbstractDataStore<T>&>(currentData);
    auto& sliceDataTyped = dynamic_cast<AbstractDataStore<T>&>(sliceData);

    for(usize axisA = 0; axisA < dA; ++axisA)
    {
      for(usize axisB = 0; axisB < dB; ++axisB)
      {
        usize index = (dims.getX() * axisA * dB) + (axisB * dims.getX()) + slice;
        usize indexNew = (axisA * dB) + axisB;
        if(Result<> copyResult = sliceDataTyped.copyFrom(indexNew, currentDataTyped, index, 1); copyResult.invalid())
        {
          return copyResult;
        }
      }
    }
    return {};
  }
};

struct CreateDataStoreFunctor
{
  template <class T>
  std::unique_ptr<IDataStore> operator()(const ShapeType& sliceShape, const ShapeType& cDims) const
  {
    return std::make_unique<DataStore<T>>(sliceShape, cDims, static_cast<T>(0));
  }
};
} // namespace cxITKImageWriterFilter

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKImageWriterFilter::name() const
{
  return FilterTraits<ITKImageWriterFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKImageWriterFilter::className() const
{
  return FilterTraits<ITKImageWriterFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImageWriterFilter::uuid() const
{
  return FilterTraits<ITKImageWriterFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImageWriterFilter::humanName() const
{
  return "Write Image (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageWriterFilter::defaultTags() const
{
  return {className(), "io", "output", "write", "export"};
}

//------------------------------------------------------------------------------
Parameters ITKImageWriterFilter::parameters() const
{
  Parameters params;

  using ExtensionListType = std::unordered_set<std::string>;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<ChoicesParameter>(k_Plane_Key, "Plane", "Selection for plane normal for writing the images (XY, XZ, or YZ)", 0, ChoicesParameter::Choices{"XY", "XZ", "YZ"}));
  params.insert(
      std::make_unique<FileSystemPathParameter>(k_FileName_Key, "Output File", "Path to the output file to write.", fs::path(), ExtensionListType{}, FileSystemPathParameter::PathType::OutputFile));
  params.insert(std::make_unique<UInt64Parameter>(k_IndexOffset_Key, "Index Offset", "This is the starting index when writing multiple images", 0));
  params.insert(std::make_unique<Int32Parameter>(k_TotalIndexDigits_Key, "Total Number of Index Digits", "This is the total number of digits to use when generating the index", 3));
  params.insert(std::make_unique<StringParameter>(k_LeadingDigitCharacter_Key, "Fill Character", "The character to use for the leading digits if needed", "0"));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_ImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_ImageArrayPath_Key, "Input Image Data Array", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ITK::GetScalarPixelAllowedTypes(), ArraySelectionParameter::AllowedComponentShapes{{1}, {2}, {3}, {4}, {10}, {11}, {36}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ITKImageWriterFilter::parametersVersion() const
{
  // (1) Original Version of the filter
  // (2) Added Output indexing parameters
  return 2;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageWriterFilter::clone() const
{
  return std::make_unique<ITKImageWriterFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageWriterFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto filePath = filterArgs.value<fs::path>(k_FileName_Key);
  auto indexOffset = filterArgs.value<uint64>(k_IndexOffset_Key);
  auto imageArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);

  auto totalDigits = filterArgs.value<int32>(k_TotalIndexDigits_Key);
  auto fillChar = filterArgs.value<StringParameter::ValueType>(k_LeadingDigitCharacter_Key);

  // Stored fastest to slowest i.e. X Y Z
  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  // Stored slowest to fastest i.e. Z Y X
  const auto& imageArray = dataStructure.getDataRefAs<IDataArray>(imageArrayPath);

  const IDataStore& imageArrayStore = imageArray.getIDataStoreRef();

  if(!nx::core::DoDimensionsMatch(imageArrayStore, imageGeom))
  {
    return {MakeErrorResult<OutputActions>(-25600, fmt::format("Image array '{}' dimensions ({}) do not match image geometry '{}' dimensions ({}).", imageArrayPath.toString(),
                                                               StringUtilities::formatTupleShape3D(imageArray.getTupleShape()), imageGeomPath.toString(),
                                                               StringUtilities::formatDimensions3D(imageGeom.getDimensions())))};
  }

  if(fillChar.size() != 1)
  {
    return {MakeErrorResult<OutputActions>(-25601, fmt::format("The fill character must contain exactly one character; received {} characters.", fillChar.size()))};
  }
  if(!cxITKImageWriterFilter::IsValidFillCharacter(fillChar.at(0)))
  {
    return {MakeErrorResult<OutputActions>(-25602, fmt::format("The fill character '{}' is not valid for format strings and file names.", fillChar))};
  }
  if(!imageArray.getDataFormat().empty())
  {
    return {MakeErrorResult<OutputActions>(ITK::Constants::k_OutOfCoreDataNotSupported,
                                           fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageArrayPath.toString()))};
  }

  Result<OutputActions> resultOutputActions;
  std::vector<PreflightValue> preflightUpdatedValues;

  auto imageGeomDims = imageGeom.getDimensions();
  usize maxSlice = 1;
  switch(plane)
  {
  case k_XYPlane:
    maxSlice = imageGeomDims[2];
    break;
  case k_XZPlane:
    maxSlice = imageGeomDims[1];
    break;
  case k_YZPlane:
    maxSlice = imageGeomDims[0];
    break;
  default:
    break;
  }

  // Generate example filename for PreflightValues
  const std::string indexStr = maxSlice == 1 ? "" : fmt::format("_{}", CreateIndexString(indexOffset, static_cast<usize>(totalDigits), fillChar));
  const std::string exampleFileName = (fs::absolute(filePath).parent_path() / fmt::format("{}{}{}", filePath.stem().string(), indexStr, filePath.extension().string())).string();

  preflightUpdatedValues.push_back({"Example Output File", exampleFileName});

  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> ITKImageWriterFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto plane = filterArgs.value<ChoicesParameter::ValueType>(k_Plane_Key);
  auto filePath = filterArgs.value<fs::path>(k_FileName_Key);
  auto indexOffset = filterArgs.value<uint64>(k_IndexOffset_Key);
  auto imageArrayPath = filterArgs.value<DataPath>(k_ImageArrayPath_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeomPath_Key);

  auto totalDigits = filterArgs.value<int32>(k_TotalIndexDigits_Key);
  auto fillChar = filterArgs.value<StringParameter::ValueType>(k_LeadingDigitCharacter_Key);

  const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeomPath);
  // Stored fastest to slowest i.e. X Y Z
  SizeVec3 dims = imageGeom.getDimensions();

  const auto& imageArray = dataStructure.getDataRefAs<IDataArray>(imageArrayPath);
  const IDataStore& currentData = imageArray.getIDataStoreRef();
  DataType dataType = currentData.getDataType();
  ShapeType cDims = currentData.getComponentShape();

  ITK::ImageGeomData newImageGeom(imageGeom);

  const FloatVec3 origin = imageGeom.getOrigin();
  const FloatVec3 spacing = imageGeom.getSpacing();

  switch(plane)
  {
  case k_XYPlane: {
    usize dA = dims.getX();
    usize dB = dims.getY();

    newImageGeom.dims = {dims.getX(), dims.getY(), 1};
    newImageGeom.origin = {origin.getX(), origin.getY(), 0.0f};
    newImageGeom.spacing = {spacing.getX(), spacing.getY(), 1.0f};

    ShapeType sliceShape(std::reverse_iterator(newImageGeom.dims.end()), std::reverse_iterator(newImageGeom.dims.begin()));
    std::unique_ptr<IDataStore> sliceData = ExecuteDataFunction(cxITKImageWriterFilter::CreateDataStoreFunctor{}, dataType, sliceShape, cDims);

    for(usize slice = 0; slice < dims.getZ(); ++slice)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> copyResult = ExecuteDataFunctionNoBool(cxITKImageWriterFilter::CopyXYSlicesFunctor{}, dataType, dA, dB, slice, currentData, *sliceData); copyResult.invalid())
      {
        return copyResult;
      }
      const fs::path outputFilePath = cxITKImageWriterFilter::GenerateOutputFilePath(filePath, slice + indexOffset, dims.getZ(), totalDigits, fillChar);
      messageHandler.sendInfoMessage(fmt::format("Writing file {} of {}: \"{}\"", slice + 1, dims.getZ(), outputFilePath.string()));
      Result<> result = cxITKImageWriterFilter::SaveImageData(outputFilePath, *sliceData, newImageGeom);
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
    newImageGeom.origin = {origin.getX(), origin.getZ(), 0.0f};
    newImageGeom.spacing = {spacing.getX(), spacing.getZ(), 1.0f};

    ShapeType sliceShape(std::reverse_iterator(newImageGeom.dims.end()), std::reverse_iterator(newImageGeom.dims.begin()));
    std::unique_ptr<IDataStore> sliceData = ExecuteDataFunction(cxITKImageWriterFilter::CreateDataStoreFunctor{}, dataType, sliceShape, cDims);

    for(usize slice = 0; slice < dims.getY(); ++slice)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> copyResult = ExecuteDataFunctionNoBool(cxITKImageWriterFilter::CopyXZSlicesFunctor{}, dataType, dA, dB, slice, dims, currentData, *sliceData); copyResult.invalid())
      {
        return copyResult;
      }
      const fs::path outputFilePath = cxITKImageWriterFilter::GenerateOutputFilePath(filePath, slice + indexOffset, dims.getY(), totalDigits, fillChar);
      messageHandler.sendInfoMessage(fmt::format("Writing file {} of {}: \"{}\"", slice + 1, dims.getY(), outputFilePath.string()));
      Result<> result = cxITKImageWriterFilter::SaveImageData(outputFilePath, *sliceData, newImageGeom);
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
    newImageGeom.origin = {origin.getY(), origin.getZ(), 0.0f};
    newImageGeom.spacing = {spacing.getY(), spacing.getZ(), 1.0f};

    ShapeType sliceShape(std::reverse_iterator(newImageGeom.dims.end()), std::reverse_iterator(newImageGeom.dims.begin()));
    std::unique_ptr<IDataStore> sliceData = ExecuteDataFunction(cxITKImageWriterFilter::CreateDataStoreFunctor{}, dataType, sliceShape, cDims);

    for(usize slice = 0; slice < dims.getX(); ++slice)
    {
      if(shouldCancel)
      {
        return {};
      }
      if(Result<> copyResult = ExecuteDataFunctionNoBool(cxITKImageWriterFilter::CopyYZSlicesFunctor{}, dataType, dA, dB, slice, dims, currentData, *sliceData); copyResult.invalid())
      {
        return copyResult;
      }
      const fs::path outputFilePath = cxITKImageWriterFilter::GenerateOutputFilePath(filePath, slice + indexOffset, dims.getX(), totalDigits, fillChar);
      messageHandler.sendInfoMessage(fmt::format("Writing file {} of {}: \"{}\"", slice + 1, dims.getX(), outputFilePath.string()));
      Result<> result = cxITKImageWriterFilter::SaveImageData(outputFilePath, *sliceData, newImageGeom);
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

Result<Arguments> ITKImageWriterFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKImageWriterFilter().getDefaultArguments();

  std::vector<Result<>> results;

  Result<> planeResult = SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_PlaneKey, k_Plane_Key);
  if(planeResult.valid())
  {
    // This parameter does not appear in some 6.5 pipeline, thus we only include it in the output if it's valid
    results.push_back(std::move(planeResult));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::OutputFileFilterParameterConverter>(args, json, SIMPL::k_FileNameKey, k_FileName_Key));
  Result<> offsetResult = SIMPLConversion::ConvertParameter<SIMPLConversion::IntFilterParameterConverter<uint64>>(args, json, SIMPL::k_IndexOffsetKey, k_IndexOffset_Key);
  if(offsetResult.valid())
  {
    // This parameter does not appear in 6.5, thus we only include it in the output if it's valid
    results.push_back(std::move(offsetResult));
  }
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_ImageArrayPathKey, k_ImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_ImageArrayPathKey, k_ImageArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
