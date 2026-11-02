#include "RelabelComponentImageFilter.hpp"

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
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace nx::core
{
std::string RelabelComponentImageFilter::name() const
{
  return FilterTraits<RelabelComponentImageFilter>::name;
}
std::string RelabelComponentImageFilter::className() const
{
  return FilterTraits<RelabelComponentImageFilter>::className;
}
Uuid RelabelComponentImageFilter::uuid() const
{
  return FilterTraits<RelabelComponentImageFilter>::uuid;
}
std::string RelabelComponentImageFilter::humanName() const
{
  return "Relabel Component Image Filter";
}
std::vector<std::string> RelabelComponentImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Segmentation", "Labeling"};
}

Parameters RelabelComponentImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt64Parameter>(
      k_MinimumObjectSize_Key, "Minimum Object Size",
      "Set the minimum size in pixels for an object. All objects smaller than this size will be discarded and will not appear in the output label map. NumberOfObjects will count only the objects "
      "whose pixel counts are greater than or equal to the minimum size. Call GetOriginalNumberOfObjects to find out how many objects were present in the original label map.",
      0u));
  params.insert(std::make_unique<BoolParameter>(k_SortByObjectSize_Key, "SortByObjectSize", "Controls whether the object labels are sorted by size. If false, initial order of labels is kept.", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetIntegerScalarTypes()));

  params.insertSeparator(Parameters::Separator{"Output Cell Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Cell Data",
                                                          "The result of the processing will be stored in this Data Array inside the same group as the input data.", "Output Image Data"));
  return params;
}

IFilter::VersionType RelabelComponentImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer RelabelComponentImageFilter::clone() const
{
  return std::make_unique<RelabelComponentImageFilter>();
}

IFilter::PreflightResult RelabelComponentImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  // requireScalar=true: relabeling operates on a scalar label image. The output is SameAsInput (type-preserving).
  // IntegerOnly admits ONLY integer scalar types (the legacy IntegerScalarPixelIdTypeList); float input is rejected.
  Result<OutputActions> resultOutputActions =
      ImageProcessing::PreflightFullOverwriteImageFilter<ImageProcessing::IntegerOnly, ImageProcessing::SameAsInput>(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath,
                                                                                                                     /*requireScalar=*/true);
  return {std::move(resultOutputActions)};
}

Result<> RelabelComponentImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto minimumObjectSize = filterArgs.value<uint64>(k_MinimumObjectSize_Key);
  auto sortByObjectSize = filterArgs.value<bool>(k_SortByObjectSize_Key);

  return ImageProcessing::ExecuteRelabelComponentImageFilter(dataStructure, selectedInputArray, outputArrayPath, minimumObjectSize, sortByObjectSize, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MinimumObjectSizeKey = "MinimumObjectSize";
constexpr StringLiteral k_SortByObjectSizeKey = "SortByObjectSize";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> RelabelComponentImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RelabelComponentImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt64FilterParameterConverter>(args, json, SIMPL::k_MinimumObjectSizeKey, k_MinimumObjectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::BooleanFilterParameterConverter>(args, json, SIMPL::k_SortByObjectSizeKey, k_SortByObjectSize_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
