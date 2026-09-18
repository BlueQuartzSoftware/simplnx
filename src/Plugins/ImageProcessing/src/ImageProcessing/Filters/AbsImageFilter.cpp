#include "AbsImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>
#include <type_traits>

using namespace nx::core;

namespace
{
struct AbsOperation
{
  template <class T, class U>
  auto makeMapOp() const
  {
    return [](T value) -> U {
      if constexpr(std::is_integral_v<T>)
      {
        if constexpr(std::is_unsigned_v<T>)
        {
          return static_cast<U>(value); // abs is the identity on unsigned types
        }
        else
        {
          // Signed integer: take abs in the INTEGER domain (matches itk::Math::abs) instead of round-tripping
          // through float64, which loses precision for values beyond 2^53 (e.g. int64). abs(lowest()) is not
          // representable and remains undefined here exactly as in ITK (Math::abs has the same limitation).
          return static_cast<U>(value < 0 ? -value : value);
        }
      }
      else
      {
        return static_cast<U>(std::abs(static_cast<double>(value)));
      }
    };
  }
};
} // namespace

namespace nx::core
{
std::string AbsImageFilter::name() const
{
  return FilterTraits<AbsImageFilter>::name;
}
std::string AbsImageFilter::className() const
{
  return FilterTraits<AbsImageFilter>::className;
}
Uuid AbsImageFilter::uuid() const
{
  return FilterTraits<AbsImageFilter>::uuid;
}
std::string AbsImageFilter::humanName() const
{
  return "Abs Image Filter";
}
std::vector<std::string> AbsImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Abs", "Pointwise"};
}
Parameters AbsImageFilter::parameters() const
{
  Parameters params;
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
IFilter::VersionType AbsImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer AbsImageFilter::clone() const
{
  return std::make_unique<AbsImageFilter>();
}
IFilter::PreflightResult AbsImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                       const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> AbsImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                     const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  const AbsOperation operation{};
  return ImageProcessing::ExecuteImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> AbsImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = AbsImageFilter().getDefaultArguments();
  std::vector<Result<>> results;
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));
  Result<> conversionResult = MergeResults(std::move(results));
  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
