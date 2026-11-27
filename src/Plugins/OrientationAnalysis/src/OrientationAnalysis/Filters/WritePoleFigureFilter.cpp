#include "WritePoleFigureFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/WritePoleFigure.hpp"
#include "OrientationAnalysis/utilities/LatoBold.hpp"
#include "OrientationAnalysis/utilities/SIMPLConversion.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Filter/Actions/CreateImageGeometryAction.hpp"
#include "simplnx/Filter/Actions/CreateStringArrayAction.hpp"
#include "simplnx/Filter/Actions/DeleteDataAction.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupCreationParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/Fonts/Fonts.hpp"

#define CANVAS_ITY_IMPLEMENTATION
#include <canvas_ity.hpp>

#include <filesystem>
namespace fs = std::filesystem;

using namespace nx::core;

namespace
{

/**
 * @brief Measures the width of the letter X at the selected font size.
 * @param imageSize Temporary canvas width and height in pixels.
 * @param fontPtSize Font size in points.
 * @return Measured text width in pixels.
 */
float32 GetXCharWidth(int32 imageSize, float32 fontPtSize)
{
  std::vector<unsigned char> mLatoBold;
  fonts::Base64Decode(fonts::k_LatoBoldBase64, mLatoBold);

  canvas_ity::canvas tempContext(imageSize, imageSize);
  tempContext.set_font(mLatoBold.data(), static_cast<int>(mLatoBold.size()), fontPtSize);
  return tempContext.measure_text("X");
}

} // namespace

namespace nx::core
{
std::string WritePoleFigureFilter::name() const
{
  return FilterTraits<WritePoleFigureFilter>::name.str();
}

std::string WritePoleFigureFilter::className() const
{
  return FilterTraits<WritePoleFigureFilter>::className;
}

Uuid WritePoleFigureFilter::uuid() const
{
  return FilterTraits<WritePoleFigureFilter>::uuid;
}

std::string WritePoleFigureFilter::humanName() const
{
  return "Generate and Write Pole Figure Images";
}

std::vector<std::string> WritePoleFigureFilter::defaultTags() const
{
  return {className(), "IO", "Output", "Write", "Export", "EBSD", "Pole Figure"};
}

Parameters WritePoleFigureFilter::parameters() const
{
  Parameters params;
  // Create the filter parameter descriptors.
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<StringParameter>(k_Title_Key, "Figure Title", "The title to place at the top of the Pole Figure", "Figure Title"));
  params.insert(std::make_unique<Int32ParameterType>(k_ImageSize_Key, "Image Size (Square Pixels)", "The number of pixels that define the height and width of **each** output pole figure", 512));

  params.insert(std::make_unique<ChoicesParameter>(k_ImageLayout_Key, "Image Layout", "How to layout the 3 pole figures. 0=Horizontal, 1=Vertical, 2=Square", 0,
                                                   ChoicesParameter::ChoicesType{"Horizontal", "Vertical", "Square"}));

  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(k_GenerationAlgorithm_Key, "Pole Figure Type", "The type of pole figure generated. 0=Color, 1=Discrete", 0,
                                                                    ChoicesParameter::ChoicesType{"Color Intensity", "Discrete"}));
  params.insert(std::make_unique<Int32ParameterType>(k_LambertSize_Key, "Lambert Image Size (Pixels)", "The height/width of the internal Lambert Square that is used for interpolation", 64));
  params.insert(std::make_unique<Int32ParameterType>(k_NumColors_Key, "Number of Colors", "The number of colors to use for the Color Intensity pole figures", 32));
  params.insert(
      std::make_unique<Int32ParameterType>(k_DiscreteMarkerRadius_Key, "Discrete Marker Radius (Pixels)", "Radius of each discrete pole marker. Only used for the Discrete pole figure type.", 3));
  params.insert(std::make_unique<ChoicesParameter>(
      k_HexConvention_Key, "Hex/Trig Cartesian Basis Convention",
      "Cartesian basis used for hex/trigonal phases. Pole-figure positions and corner labels are rotated 30° about the c-axis between the two:\n"
      "  X parallel to a: EDAX/TSL/OIM Analysis convention. This is the convention every released DREAM.3D / DREAM3DNX / SIMPL / SIMPLNX file stores hex/trig EulerAngles in.\n"
      "  X parallel to a*: MTEX / Oxford Channel 5 / AZtec convention. Pick this for apples-to-apples comparison against MTEX-produced pole figures.\n"
      "Cubic, tetragonal, orthorhombic, monoclinic, and triclinic phases ignore this setting.",
      0, ChoicesParameter::ChoicesType{"X || A (EDAX/TSL)", "X || A* (MTEX/Aztec)"}));

  params.insertSeparator(Parameters::Separator{"Input Orientation Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellEulerAnglesArrayPath_Key, "Euler Angles", "Three angles defining the orientation of the Element in Bunge convention (Z-X-Z)",
                                                          DataPath{}, GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::float32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellPhasesArrayPath_Key, "Phases", "Specifies to which Ensemble each Euler angle belongs", DataPath{}, GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Should the algorithm use a mask array to remove non-indexed points", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "DataPath to the input Mask DataArray", DataPath({"Mask"}), GetDataArrayOnlyTypes(),
                                                          ArraySelectionParameter::AllowedDataTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaterialNameArrayPath_Key, "Material Name", "DataPath to the input DataArray that holds the material names", DataPath{},
                                                          ArraySelectionParameter::AllowedArrayTypes{IArray::ArrayType::StringArray}, ArraySelectionParameter::AllowedDataTypes::StringOnly(),
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Image File"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_WriteImageToDisk, "Write Pole Figure as Image", "Should the filter write the pole figure plots to a file.", true));

  params.insert(std::make_unique<FileSystemPathParameter>(k_OutputPath_Key, "Output Directory Path",
                                                          "This is the path to the directory where the pole figures will be created. One file for each phase.", fs::path(""),
                                                          FileSystemPathParameter::ExtensionsType{}, FileSystemPathParameter::PathType::OutputDir, true));
  params.insert(
      std::make_unique<StringParameter>(k_ImagePrefix_Key, "Pole Figure File Prefix", "The prefix to apply to each generated pole figure. Each Phase will have its own pole figure.", "Phase_"));

  params.insertSeparator(Parameters::Separator{"Output Image Geometry"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveAsImageGeometry_Key, "Save Output as Image Geometry", "Save the generated pole figure as an ImageGeometry", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_ImageGeometryPath_Key, "Output Image Geometry", "The path to the created Image Geometry", DataPath({"PoleFigure"})));

  params.insertSeparator(Parameters::Separator{"Output Count Data Arrays"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_SaveIntensityDataArrays, "Save Count Images", "Save the Count Plots (x3)", true));
  params.insert(std::make_unique<DataGroupCreationParameter>(k_IntensityGeometryPath, "Output Count Image Geometry", "The path to the created Count Image Geometries", DataPath({"Intensity Data"})));
  params.insert(std::make_unique<BoolParameter>(k_NormalizeToMRD, "Normalize Count Data to MRD", "The Pole Figure data should be normalized to MRD values", true));
  params.insert(std::make_unique<DataObjectNameParameter>(k_IntensityPlot1Name, "Count Plot 1", "The counts data for the plot", "<001>"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_IntensityPlot2Name, "Count Plot 2", "The counts data for the plot", "<011>"));
  params.insert(std::make_unique<DataObjectNameParameter>(k_IntensityPlot3Name, "Count Plot 3", "The counts data for the plot", "<111>"));

  // Link each parent parameter to the child parameters it controls.
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  params.linkParameters(k_GenerationAlgorithm_Key, k_LambertSize_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GenerationAlgorithm_Key, k_NumColors_Key, std::make_any<ChoicesParameter::ValueType>(0));
  params.linkParameters(k_GenerationAlgorithm_Key, k_DiscreteMarkerRadius_Key, std::make_any<ChoicesParameter::ValueType>(1));

  params.linkParameters(k_WriteImageToDisk, k_OutputPath_Key, true);
  params.linkParameters(k_WriteImageToDisk, k_ImagePrefix_Key, true);

  params.linkParameters(k_SaveAsImageGeometry_Key, k_ImageGeometryPath_Key, true);

  params.linkParameters(k_SaveIntensityDataArrays, k_IntensityGeometryPath, true);
  params.linkParameters(k_SaveIntensityDataArrays, k_NormalizeToMRD, true);
  params.linkParameters(k_SaveIntensityDataArrays, k_IntensityPlot1Name, true);
  params.linkParameters(k_SaveIntensityDataArrays, k_IntensityPlot2Name, true);
  params.linkParameters(k_SaveIntensityDataArrays, k_IntensityPlot3Name, true);

  return params;
}

IFilter::VersionType WritePoleFigureFilter::parametersVersion() const
{
  return 3;
}

IFilter::UniquePointerType WritePoleFigureFilter::clone() const
{
  return std::make_unique<WritePoleFigureFilter>();
}

IFilter::PreflightResult WritePoleFigureFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                              const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{

  auto pTitleValue = filterArgs.value<StringParameter::ValueType>(k_Title_Key);
  auto pImageLayoutValue = filterArgs.value<ChoicesParameter::ValueType>(k_ImageLayout_Key);
  auto pOutputPathValue = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  auto pImagePrefixValue = filterArgs.value<StringParameter::ValueType>(k_ImagePrefix_Key);
  auto pImageSizeValue = filterArgs.value<int32>(k_ImageSize_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  auto pCellPhasesArrayPathValue = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  auto pGoodVoxelsArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pCrystalStructuresArrayPathValue = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);

  auto pSaveAsImageGeometry = filterArgs.value<bool>(k_SaveAsImageGeometry_Key);
  auto pWriteImageToDisk = filterArgs.value<bool>(k_WriteImageToDisk);
  auto pOutputImageGeometryPath = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);

  auto pSaveIntensityPlot = filterArgs.value<bool>(k_SaveIntensityDataArrays);
  auto pOutputIntensityGeometryPath = filterArgs.value<DataPath>(k_IntensityGeometryPath);
  auto pIntensityPlot1Name = filterArgs.value<DataObjectNameParameter::ValueType>(k_IntensityPlot1Name);
  auto pIntensityPlot2Name = filterArgs.value<DataObjectNameParameter::ValueType>(k_IntensityPlot2Name);
  auto pIntensityPlot3Name = filterArgs.value<DataObjectNameParameter::ValueType>(k_IntensityPlot3Name);

  PreflightResult preflightResult;

  if(pImageSizeValue <= 0)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(-680002, fmt::format("'Image Size (Square Pixels)' must be greater than zero. Value given: {}", pImageSizeValue)), .output_values = {}};
  }
  // The marker radius is only active (and only used) for the Discrete pole figure type.
  if(filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key) == 1ULL)
  {
    const auto pDiscreteMarkerRadiusValue = filterArgs.value<int32>(k_DiscreteMarkerRadius_Key);
    if(pDiscreteMarkerRadiusValue < 1)
    {
      return {.output_actions = MakeErrorResult<OutputActions>(-680003, fmt::format("'Discrete Marker Radius (Pixels)' must be at least 1. Value given: {}", pDiscreteMarkerRadiusValue)),
              .output_values = {}};
    }
  }

  nx::core::Result<OutputActions> resultOutputActions;

  // Calculate output dimensions from the image size, margins, and text width.
  float32 fontPtSize = pImageSizeValue / 16.0f;
  float32 margins = pImageSizeValue / 32.0f;

  float32 xCharWidth = GetXCharWidth(pImageSizeValue, fontPtSize);

  int32 pageWidth = 0;
  int32 pageHeight = margins + fontPtSize;
  // Reserve one square subcanvas for each pole figure.
  float32 subCanvasWidth = margins + pImageSizeValue + xCharWidth + margins;
  float32 subCanvasHeight = margins + fontPtSize + pImageSizeValue + fontPtSize * 2 + margins * 2;
  if(static_cast<WritePoleFigure::LayoutType>(pImageLayoutValue) == WritePoleFigure::LayoutType::Horizontal)
  {
    pageWidth = subCanvasWidth * 4;
    pageHeight = pageHeight + subCanvasHeight;
  }
  else if(static_cast<WritePoleFigure::LayoutType>(pImageLayoutValue) == WritePoleFigure::LayoutType::Vertical)
  {
    pageWidth = subCanvasWidth;
    pageHeight = pageHeight + subCanvasHeight * 4.0f;
  }
  else if(static_cast<WritePoleFigure::LayoutType>(pImageLayoutValue) == nx::core::WritePoleFigure::LayoutType::Square)
  {
    pageWidth = subCanvasWidth * 2.0f;
    pageHeight = pageHeight + subCanvasHeight * 2.0f;
  }
  {
    const std::vector<size_t> dims = {1ULL, static_cast<usize>(pageHeight), static_cast<usize>(pageWidth)};
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pOutputImageGeometryPath, dims, std::vector<float>{0.0f, 0.0f, 0.0f}, std::vector<float>{1.0f, 1.0f, 1.0f},
                                                                                 write_pole_figure::k_ImageAttrMatName, IGeometry::LengthUnit::Unspecified);
    resultOutputActions.value().appendAction(std::move(createImageGeometryAction));
  }
  if(!pSaveAsImageGeometry)
  {
    // Remove the temporary output geometry after execution.
    resultOutputActions.value().appendDeferredAction(std::make_unique<DeleteDataAction>(pOutputImageGeometryPath));
  }

  std::vector<PreflightValue> preflightUpdatedValues;
  if(pWriteImageToDisk)
  {
    preflightUpdatedValues.push_back({"Example Output File.", fmt::format("{}/{}Phase_1.png", pOutputPathValue.string(), pImagePrefixValue)});
  }

  if(pSaveIntensityPlot)
  {
    const std::vector<size_t> intensityImageDims = {static_cast<usize>(pImageSizeValue), static_cast<usize>(pImageSizeValue), 1ULL};
    auto createImageGeometryAction = std::make_unique<CreateImageGeometryAction>(pOutputIntensityGeometryPath, intensityImageDims, std::vector<float>{0.0f, 0.0f, 0.0f},
                                                                                 std::vector<float>{1.0f, 1.0f, 1.0f}, write_pole_figure::k_ImageAttrMatName, IGeometry::LengthUnit::Unspecified);
    resultOutputActions.value().appendAction(std::move(createImageGeometryAction));

    const std::vector<size_t> intensityImageArrayDims = std::vector<usize>{intensityImageDims.rbegin(), intensityImageDims.rend()};
    std::vector<size_t> cDims = {1ULL};
    DataPath path = pOutputIntensityGeometryPath.createChildPath(write_pole_figure::k_ImageAttrMatName).createChildPath(fmt::format("Phase_{}_{}", 1, pIntensityPlot1Name));
    auto createArray1 = std::make_unique<CreateArrayAction>(DataType::float64, intensityImageArrayDims, cDims, path);
    resultOutputActions.value().appendAction(std::move(createArray1));

    path = pOutputIntensityGeometryPath.createChildPath(write_pole_figure::k_ImageAttrMatName).createChildPath(fmt::format("Phase_{}_{}", 1, pIntensityPlot2Name));
    auto createArray2 = std::make_unique<CreateArrayAction>(DataType::float64, intensityImageArrayDims, cDims, path);
    resultOutputActions.value().appendAction(std::move(createArray2));

    path = pOutputIntensityGeometryPath.createChildPath(write_pole_figure::k_ImageAttrMatName).createChildPath(fmt::format("Phase_{}_{}", 1, pIntensityPlot3Name));
    auto createArray3 = std::make_unique<CreateArrayAction>(DataType::float64, intensityImageArrayDims, cDims, path);
    resultOutputActions.value().appendAction(std::move(createArray3));

    path = pOutputIntensityGeometryPath.createChildPath(write_pole_figure::k_MetaDataName);
    auto createMetaData = std::make_unique<CreateStringArrayAction>(std::vector<usize>{0ULL}, path);
    resultOutputActions.value().appendAction(std::move(createMetaData));
  }

  // Return actions and updated preflight values.
  return {.output_actions = std::move(resultOutputActions), .output_values = std::move(preflightUpdatedValues)};
}

Result<> WritePoleFigureFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{

  WritePoleFigureInputValues inputValues;

  inputValues.title = filterArgs.value<StringParameter::ValueType>(k_Title_Key);
  inputValues.generation_algorithm = filterArgs.value<ChoicesParameter::ValueType>(k_GenerationAlgorithm_Key);
  inputValues.lambert_size = filterArgs.value<int32>(k_LambertSize_Key);
  inputValues.num_colors = filterArgs.value<int32>(k_NumColors_Key);
  inputValues.image_size = filterArgs.value<int32>(k_ImageSize_Key);
  inputValues.discrete_marker_radius = static_cast<float32>(filterArgs.value<int32>(k_DiscreteMarkerRadius_Key)) / static_cast<float32>(inputValues.image_size);
  inputValues.image_layout = filterArgs.value<ChoicesParameter::ValueType>(k_ImageLayout_Key);
  inputValues.output_path = filterArgs.value<FileSystemPathParameter::ValueType>(k_OutputPath_Key);
  inputValues.image_prefix = filterArgs.value<StringParameter::ValueType>(k_ImagePrefix_Key);

  inputValues.use_mask = filterArgs.value<bool>(k_UseMask_Key);
  inputValues.cell_euler_angles_array_path = filterArgs.value<DataPath>(k_CellEulerAnglesArrayPath_Key);
  inputValues.cell_phases_array_path = filterArgs.value<DataPath>(k_CellPhasesArrayPath_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.crystal_structures_array_path = filterArgs.value<DataPath>(k_CrystalStructuresArrayPath_Key);
  inputValues.material_name_array_path = filterArgs.value<ArraySelectionParameter::ValueType>(k_MaterialNameArrayPath_Key);
  inputValues.save_as_image_geometry = filterArgs.value<bool>(k_SaveAsImageGeometry_Key);
  inputValues.write_image_to_disk = filterArgs.value<bool>(k_WriteImageToDisk);
  inputValues.output_image_geometry_path = filterArgs.value<DataPath>(k_ImageGeometryPath_Key);
  // Keep +Y upward to match the fixed renderer orientation.
  inputValues.flip_final_image = true;

  inputValues.save_intensity_data = filterArgs.value<bool>(k_SaveIntensityDataArrays);
  inputValues.normalize_to_mrd = filterArgs.value<bool>(k_NormalizeToMRD);
  inputValues.intensity_geometry_data_path = filterArgs.value<DataPath>(k_IntensityGeometryPath);
  inputValues.intensity_plot1_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_IntensityPlot1Name);
  inputValues.intensity_plot2_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_IntensityPlot2Name);
  inputValues.intensity_plot3_name = filterArgs.value<DataObjectNameParameter::ValueType>(k_IntensityPlot3Name);
  const auto hexConventionChoice = filterArgs.value<ChoicesParameter::ValueType>(k_HexConvention_Key);
  switch(hexConventionChoice)
  {
  case 0:
    inputValues.hex_convention = ebsdlib::HexConvention::XParallelA;
    break;
  case 1:
    inputValues.hex_convention = ebsdlib::HexConvention::XParallelAStar;
    break;
  default:
    return MakeErrorResult(-680010, fmt::format("Invalid Hex Convention choice: {}. Valid range is [0, 1].", hexConventionChoice));
  }

  return WritePoleFigure(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_TitleKey = "Title";
constexpr StringLiteralType k_GenerationAlgorithmKey = "GenerationAlgorithm";
constexpr StringLiteralType k_LambertSizeKey = "LambertSize";
constexpr StringLiteralType k_NumColorsKey = "NumColors";
constexpr StringLiteralType k_ImageLayoutKey = "ImageLayout";
constexpr StringLiteralType k_OutputPathKey = "OutputPath";
constexpr StringLiteralType k_ImagePrefixKey = "ImagePrefix";
constexpr StringLiteralType k_ImageSizeKey = "ImageSize";
constexpr StringLiteralType k_UseGoodVoxelsKey = "UseGoodVoxels";
constexpr StringLiteralType k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
constexpr StringLiteralType k_CellPhasesArrayPathKey = "CellPhasesArrayPath";
constexpr StringLiteralType k_GoodVoxelsArrayPathKey = "GoodVoxelsArrayPath";
constexpr StringLiteralType k_CrystalStructuresArrayPathKey = "CrystalStructuresArrayPath";
constexpr StringLiteralType k_MaterialNameArrayPathKey = "MaterialNameArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> WritePoleFigureFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = WritePoleFigureFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_TitleKey, k_Title_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedChoicesFilterParameterConverter>(args, json, simpl::k_GenerationAlgorithmKey, k_GenerationAlgorithm_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_LambertSizeKey, k_LambertSize_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumColorsKey, k_NumColors_Key));
  // SIMPLNX always writes PNG, so the legacy ImageFormat choice is not converted.
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_ImageLayoutKey, k_ImageLayout_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::OutputFileFilterParameterConverter>(args, json, simpl::k_OutputPathKey, k_OutputPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_ImagePrefixKey, k_ImagePrefix_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_ImageSizeKey, k_ImageSize_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::LinkedBooleanFilterParameterConverter>(args, json, simpl::k_UseGoodVoxelsKey, k_UseMask_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellEulerAnglesArrayPathKey, k_CellEulerAnglesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CellPhasesArrayPathKey, k_CellPhasesArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_GoodVoxelsArrayPathKey, k_MaskArrayPath_Key));
  // Do not map the legacy crystal-structure container to the output geometry path.
  // The filter creates a new geometry, and reusing the input name can collide in preflight.
  // Keep the default output geometry name.
  results.push_back(
      simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_CrystalStructuresArrayPathKey, k_CrystalStructuresArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_MaterialNameArrayPathKey, k_MaterialNameArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
