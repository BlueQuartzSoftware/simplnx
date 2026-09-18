#include "OpeningByReconstructionImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/MorphologyEngine.hpp"
#include "simplnx/Utilities/ImageProcessing/StructuringElement.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string OpeningByReconstructionImageFilter::name() const
{
  return FilterTraits<OpeningByReconstructionImageFilter>::name;
}
std::string OpeningByReconstructionImageFilter::className() const
{
  return FilterTraits<OpeningByReconstructionImageFilter>::className;
}
Uuid OpeningByReconstructionImageFilter::uuid() const
{
  return FilterTraits<OpeningByReconstructionImageFilter>::uuid;
}
std::string OpeningByReconstructionImageFilter::humanName() const
{
  return "Opening By Reconstruction Image Filter";
}
std::vector<std::string> OpeningByReconstructionImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "OpeningByReconstruction", "MathematicalMorphology", "Morphology"};
}
Parameters OpeningByReconstructionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_KernelRadius_Key, "Kernel Radius", "The radius of the kernel structuring element.", std::vector<uint32>(3, 1),
                                                        std::vector<std::string>{"X", "Y", "Z"}));
  params.insert(std::make_unique<ChoicesParameter>(k_KernelType_Key, "Kernel Type", "Set the kernel or structuring element used for the morphology.",
                                                   static_cast<uint64>(ImageProcessing::KernelType::Ball), ChoicesParameter::Choices{"Annulus", "Ball", "Box", "Cross"}));
  params.insert(std::make_unique<BoolParameter>(k_FullyConnected_Key, "Fully Connected",
                                                "Whether the connected components of the reconstruction are defined strictly by face connectivity (Off) or by face+edge+vertex connectivity (On).",
                                                false));
  params.insert(
      std::make_unique<BoolParameter>(k_PreserveIntensities_Key, "Preserve Intensities", "Preserve the original intensities of the reconstructed maxima instead of the eroded values.", false));

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
IFilter::VersionType OpeningByReconstructionImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer OpeningByReconstructionImageFilter::clone() const
{
  return std::make_unique<OpeningByReconstructionImageFilter>();
}
IFilter::PreflightResult OpeningByReconstructionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                           const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: the grayscale reconstruction engine is defined on scalar images (matching the legacy ITK
  // operator); the shared preflight also validates type membership and the geometry/tuple match.
  return {ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::AllNumeric, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                        /*requireScalar=*/true)};
}
Result<> OpeningByReconstructionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                         const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto kernelRadius = filterArgs.value<VectorUInt32Parameter::ValueType>(k_KernelRadius_Key);
  auto kernelType = filterArgs.value<ChoicesParameter::ValueType>(k_KernelType_Key);
  auto fullyConnected = filterArgs.value<bool>(k_FullyConnected_Key);
  auto preserveIntensities = filterArgs.value<bool>(k_PreserveIntensities_Key);

  const ImageProcessing::StructuringElement se = ImageProcessing::MakeStructuringElement(
      static_cast<ImageProcessing::KernelType>(kernelType), {static_cast<int32>(kernelRadius[0]), static_cast<int32>(kernelRadius[1]), static_cast<int32>(kernelRadius[2])});

  // Opening by reconstruction == grayscale erode with the structuring element, then reconstruct-by-dilation of the
  // eroded image under the original input as the mask (firstOp = Erode).
  return ImageProcessing::ExecuteReconstructionOpenCloseImageFilter(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, se, ImageProcessing::MorphOp::Erode, fullyConnected,
                                                                    preserveIntensities, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_KernelTypeKey = "KernelType";
constexpr StringLiteral k_KernelRadiusKey = "KernelRadius";
constexpr StringLiteral k_FullyConnectedKey = "FullyConnected";
constexpr StringLiteral k_PreserveIntensitiesKey = "PreserveIntensities";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> OpeningByReconstructionImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = OpeningByReconstructionImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::ChoiceFilterParameterConverter>(args, json, SIMPL::k_KernelTypeKey, k_KernelType_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_KernelRadiusKey, k_KernelRadius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_FullyConnectedKey, k_FullyConnected_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_PreserveIntensitiesKey, k_PreserveIntensities_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
