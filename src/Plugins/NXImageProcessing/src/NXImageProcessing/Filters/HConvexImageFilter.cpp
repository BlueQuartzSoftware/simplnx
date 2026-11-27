#include "HConvexImageFilter.hpp"

#include "NXImageProcessing/utils/ImageFilterExecutors.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingOperationTypes.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/format.h>

#include <cmath>
#include <memory>

using namespace nx::core;

namespace
{
// Error code for this filter. HMaxima uses -8551 and HMinima uses -8552; HConvex uses the next free slot -8553 for
// its Height guards: -8553 (non-finite) and -8556 (negative).
constexpr int32 k_NonFiniteHeight = -8553;
constexpr int32 k_NegativeHeight = -8556;
constexpr int32 k_FractionalIntegerHeight = -29101;
} // namespace

namespace nx::core
{
std::string HConvexImageFilter::name() const
{
  return FilterTraits<HConvexImageFilter>::name;
}
std::string HConvexImageFilter::className() const
{
  return FilterTraits<HConvexImageFilter>::className;
}
Uuid HConvexImageFilter::uuid() const
{
  return FilterTraits<HConvexImageFilter>::uuid;
}
std::string HConvexImageFilter::humanName() const
{
  return "H Convex Image Filter";
}
std::vector<std::string> HConvexImageFilter::defaultTags() const
{
  return {className(), "NXImageProcessing", "HConvex", "MathematicalMorphology", "Morphology"};
}
Parameters HConvexImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64ParameterType>(
      k_Height_Key, "Height", "Maximum contrast of a regional maximum. Height must be finite and nonnegative. Integer input requires a whole number. Floating-point input allows fractions.", 2.0));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the connected components are defined strictly by face connectivity (Off) or by face+edge+vertex connectivity (On). Default is Off. For "
                                                "maxima that are 1 pixel wide, use On.",
                                                false));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}
IFilter::VersionType HConvexImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointerType HConvexImageFilter::clone() const
{
  return std::make_unique<HConvexImageFilter>();
}
IFilter::PreflightResult HConvexImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                           const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto height = filterArgs.value<float64>(k_Height_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // The marker offset must be finite for reconstruction.
  if(!std::isfinite(height))
  {
    return {.output_actions = MakeErrorResult<OutputActions>(k_NonFiniteHeight, fmt::format("H Convex requires a finite Height, but the supplied value was {}.", height))};
  }
  // A negative Height has no meaning here and would push the internal (input - Height) HMaxima marker ABOVE the
  // mask, violating reconstruction-by-dilation's marker<=mask precondition (ITK throws). Require Height >= 0; 0 is
  // a valid offset and produces zero contrast.
  if(height < 0.0)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(k_NegativeHeight, fmt::format("H Convex requires a non-negative Height, but the supplied value was {}.", height))};
  }
  const auto* inputArrayPtr = dataStructure.getDataAs<IDataArray>(selectedInputArray);
  if(inputArrayPtr != nullptr && ImageProcessing::GetIntegerScalarTypes().contains(inputArrayPtr->getDataType()) && std::trunc(height) != height)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                k_FractionalIntegerHeight, fmt::format("H Convex requires a whole-number Height for integer input array '{}', but received {}. Choose a whole-number Height or explicitly "
                                                       "convert the input to floating point before using a fractional Height.",
                                                       selectedInputArray.toString(), height))};
  }

  // requireScalar=true: the grayscale reconstruction engine is defined on scalar images (matching the legacy ITK
  // operator); the shared preflight also validates type membership and the geometry/tuple match.
  return {.output_actions =
              ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                            /*requireScalar=*/true)};
}
Result<> HConvexImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                         const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto height = filterArgs.value<float64>(k_Height_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);

  // HConvex == input - HMaxima(input, Height): reconstruct-by-dilation of the saturating (input - Height) marker,
  // then subtract that h-maxima result from the input (the h-dome). FullyConnected selects the reconstruction
  // connectivity.
  return ImageProcessing::detail::ExecuteMorphologicalReconstructionWithValidatedHeight(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, ImageProcessing::ReconstructOp::Dilation,
                                                                                        fullyConnected, ImageProcessing::detail::ReconMarker::MinusHeight, height, shouldCancel, messageHandler,
                                                                                        /*differenceFromInput=*/true);
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_HeightKey = "Height";
constexpr StringLiteralType k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteralType k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteralType k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace simpl
} // namespace

Result<Arguments> HConvexImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = HConvexImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DoubleFilterParameterConverterType>(args, json, simpl::k_HeightKey, k_Height_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::StringFilterParameterConverter>(args, json, simpl::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
