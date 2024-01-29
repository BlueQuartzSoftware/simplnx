#include "ITKImageReader.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStore.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/UpdateImageGeomAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include "ITKImageProcessing/Common/ITKArrayHelper.hpp"
#include "ITKImageProcessing/Common/ReadImageUtils.hpp"

#include <filesystem>

#include "simplnx/Utilities/SIMPLConversion.hpp"

namespace fs = std::filesystem;

using namespace nx::core;

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ITKImageReader::name() const
{
  return FilterTraits<ITKImageReader>::name;
}

//------------------------------------------------------------------------------
std::string ITKImageReader::className() const
{
  return FilterTraits<ITKImageReader>::className;
}

//------------------------------------------------------------------------------
Uuid ITKImageReader::uuid() const
{
  return FilterTraits<ITKImageReader>::uuid;
}

//------------------------------------------------------------------------------
std::string ITKImageReader::humanName() const
{
  return "Read Image (ITK)";
}

//------------------------------------------------------------------------------
std::vector<std::string> ITKImageReader::defaultTags() const
{
  return {className(), "io", "input", "read", "import"};
}

//------------------------------------------------------------------------------
Parameters ITKImageReader::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameters"});

  params.insert(std::make_unique<ChoicesParameter>(k_LengthUnit_Key, "Length Unit", "The length unit that will be set into the created image geometry",
                                                   to_underlying(IGeometry::LengthUnit::Micrometer), IGeometry::GetAllLengthUnitStrings()));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeOrigin_Key, "Set Origin", "Specifies if the origin should be changed", true));
  params.insert(
      std::make_unique<BoolParameter>(k_CenterOrigin_Key, "Put Input Origin at the Center of Geometry", "Specifies if the origin should be aligned with the corner (false) or center (true)", false));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Origin_Key, "Origin  (Physical Units)", "Specifies the new origin values in physical units.", std::vector<float64>{0.0, 0.0, 0.0},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_ChangeSpacing_Key, "Set Spacing", "Specifies if the spacing should be changed", true));
  params.insert(std::make_unique<VectorFloat64Parameter>(k_Spacing_Key, "Spacing (Physical Units)", "Specifies the new spacing values in physical units.", std::vector<float64>{1, 1, 1},
                                                         std::vector<std::string>{"X", "Y", "Z"}));

  params.linkParameters(k_ChangeOrigin_Key, k_Origin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeOrigin_Key, k_CenterOrigin_Key, std::make_any<bool>(true));
  params.linkParameters(k_ChangeSpacing_Key, k_Spacing_Key, std::make_any<bool>(true));

  params.insert(std::make_unique<FileSystemPathParameter>(k_FileName_Key, "File", "Input image file", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{{".png"}, {".tiff"}, {".tif"}, {".bmp"}, {".jpeg"}, {".jpg"}, {".nrrd"}, {".mha"}},
                                                          FileSystemPathParameter::PathType::InputFile, false));

  params.insertSeparator(Parameters::Separator{"Created Cell Data"});
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Created Image Geometry", "The path to the created Image Geometry", DataPath({"ImageDataContainer"})));
  params.insert(std::make_unique<DataObjectNameParameter>(k_CellDataName_Key, "Cell Data Name", "The name of the created cell attribute matrix", ImageGeom::k_CellDataName));
  params.insert(std::make_unique<ArrayCreationParameter>(k_ImageDataArrayPath_Key, "Created Image Data", "The path to the created image data array",
                                                         DataPath({"ImageDataContainer", ImageGeom::k_CellDataName, "ImageData"})));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ITKImageReader::clone() const
{
  return std::make_unique<ITKImageReader>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ITKImageReader::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                       const std::atomic_bool& shouldCancel) const
{
  auto fileName = filterArgs.value<fs::path>(k_FileName_Key);
  auto imageGeomPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto cellDataName = filterArgs.value<std::string>(k_CellDataName_Key);
  auto imageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldCenterOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto origin = filterArgs.value<std::vector<float64>>(k_Origin_Key);
  auto spacing = filterArgs.value<std::vector<float64>>(k_Spacing_Key);

  std::string fileNameString = fileName.string();

  Result<> check = cxItkImageReader::ReadImageExecute<cxItkImageReader::PreflightFunctor>(fileNameString);
  if(check.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(check), {})};
  }

  Result<OutputActions> result = cxItkImageReader::ReadImagePreflight(fileNameString, imageGeomPath, cellDataName, imageDataArrayPath);

  const auto* srcImageGeom = dataStructure.getDataAs<ImageGeom>(imageGeomPath);

  if(srcImageGeom != nullptr)
  {
    FloatVec3 originalOrigin = srcImageGeom->getOrigin();
    FloatVec3 originalSpacing = srcImageGeom->getSpacing();

    if(shouldChangeOrigin)
    {
      FloatVec3 originVec(origin[0], origin[1], origin[2]);

      auto action = std::make_unique<UpdateImageGeomAction>(originVec, originalSpacing, imageGeomPath, shouldCenterOrigin);
      result.value().appendAction(std::move(action));
    }

    if(shouldChangeSpacing)
    {
      FloatVec3 spacingVec(spacing[0], spacing[1], spacing[2]);
      auto action = std::make_unique<UpdateImageGeomAction>(originalOrigin, spacingVec, imageGeomPath, shouldCenterOrigin);
      result.value().appendAction(std::move(action));
    }
  }
  return {result};
}

//------------------------------------------------------------------------------
Result<> ITKImageReader::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel) const
{
  auto fileName = filterArgs.value<FileSystemPathParameter::ValueType>(k_FileName_Key);
  auto imageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  auto imageDataArrayPath = filterArgs.value<DataPath>(k_ImageDataArrayPath_Key);
  auto shouldChangeOrigin = filterArgs.value<bool>(k_ChangeOrigin_Key);
  auto shouldCenterOrigin = filterArgs.value<bool>(k_CenterOrigin_Key);
  auto shouldChangeSpacing = filterArgs.value<bool>(k_ChangeSpacing_Key);
  auto origin = filterArgs.value<std::vector<float64>>(k_Origin_Key);
  auto spacing = filterArgs.value<std::vector<float64>>(k_Spacing_Key);

  const IDataArray* inputArray = dataStructure.getDataAs<IDataArray>(imageDataArrayPath);
  if(inputArray->getDataFormat() != "")
  {
    return MakeErrorResult(-9999, fmt::format("Input Array '{}' utilizes out-of-core data. This is not supported within ITK filters.", imageDataArrayPath.toString()));
  }

  std::string fileNameString = fileName.string();

  ImageGeom& imageGeom = dataStructure.getDataRefAs<ImageGeom>(imageGeometryPath);
  imageGeom.getLinkedGeometryData().addCellData(imageDataArrayPath);

  {
    FloatVec3 originalOrigin = imageGeom.getOrigin();
    FloatVec3 originalSpacing = imageGeom.getSpacing();

    if(shouldChangeOrigin)
    {
      FloatVec3 originVec(origin[0], origin[1], origin[2]);
      if(shouldCenterOrigin)
      {
        BoundingBox3Df bounds = imageGeom.getBoundingBoxf();
        Point3Df centerPoint = bounds.center();
        Point3Df minPoint = bounds.getMinPoint();

        for(uint8 i = 0; i < 3; i++)
        {
          // absolute center - absolute min point = absolute offset
          float32 offset = std::abs(centerPoint[i]) - std::abs(minPoint[i]);
          // subtract absolute offset and save point into "new" origin
          originVec[i] -= offset;
        }
      }
      imageGeom.setOrigin(originVec);

    }

    if(shouldChangeSpacing)
    {
      FloatVec3 spacingVec(spacing[0], spacing[1], spacing[2]);
      imageGeom.setSpacing(spacingVec);
      //      auto action = std::make_unique<UpdateImageGeomAction>(originalOrigin, spacingVec, imageGeomPath, shouldCenterOrigin);
      //      result.value().appendAction(std::move(action));
    }
  }

  auto result = cxItkImageReader::ReadImageExecute<cxItkImageReader::ReadImageIntoArrayFunctor>(fileNameString, dataStructure, imageDataArrayPath, fileNameString);

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

Result<Arguments> ITKImageReader::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ITKImageReader().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::InputFileFilterParameterConverter>(args, json, SIMPL::k_FileNameKey, k_FileName_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerCreationFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, k_ImageGeometryPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_CellAttributeMatrixNameKey, k_CellDataName_Key));
  results.push_back(SIMPLConversion::Convert3Parameters<SIMPLConversion::DAPathBuilderFilterParameterConverter>(args, json, SIMPL::k_DataContainerNameKey, SIMPL::k_CellAttributeMatrixNameKey,
                                                                                                                SIMPL::k_ImageDataArrayNameKey, k_ImageDataArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
