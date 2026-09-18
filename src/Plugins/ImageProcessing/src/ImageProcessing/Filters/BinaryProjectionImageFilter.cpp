#include "BinaryProjectionImageFilter.hpp"

#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingConstants.hpp"
#include "simplnx/Utilities/ImageProcessing/ImageProcessingFilterUtilities.hpp"
#include "simplnx/Utilities/ImageProcessing/ProjectionReducers.hpp"

#include <fmt/core.h>

#include <array>
#include <cmath>
#include <limits>
#include <utility>

using namespace nx::core;

namespace
{
// Error/warning codes for the foreground/background-value guard below (verified unused in the codebase).
constexpr int32 k_NonFiniteBinaryValue = -8350;
constexpr int32 k_BinaryValueOutOfRange = -8351;
constexpr int32 k_BinaryValueTruncated = -8352;

/**
 * @brief Validates the Float64 foreground/background parameters against an INTEGER input element type T.
 *        BinaryReduce casts these values to T; for an integral T a non-finite or out-of-[lowest, max]
 *        value is an UNDEFINED float->integral conversion, so those are rejected here. A value with a
 *        fractional part is representable (it truncates toward zero) and only produces a warning. Float
 *        input types need no check and never reach this functor.
 */
struct BinaryIntegerParamGuard
{
  nx::core::float64 foregroundValue;
  nx::core::float64 backgroundValue;

  template <class T>
  nx::core::Result<> operator()() const
  {
    using nx::core::MakeErrorResult;
    using nx::core::Warning;
    nx::core::Result<> result;
    const std::array<std::pair<const char*, nx::core::float64>, 2> params{{{"Foreground Value", foregroundValue}, {"Background Value", backgroundValue}}};
    const std::string typeName = nx::core::DataTypeToString(nx::core::GetDataType<T>());
    const auto lowest = static_cast<nx::core::float64>(std::numeric_limits<T>::lowest());
    const auto highest = static_cast<nx::core::float64>(std::numeric_limits<T>::max());
    for(const auto& [label, value] : params)
    {
      if(!std::isfinite(value))
      {
        return MakeErrorResult(k_NonFiniteBinaryValue, fmt::format("{} must be a finite number for the integer input image type '{}', but is {}.", label, typeName, value));
      }
      const nx::core::float64 truncated = std::trunc(value);
      if(truncated < lowest || truncated > highest)
      {
        return MakeErrorResult(k_BinaryValueOutOfRange, fmt::format("{} ({}) is outside the representable range [{}, {}] of the integer input image type '{}' and would be an undefined conversion.",
                                                                    label, value, lowest, highest, typeName));
      }
      if(truncated != value)
      {
        result.warnings().push_back(Warning{k_BinaryValueTruncated, fmt::format("{} ({}) will be truncated to {} to match the integer input image type '{}'.", label, value, truncated, typeName)});
      }
    }
    return result;
  }
};
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string BinaryProjectionImageFilter::name() const
{
  return FilterTraits<BinaryProjectionImageFilter>::name;
}

//------------------------------------------------------------------------------
std::string BinaryProjectionImageFilter::className() const
{
  return FilterTraits<BinaryProjectionImageFilter>::className;
}

//------------------------------------------------------------------------------
Uuid BinaryProjectionImageFilter::uuid() const
{
  return FilterTraits<BinaryProjectionImageFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string BinaryProjectionImageFilter::humanName() const
{
  return "Binary Projection Image Filter";
}

//------------------------------------------------------------------------------
std::vector<std::string> BinaryProjectionImageFilter::defaultTags() const
{
  return {className(), "ImageProcessing", "Binary", "Projection", "ImageStatistics"};
}

//------------------------------------------------------------------------------
Parameters BinaryProjectionImageFilter::parameters() const
{
  Parameters params;
  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<UInt32Parameter>(k_ProjectionDimension_Key, "Projection Dimension", "The axis to project along: 0=X, 1=Y, 2=Z.", 0u));
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_RemoveOriginalGeometry_Key, "Perform In-Place", "Performs the projection in-place for the given Image Geometry", true));
  params.insert(std::make_unique<Float64Parameter>(
      k_ForegroundValue_Key, "Foreground Value",
      "The value a projected column is set to when ANY voxel along the projection axis equals this value. For an integer input image the value must be finite and within the image type's range.",
      1.0));
  params.insert(std::make_unique<Float64Parameter>(k_BackgroundValue_Key, "Background Value",
                                                   "The value a projected column is set to when NO voxel along the projection axis equals the Foreground Value. For an integer input image the value "
                                                   "must be finite and within the image type's range.",
                                                   0.0));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_InputImageGeomPath_Key, "Image Geometry", "Select the Image Geometry Group from the DataStructure.", DataPath({"Image Geometry"}),
                                                             GeometrySelectionParameter::AllowedTypes{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_InputImageDataPath_Key, "Input Cell Data", "The image data that will be processed by this filter.", DataPath{},
                                                          nx::core::ImageProcessing::GetScalarNumericTypes()));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<StringParameter>(k_OutputImageGeomName_Key, "Created Image Geometry", "The name of the projected geometry", "Projected Image"));
  params.insert(
      std::make_unique<DataObjectNameParameter>(k_OutputImageArrayName_Key, "Output Image Data Array", "The result of the processing will be stored in this Data Array.", "Output Image Data"));

  params.linkParameters(k_RemoveOriginalGeometry_Key, k_OutputImageGeomName_Key, false);

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType BinaryProjectionImageFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer BinaryProjectionImageFilter::clone() const
{
  return std::make_unique<BinaryProjectionImageFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult BinaryProjectionImageFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                    const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto performInPlace = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto outputGeomName = filterArgs.value<std::string>(k_OutputImageGeomName_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  // Binary projection preserves the element type (output == input), but unlike Min/Median it accepts the
  // full set of scalar numeric types, so the façade is instantiated with the AllNumeric type policy (the
  // default SameAsInput OutTypeMap keeps the allocated and written output types in lock-step). The scalar
  // (single-component) and tuple-consistency checks are performed inside PreflightAxisProjection.
  Result<OutputActions> resultOutputActions = ImageProcessing::PreflightAxisProjection<ImageProcessing::AllNumeric>(
      dataStructure, imageGeomPath, selectedInputArray, static_cast<usize>(projectionDimension), performInPlace, outputGeomName, outputArrayName);
  if(resultOutputActions.invalid())
  {
    return {std::move(resultOutputActions)};
  }

  // BinaryReduce casts the Float64 foreground/background parameters to the input element type. For an
  // integer input type that conversion is UNDEFINED for a non-finite or out-of-range value, so validate the
  // parameters here (the array is guaranteed present/scalar by the successful PreflightAxisProjection above).
  // Float input types can represent any Float64 value, so they are skipped.
  const DataType inputType = dataStructure.getDataRefAs<IDataArray>(selectedInputArray).getDataType();
  if(ImageProcessing::GetIntegerScalarTypes().count(inputType) != 0)
  {
    Result<> paramGuard = ImageProcessing::IntegerOnly::dispatch(inputType, BinaryIntegerParamGuard{foregroundValue, backgroundValue});
    if(paramGuard.invalid())
    {
      return {ConvertResultTo<OutputActions>(std::move(paramGuard), OutputActions{})};
    }
    for(Warning& warning : paramGuard.warnings())
    {
      resultOutputActions.warnings().push_back(std::move(warning));
    }
  }
  return {std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> BinaryProjectionImageFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                  const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  auto imageGeomPath = filterArgs.value<DataPath>(k_InputImageGeomPath_Key);
  auto selectedInputArray = filterArgs.value<DataPath>(k_InputImageDataPath_Key);
  auto outputArrayName = filterArgs.value<DataObjectNameParameter::ValueType>(k_OutputImageArrayName_Key);
  auto outputGeomName = filterArgs.value<std::string>(k_OutputImageGeomName_Key);
  auto performInPlace = filterArgs.value<bool>(k_RemoveOriginalGeometry_Key);
  auto projectionDimension = filterArgs.value<uint32>(k_ProjectionDimension_Key);
  auto foregroundValue = filterArgs.value<float64>(k_ForegroundValue_Key);
  auto backgroundValue = filterArgs.value<float64>(k_BackgroundValue_Key);

  // Resolve the output array's location via the shared helper so preflight (which allocated it) and this
  // execute (which writes it) stay byte-identical. For an in-place run this is the temporary geometry the
  // deferred swap renames afterward; the original geometry is still intact here.
  const DataPath outputArrayPath = ImageProcessing::ProjectionOutputArrayPath(dataStructure, imageGeomPath, performInPlace, outputGeomName, outputArrayName);

  const ImageProcessing::BinaryReduce reduce{.foregroundValue = foregroundValue, .backgroundValue = backgroundValue};
  return ImageProcessing::ExecuteAxisProjectionImageFilter<ImageProcessing::AllNumeric>(dataStructure, imageGeomPath, selectedInputArray, outputArrayPath, static_cast<usize>(projectionDimension),
                                                                                        reduce, shouldCancel, messageHandler);
}
} // namespace nx::core
