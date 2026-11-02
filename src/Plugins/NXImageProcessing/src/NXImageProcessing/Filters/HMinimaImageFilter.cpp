#include "HMinimaImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologicalReconstructionEngine.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <cmath>

using namespace nx::core;

namespace
{
// Error code for this filter. The -85xx range is shared by the ImageProcessing filters; the -8540 block is
// AdaptiveHistogramEqualization and the -8500..-8513 blocks are BinaryContour/LabelContour, so this filter uses
// the -8552 (non-finite) and -8555 (negative) slots for its Height guards (HMaxima owns -8551/-8554).
constexpr int32 k_NonFiniteHeight = -8552;
constexpr int32 k_NegativeHeight = -8555;
} // namespace

namespace nx::core
{
std::string HMinimaImageFilter::name() const
{
  return FilterTraits<HMinimaImageFilter>::name;
}
std::string HMinimaImageFilter::className() const
{
  return FilterTraits<HMinimaImageFilter>::className;
}
Uuid HMinimaImageFilter::uuid() const
{
  return FilterTraits<HMinimaImageFilter>::uuid;
}
std::string HMinimaImageFilter::humanName() const
{
  return "H Minima Image Filter";
}
std::vector<std::string> HMinimaImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "HMinima", "MathematicalMorphology", "Morphology"};
}
Parameters HMinimaImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height",
                                                   "The height (contrast) below which a regional minimum is suppressed. Every regional minimum whose depth is less than Height is removed; each "
                                                   "surviving minimum is raised by exactly Height. Must be finite.",
                                                   2.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the connected components are defined strictly by face connectivity (Off) or by face+edge+vertex connectivity (On). Default is Off. For "
                                                "minima that are 1 pixel wide, use On.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType HMinimaImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer HMinimaImageFilter::clone() const
{
  return std::make_unique<HMinimaImageFilter>();
}
IFilter::PreflightResult HMinimaImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                           const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // The marker offsets every voxel by +Height through a saturating cast, which is undefined for a non-finite
  // value; reject it here so the offset is always well-defined.
  if(!std::isfinite(height))
  {
    return {MakeErrorResult<OutputActions>(k_NonFiniteHeight, fmt::format("H Minima requires a finite Height, but the supplied value was {}.", height))};
  }
  // A negative Height has no meaning (no regional minimum has negative depth) and would push the (input + Height)
  // marker BELOW the mask, violating reconstruction-by-erosion's marker>=mask precondition (ITK throws). Require
  // Height >= 0; 0 is a valid no-op.
  if(height < 0.0)
  {
    return {MakeErrorResult<OutputActions>(k_NegativeHeight, fmt::format("H Minima requires a non-negative Height, but the supplied value was {}.", height))};
  }

  // requireScalar=true: the grayscale reconstruction engine is defined on scalar images (matching the legacy ITK
  // operator); the shared preflight also validates type membership and the geometry/tuple match.
  return {ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                        /*requireScalar=*/true)};
}
Result<> HMinimaImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  // HMinima == reconstruct-by-erosion of the saturating (input + Height) marker under the input mask; unlike
  // HMaxima the legacy ITK filter exposes a FullyConnected option, so honor it here.
  return ImageProcessing::ExecuteMorphologicalReconstructionImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, ImageProcessing::ReconstructOp::Erosion, fullyConnected,
                                                                        ImageProcessing::detail::ReconMarker::PlusHeight, height, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_HeightKey = "Height";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> HMinimaImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = HMinimaImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_HeightKey, k_Height_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
