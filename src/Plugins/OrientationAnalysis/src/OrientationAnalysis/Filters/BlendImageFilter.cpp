#include "BlendImageFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/BlendImage.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

using namespace nx::core;

namespace
{
// Blend Mode choice indices. The leveling toggle is folded into the choice (as a dedicated state)
// so that its Image Geometry input is only required when background leveling is actually selected.
const ChoicesParameter::ValueType k_BasicIndex = 0;
const ChoicesParameter::ValueType k_AdvancedIndex = 1;
const ChoicesParameter::ValueType k_AdvancedLevelingIndex = 2;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string BlendImageFilter::name() const
{
  return FilterTraits<BlendImageFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string BlendImageFilter::className() const
{
  return FilterTraits<BlendImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid BlendImageFilter::uuid() const
{
  return FilterTraits<BlendImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string BlendImageFilter::humanName() const
{
  return "Blend Image";
}

//------------------------------------------------------------------------------
std::vector<std::string> BlendImageFilter::defaultTags() const
{
  return {className(), "Processing", "EBSD", "Image", "IPF", "Confidence Index", "Blend", "Colorize"};
}

//------------------------------------------------------------------------------
Parameters BlendImageFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CellIPFColorsArrayPath_Key, "Color Image Array", "The RGB color array produced by a coloring filter (3-component uint8).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{3}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_CIArrayPath_Key, "Modifier Array", "The array used to weight color brightness (1-component float32).", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Blend Mode"});
  params.insertLinkableParameter(std::make_unique<ChoicesParameter>(
      k_BlendMode_Key, "Blend Mode",
      "Basic: generic continuous blend of any 3-channel color array by the normalized modifier (values truncated). Advanced: adds 8-bit intensity scaling (the normalized weight is "
      "quantized to 256 levels and colors are rounded to the nearest value) plus the optional blackout and final-rescale transforms below, normalized globally over the whole image. "
      "'Advanced with Background Leveling' additionally subtracts a per-slice 2D background surface fit and normalizes/rescales each Z slice independently, which requires the Input Image Geometry.",
      k_BasicIndex, ChoicesParameter::Choices{"Basic", "Advanced", "Advanced with Background Leveling"}));

  params.insertSeparator(Parameters::Separator{"Advanced : Bad-Point Blackout"});
  params.insert(std::make_unique<BoolParameter>(k_UseBlackout_Key, "Blackout Bad Points",
                                                "Force any tuple whose criterion value is below the threshold to black (0,0,0). Use this to hide unindexed or invalid measurement points.", true));
  params.insert(std::make_unique<ArraySelectionParameter>(
      k_CriterionArrayPath_Key, "Blackout Criterion Array", "1-component float32 array tested against the threshold to identify bad/unindexed points (e.g. the Confidence Index).", DataPath{},
      ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insert(std::make_unique<Float32Parameter>(k_BlackoutThreshold_Key, "Blackout Threshold", "Tuples whose criterion value is strictly less than this value are blacked out.", 0.0f));

  params.insertSeparator(Parameters::Separator{"Advanced : Final Rescale"});
  params.insert(std::make_unique<BoolParameter>(k_UseFinalRescale_Key, "Apply Final Rescale",
                                                "Contrast-stretch the blended image across all channels so its darkest value maps to 0 and its brightest to 255. Off by default.", false));

  params.insertSeparator(Parameters::Separator{"Advanced with Background Leveling"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeometryPath_Key, "Input Image Geometry",
                                                             "Image Geometry supplying the X/Y/Z cell dimensions used for the per-slice surface fit and per-slice normalization.", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<Int32Parameter>(k_LevelingDegree_Key, "Surface Fit Degree", "Maximum total polynomial degree of the per-slice 2D background surface fit.", 3));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputArrayName_Key, "Blended Colors Array Name",
                                                          "Name of the output array containing weighted colors encoded as 3-component uint8.", "BlendedColors"));

  // Blackout, threshold and final rescale apply to both Advanced states; the geometry and degree are
  // only needed (and therefore only required) in the leveling state.
  params.linkParameters(k_BlendMode_Key, k_UseBlackout_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedIndex));
  params.linkParameters(k_BlendMode_Key, k_UseBlackout_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedLevelingIndex));
  params.linkParameters(k_BlendMode_Key, k_CriterionArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedIndex));
  params.linkParameters(k_BlendMode_Key, k_CriterionArrayPath_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedLevelingIndex));
  params.linkParameters(k_BlendMode_Key, k_BlackoutThreshold_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedIndex));
  params.linkParameters(k_BlendMode_Key, k_BlackoutThreshold_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedLevelingIndex));
  params.linkParameters(k_BlendMode_Key, k_UseFinalRescale_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedIndex));
  params.linkParameters(k_BlendMode_Key, k_UseFinalRescale_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedLevelingIndex));
  params.linkParameters(k_BlendMode_Key, k_InputImageGeometryPath_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedLevelingIndex));
  params.linkParameters(k_BlendMode_Key, k_LevelingDegree_Key, std::make_any<ChoicesParameter::ValueType>(k_AdvancedLevelingIndex));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType BlendImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer BlendImageFilter::clone() const
{
  return std::make_unique<BlendImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult BlendImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                             const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto pCellIPFColorsArrayPath = filterArgs.value<DataPath>(k_CellIPFColorsArrayPath_Key);
  auto pCIArrayPath = filterArgs.value<DataPath>(k_CIArrayPath_Key);
  auto pOutputArrayName = filterArgs.value<std::string>(k_OutputArrayName_Key);

  const auto pBlendMode = filterArgs.value<ChoicesParameter::ValueType>(k_BlendMode_Key);
  const bool advancedMode = pBlendMode != k_BasicIndex;
  const bool useLeveling = pBlendMode == k_AdvancedLevelingIndex;

  auto pOutputArrayPath = pCellIPFColorsArrayPath.replaceName(pOutputArrayName);

  // All arrays blended together must share a tuple count. The blackout criterion array is only
  // consulted when blackout is enabled in Advanced mode, so only enforce its tuple match then.
  std::vector<DataPath> dataPaths = {pCellIPFColorsArrayPath, pCIArrayPath};
  if(advancedMode && filterArgs.value<bool>(k_UseBlackout_Key))
  {
    dataPaths.push_back(filterArgs.value<DataPath>(k_CriterionArrayPath_Key));
  }
  auto tupleValidityCheck = dataStructure.validateNumberOfTuples(dataPaths);
  if(!tupleValidityCheck)
  {
    return {MakeErrorResult<OutputActions>(-68400, fmt::format("The following DataArrays all must have equal number of tuples but this was not satisfied.\n{}", tupleValidityCheck.error()))};
  }

  const auto& ipfColorsArray = dataStructure.getDataRefAs<UInt8Array>(pCellIPFColorsArrayPath);
  const usize numColorTuples = ipfColorsArray.getNumberOfTuples();

  // Background leveling processes each Z slice of the image geometry as an independent 2D image, so
  // the geometry cell count must equal the array tuple count and the fit degree must be valid. The
  // color and modifier arrays are already validated to have equal tuple counts.
  if(useLeveling)
  {
    const auto pLevelingDegree = filterArgs.value<int32>(k_LevelingDegree_Key);
    if(pLevelingDegree < 1)
    {
      return {MakeErrorResult<OutputActions>(-68401, fmt::format("Surface Fit Degree must be >= 1, but was {}.", pLevelingDegree))};
    }

    const auto pImageGeometryPath = filterArgs.value<DataPath>(k_InputImageGeometryPath_Key);
    const auto& imageGeom = dataStructure.getDataRefAs<ImageGeom>(pImageGeometryPath);
    const SizeVec3 dims = imageGeom.getDimensions();
    const usize numCells = dims[0] * dims[1] * dims[2];
    if(numCells != numColorTuples)
    {
      return {MakeErrorResult<OutputActions>(
          -68402, fmt::format("Background leveling requires the Input Image Geometry cell count ({}) to equal the color/modifier array tuple count ({}).", numCells, numColorTuples))};
    }
  }

  OutputActions actions;
  auto createAction = std::make_unique<CreateArrayAction>(DataType::uint8, ipfColorsArray.getIDataStore()->getTupleShape(), std::vector<usize>{3}, pOutputArrayPath);
  actions.appendAction(std::move(createAction));

  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> BlendImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  BlendImageInputValues inputValues;

  inputValues.cellIPFColorsArrayPath = filterArgs.value<DataPath>(k_CellIPFColorsArrayPath_Key);
  inputValues.ciArrayPath = filterArgs.value<DataPath>(k_CIArrayPath_Key);
  inputValues.outputArrayPath = inputValues.cellIPFColorsArrayPath.replaceName(filterArgs.value<std::string>(k_OutputArrayName_Key));

  const auto pBlendMode = filterArgs.value<ChoicesParameter::ValueType>(k_BlendMode_Key);
  inputValues.advancedMode = pBlendMode != k_BasicIndex;
  inputValues.useLeveling = pBlendMode == k_AdvancedLevelingIndex;
  inputValues.useBlackout = filterArgs.value<bool>(k_UseBlackout_Key);
  inputValues.criterionArrayPath = filterArgs.value<DataPath>(k_CriterionArrayPath_Key);
  inputValues.blackoutThreshold = filterArgs.value<float32>(k_BlackoutThreshold_Key);
  inputValues.imageGeometryPath = filterArgs.value<DataPath>(k_InputImageGeometryPath_Key);
  inputValues.levelingDegree = filterArgs.value<int32>(k_LevelingDegree_Key);
  inputValues.useFinalRescale = filterArgs.value<bool>(k_UseFinalRescale_Key);

  return BlendImage(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
