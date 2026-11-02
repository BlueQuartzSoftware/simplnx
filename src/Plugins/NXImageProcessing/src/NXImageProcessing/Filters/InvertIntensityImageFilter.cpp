#include "InvertIntensityImageFilter.hpp"

#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

#include <cmath>
#include <limits>
#include <type_traits>

using namespace nx::core;

namespace
{
template <class T, class U>
struct InvertIntensityMapOp
{
  float64 maximum = 255.0;
  bool useIntegerArithmetic = false;
  int64 integerMaximum = 255;

  U operator()(T value) const
  {
    if constexpr(std::is_integral_v<T> && std::is_integral_v<U> && sizeof(T) <= sizeof(int32) && sizeof(U) <= sizeof(int32))
    {
      if(useIntegerArithmetic)
      {
        const int64 inverted = integerMaximum - static_cast<int64>(value);
        if(inverted < static_cast<int64>(std::numeric_limits<U>::lowest()))
        {
          return std::numeric_limits<U>::lowest();
        }
        if(inverted > static_cast<int64>(std::numeric_limits<U>::max()))
        {
          return std::numeric_limits<U>::max();
        }
        return static_cast<U>(inverted);
      }
    }
    return ImageProcessing::detail::SaturateCastFromDouble<U>(maximum - static_cast<float64>(value));
  }
};

struct InvertIntensityOperation
{
  float64 maximum = 255.0;

  template <class T, class U>
  auto makeMapOp() const
  {
    bool useIntegerArithmetic = false;
    int64 integerMaximum = 0;
    if constexpr(std::is_integral_v<T> && std::is_integral_v<U> && sizeof(T) <= sizeof(int32) && sizeof(U) <= sizeof(int32))
    {
      constexpr float64 k_Int64UpperExclusive = 9223372036854775808.0;
      constexpr float64 k_Int64Lowest = -9223372036854775808.0;
      const float64 minimumInverted = maximum - static_cast<float64>(std::numeric_limits<T>::max());
      const float64 maximumInverted = maximum - static_cast<float64>(std::numeric_limits<T>::lowest());
      useIntegerArithmetic = std::isfinite(maximum) && std::trunc(maximum) == maximum && minimumInverted >= k_Int64Lowest && maximumInverted < k_Int64UpperExclusive;
      if(useIntegerArithmetic)
      {
        integerMaximum = static_cast<int64>(maximum);
      }
    }
    // The integer path avoids a floating-point saturating conversion for common integral maxima while preserving
    // the general Float64 parameter semantics through the fallback.
    return InvertIntensityMapOp<T, U>{maximum, useIntegerArithmetic, integerMaximum};
  }
};
} // namespace

namespace nx::core
{
std::string InvertIntensityImageFilter::name() const
{
  return FilterTraits<InvertIntensityImageFilter>::name;
}
std::string InvertIntensityImageFilter::className() const
{
  return FilterTraits<InvertIntensityImageFilter>::className;
}
Uuid InvertIntensityImageFilter::uuid() const
{
  return FilterTraits<InvertIntensityImageFilter>::uuid;
}
std::string InvertIntensityImageFilter::humanName() const
{
  return "Invert Intensity Image Filter";
}
std::vector<std::string> InvertIntensityImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "InvertIntensity", "Pointwise"};
}
Parameters InvertIntensityImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<Float64Parameter>(k_Maximum_Key, "Maximum", "Set/Get the maximum intensity value for the inversion.", 255.0));

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
IFilter::VersionType InvertIntensityImageFilter::parametersVersion() const
{
  return 1;
}
IFilter::UniquePointer InvertIntensityImageFilter::clone() const
{
  return std::make_unique<InvertIntensityImageFilter>();
}
IFilter::PreflightResult InvertIntensityImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                   const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightFullOverwriteImageFilter(dataStructure, selectedInputArray, imageGeomPath, outputArrayPath);
  return {std::move(resultOutputActions)};
}
Result<> InvertIntensityImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  const DataPath outputArrayPath = selectedInputArray.replaceName(outputArrayName);

  auto maximum = filterArgs.value<float64>(k_Maximum_Key);

  const InvertIntensityOperation operation{maximum};
  return ImageProcessing::ExecuteImageFilter(dataStructure, selectedInputArray, outputArrayPath, operation, shouldCancel, messageHandler);
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_MaximumKey = "Maximum";
constexpr StringLiteral k_SelectedCellArrayPathKey = "SelectedCellArrayPath";
constexpr StringLiteral k_NewCellArrayNameKey = "NewCellArrayName";
} // namespace SIMPL
} // namespace

Result<Arguments> InvertIntensityImageFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = InvertIntensityImageFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DoubleFilterParameterConverter>(args, json, SIMPL::k_MaximumKey, k_Maximum_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataContainerSelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageGeomPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_SelectedCellArrayPathKey, k_InputImageDataPath_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::StringFilterParameterConverter>(args, json, SIMPL::k_NewCellArrayNameKey, k_OutputImageArrayName_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
