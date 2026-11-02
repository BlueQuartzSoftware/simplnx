#include "MedianImageFilter.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <fmt/core.h>

#include <algorithm>

using namespace nx::core;

namespace
{
struct MedianReduce
{
  template <class T>
  auto makeReduceOp() const
  {
    return [](nonstd::span<T> scratch) -> T {
      // nth_element at size()/2 (upper-middle for even counts) reproduces ITK's median index. NaN note: for a
      // float T a neighborhood containing NaN makes operator< a non-strict-weak-ordering, so nth_element is
      // formally UB (returns an arbitrary element in practice). This is NOT a regression -- itk::MedianImageFilter
      // does the identical std::nth_element with the same exposure -- so it is left as-is for ITK parity.
      const auto middle = scratch.begin() + scratch.size() / 2;
      std::nth_element(scratch.begin(), middle, scratch.end());
      return *middle;
    };
  }
};
} // namespace

namespace nx::core
{
std::string MedianImageFilter::name() const
{
  return FilterTraits<MedianImageFilter>::name;
}
std::string MedianImageFilter::className() const
{
  return FilterTraits<MedianImageFilter>::className;
}
Uuid MedianImageFilter::uuid() const
{
  return FilterTraits<MedianImageFilter>::uuid;
}
std::string MedianImageFilter::humanName() const
{
  return "Median Image Filter";
}
std::vector<std::string> MedianImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Median", "Neighborhood"};
}
Parameters MedianImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorUInt32Parameter>(k_Radius_Key, "Radius", "Radius Dimensions XYZ", std::vector<uint32>(3, 1), std::vector<std::string>{"X", "Y", "Z"}));

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
IFilter::VersionType MedianImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer MedianImageFilter::clone() const
{
  return std::make_unique<MedianImageFilter>();
}
IFilter::PreflightResult MedianImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                          const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const auto& inputArray = dataStructure.getDataRefAs<IDataArray>(selectedInputArray);
  if(inputArray.getNumberOfComponents() != 1)
  {
    return {MakeErrorResult<OutputActions>(
        -8300, fmt::format("Median requires a single-component (scalar) input array, but '{}' has {} components.", selectedInputArray.toString(), inputArray.getNumberOfComponents()))};
  }
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> MedianImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                        const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto radiusVec = filterArgs.value<VectorUInt32Parameter::ValueType>(k_Radius_Key);
  const std::array<usize, 3> radius = {static_cast<usize>(radiusVec[0]), static_cast<usize>(radiusVec[1]), static_cast<usize>(radiusVec[2])};

  const MedianReduce operation{};
  return ImageProcessing::ExecuteBoxNeighborhoodImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath, radius, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_RadiusKey = "Radius";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> MedianImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = MedianImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::UInt32Vec3FilterParameterConverter>(args, json, SIMPL::k_RadiusKey, k_Radius_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
