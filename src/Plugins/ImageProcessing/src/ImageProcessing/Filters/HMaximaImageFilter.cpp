#include "HMaximaImageFilter.hpp"

#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
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
// the -8551 (non-finite) and -8554 (negative) slots for its Height guards.
constexpr int32 k_NonFiniteHeight = -8551;
constexpr int32 k_NegativeHeight = -8554;
} // namespace

namespace nx::core
{
std::string HMaximaImageFilter::name() const
{
  return FilterTraits<HMaximaImageFilter>::name;
}
std::string HMaximaImageFilter::className() const
{
  return FilterTraits<HMaximaImageFilter>::className;
}
Uuid HMaximaImageFilter::uuid() const
{
  return FilterTraits<HMaximaImageFilter>::uuid;
}
std::string HMaximaImageFilter::humanName() const
{
  return "H Maxima Image Filter";
}
std::vector<std::string> HMaximaImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "HMaxima", "MathematicalMorphology", "Morphology"};
}
Parameters HMaximaImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Height_Key, "Height",
                                                   "The height (contrast) below which a regional maximum is suppressed. Every regional maximum whose depth is less than Height is removed; each "
                                                   "surviving maximum is lowered by exactly Height. Must be finite.",
                                                   2.0));

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
IFilter::VersionType HMaximaImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer HMaximaImageFilter::clone() const
{
  return std::make_unique<HMaximaImageFilter>();
}
IFilter::PreflightResult HMaximaImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                           const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // The marker offsets every voxel by -Height through a saturating cast, which is undefined for a non-finite
  // value; reject it here so the offset is always well-defined.
  if(!std::isfinite(height))
  {
    return {MakeErrorResult<OutputActions>(k_NonFiniteHeight, fmt::format("H Maxima requires a finite Height, but the supplied value was {}.", height))};
  }
  // A negative Height has no meaning (no regional maximum has negative depth) and would push the (input - Height)
  // marker ABOVE the mask, violating reconstruction-by-dilation's marker<=mask precondition (ITK throws). Require
  // Height >= 0; 0 is a valid no-op.
  if(height < 0.0)
  {
    return {MakeErrorResult<OutputActions>(k_NegativeHeight, fmt::format("H Maxima requires a non-negative Height, but the supplied value was {}.", height))};
  }

  // requireScalar=true: the grayscale reconstruction engine is defined on scalar images (matching the legacy ITK
  // operator); the shared preflight also validates type membership and the geometry/tuple match.
  return {ImageProcessing::PreflightImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                           /*requireScalar=*/true)};
}
Result<> HMaximaImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto height = filterArgs.value<float64>(k_Height_Key);

  // HMaxima == reconstruct-by-dilation of the saturating (input - Height) marker under the input mask; there is no
  // FullyConnected option (the legacy ITK filter is fixed face-connectivity).
  return ImageProcessing::ExecuteMorphologicalReconstructionImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, ImageProcessing::ReconstructOp::Dilation,
                                                                        /*fullyConnected=*/false, ImageProcessing::detail::ReconMarker::MinusHeight, height, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_HeightKey = "Height";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> HMaximaImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = HMaximaImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_HeightKey, k_Height_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
