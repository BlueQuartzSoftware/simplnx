#include "ITKImageReaderFilter.hpp"

#include "simplnx/Core/Application.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Filter/FilterHandle.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include "ITKImageProcessing/Common/ReadImageUtils.hpp"

#include <filesystem>

namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKImageReaderFilter::name() const
{
  return FilterTraits<ITKImageReaderFilter>::name;
}

//------------------------------------------------------------------------------
std::string ITKImageReaderFilter::className() const
{
  return FilterTraits<ITKImageReaderFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImageReaderFilter::uuid() const
{
  return FilterTraits<ITKImageReaderFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImageReaderFilter::humanName() const
{
  return "Read Image (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageReaderFilter::defaultTags() const
{
  return {className(), "io", "input", "read", "import"};
}

//------------------------------------------------------------------------------
Parameters ITKImageReaderFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry",
                                                   to_underlying(IGeometry::LengthUnit::Micrometer), IGeometry::GetAllLengthUnitStrings()));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Set Origin", "Specifies if the origin should be changed", false));
  params.insert(
      std::make_unique<BoolParameter>(k_CenterOrigin_Key, "Put Input Origin at the Center of Geometry", "Specifies if the origin should be aligned with the corner (false) or center (true)", false));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin (Physical Units)", "Specifies the new origin values in physical units.", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Set Spacing", "Specifies if the spacing should be changed", false));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing (Physical Units)", "Specifies the new spacing values in physical units.", std::vector<float64>{1, 1, 1},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeDataType_Key, "Set Image Data Type", "Set the final created image data type.", false));
  params.insert(std::make_unique<ChoicesParameter>(k_ImageDataType_Key, "Output Data Type", "Numeric Type of data to create", 0ULL,
                                                   ChoicesParameter::Choices{"uint8", "uint16", "uint32"})); // Sequence Dependent DO NOT REORDER

  params.linkParameters(k_ChangeDataType_Key, k_ImageDataType_Key, true);

  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeOrigin_Key, k_CenterOrigin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, std::make_any<bool>(true));

  params.insert(std::make_unique<FileSystemPathParameter>(k_FileName_Key, "File", "Input image file", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".png"}, {".tiff"}, {".tif"}, {".bmp"}, {".jpeg"}, {".jpg"}, {".nrrd"}, {".mha"}},
                                                          FileSystemPathParameter::PathType::InputFile, false));

  params.insertSeparator(Parameters::Separator{"Output Data Object(s)"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Created Cell Attribute Matrix", "The name of the created cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<DataObjectNameParameter>(k_ImageDataArrayPath_Key, "Created Cell Data",
                                                          "The name of the created image data array. Will be stored in the created Cell Attribute Matrix", "ImageData"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageReaderFilter::clone() const
{
  return std::make_unique<ITKImageReaderFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageReaderFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel) const
{
  auto fileName = filterArgs.value<fs::path>(k_FileName_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldCenterOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto origin = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Spacing_Key);
  auto pChangeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto pChoiceType = filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key);

  std::string fileNameString = fileName.string();

  cxItkImageReaderFilter::ImageReaderOptions imageReaderOptions;

  imageReaderOptions.OverrideOrigin = shouldChangeOrigin;
  imageReaderOptions.OverrideSpacing = shouldChangeSpacing;
  imageReaderOptions.OriginAtCenterOfGeometry = shouldCenterOrigin;
  imageReaderOptions.Origin = FloatVec3(static_cast<float32>(origin[0]), static_cast<float32>(origin[1]), static_cast<float32>(origin[2]));
  imageReaderOptions.Spacing = FloatVec3(static_cast<float32>(spacing[0]), static_cast<float32>(spacing[1]), static_cast<float32>(spacing[2]));
  imageReaderOptions.ChangeDataType = pChangeDataType;
  imageReaderOptions.ImageDataType = ITK::detail::ConvertChoiceToDataType(pChoiceType);

  Result<OutputActions> result = cxItkImageReaderFilter::ReadImagePreflight(fileNameString, imageGeomPath, cellDataName, imageDataArrayName, imageReaderOptions);

  return {result};
}

//------------------------------------------------------------------------------
Result<> ITKImageReaderFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel) const
{
  auto fileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<DataObjectNameParameter::ValueType>(k_CellDataName_Key);
  auto imageDataArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_ImageDataArrayPath_Key);
  //  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  //  auto shouldCenterOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  //  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto origin = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Origin_Key);
  auto spacing = filterArgs.value<VectorFloat64Parameter::ValueType>(k_Spacing_Key);
  auto pChangeDataType = filterArgs.value<bool>(k_ChangeDataType_Key);
  auto newDataType = ITK::detail::ConvertChoiceToDataType(filterArgs.value<ChoicesParameter::ValueType>(k_ImageDataType_Key));

  DataPath imageDataArrayPath = imageGeometryPath.createChildPath(cellDataName).createChildPath(imageDataArrayName);
  const IDataArray* inputArrayPtr = dataStructure.getDataAs<IDataArray>(imageDataArrayPath);
  if(!inputArrayPtr->getDataFormat().empty())
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageDataArrayPath.toString()));
  }

  std::string fileNameString = fileName.string();

  Result<> result = {};
  if(pChangeDataType)
  {
    result = cxItkImageReaderFilter::ReadImageExecute<cxItkImageReaderFilter::ReadImageIntoArrayFunctor>(fileNameString, dataStructure, fileNameString, imageDataArrayPath, newDataType);
  }
  else
  {
    result = cxItkImageReaderFilter::ReadImageExecute<cxItkImageReaderFilter::ReadImageIntoArrayFunctor>(fileNameString, dataStructure, imageDataArrayPath, fileNameString);
  }

  return result;
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_FileNameKey = "FileName";
constexpr StringLiteral k_DataContainerNameKey = "DataContainerName";
constexpr StringLiteral k_CellAttributeMatrixNameKey = "CellAttributeMatrixName";
constexpr StringLiteral k_ImageDataArrayNameKey = "ImageDataArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> ITKImageReaderFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKImageReaderFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_FileNameKey, k_FileName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellDataName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::LinkedPathCreationFilterParameterConverter>(args, json, SIMPL::k_ImageDataArrayNameKey, k_ImageDataArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
