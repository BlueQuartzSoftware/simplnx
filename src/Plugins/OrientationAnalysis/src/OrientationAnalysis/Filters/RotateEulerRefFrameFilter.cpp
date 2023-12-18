#include "RotateEulerRefFrameFilter.hpp"
#include "OrientationAnalysis/Filters/Algorithms/RotateEulerRefFrame.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

#include "complex/Utilities/SIMPLConversion.hpp"

#include "complex/Parameters/VectorParameter.hpp"

using namespace complex;

namespace complex
{
//------------------------------------------------------------------------------
std::string RotateEulerRefFrameFilter::name() const
{
  return FilterTraits<RotateEulerRefFrameFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string RotateEulerRefFrameFilter::className() const
{
  return FilterTraits<RotateEulerRefFrameFilter>::className;
}

//------------------------------------------------------------------------------
Uuid RotateEulerRefFrameFilter::uuid() const
{
  return FilterTraits<RotateEulerRefFrameFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string RotateEulerRefFrameFilter::humanName() const
{
  return "Rotate Euler Reference Frame";
}

//------------------------------------------------------------------------------
std::vector<std::string> RotateEulerRefFrameFilter::defaultTags() const
{
  return {className(), "Processing", "Conversion", "Euler", "Angles", "ReferenceFrame"};
}

//------------------------------------------------------------------------------
Parameters RotateEulerRefFrameFilter::parameters() const
{
  Parameters params;

  // Create the parameter descriptors that are needed for this filter
  params.insertSeparator(Parameters::Separator{"Input Parameters"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_RotationAxisAngle_Key, "Rotation Axis-Angle [<ijk>w]", "Axis-Angle in sample reference frame to rotate about.",
                                                         VectorFloat32Parameter::ValueType{0.0f, 0.0f, 0.0f, 90.0F}, std::vector<std::string>{"i", "j", "k", "w (Deg)"}));

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_EulerAnglesArrayPath_Key, "Input Euler Angles", "Three angles defining the orientation of the Cell in Bunge convention (Z-X-Z)", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{3}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer RotateEulerRefFrameFilter::clone() const
{
  return std::make_unique<RotateEulerRefFrameFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult RotateEulerRefFrameFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                  const std::atomic_bool& shouldCancel) const
{
  auto pRotationAxisAngleValue = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);
  auto pCellEulerAnglesArrayPathValue = filterArgs.value<DataPath>(k_EulerAnglesArrayPath_Key);

  complex::Result<OutputActions> resultOutputActions;

  std::vector<PreflightValue> preflightUpdatedValues;

  resultOutputActions.value().modifiedActions.emplace_back(
      DataObjectModification{pCellEulerAnglesArrayPathValue, DataObjectModification::ModifiedType::Modified, dataStructure.getData(pCellEulerAnglesArrayPathValue)->getDataObjectType()});

  // Return both the resultOutputActions and the preflightUpdatedValues via std::move()
  return {std::move(resultOutputActions), std::move(preflightUpdatedValues)};
}

//------------------------------------------------------------------------------
Result<> RotateEulerRefFrameFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                const std::atomic_bool& shouldCancel) const
{
  complex::RotateEulerRefFrameInputValues inputValues;

  inputValues.eulerAngleDataPath = filterArgs.value<DataPath>(k_EulerAnglesArrayPath_Key);
  inputValues.rotationAxis = filterArgs.value<VectorFloat32Parameter::ValueType>(k_RotationAxisAngle_Key);

  // Let the Algorithm instance do the work
  return RotateEulerRefFrame(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace SIMPL
{
constexpr StringLiteral k_RotationAngleKey = "RotationAngle";
constexpr StringLiteral k_RotationAxisKey = "RotationAxis";
constexpr StringLiteral k_CellEulerAnglesArrayPathKey = "CellEulerAnglesArrayPath";
} // namespace SIMPL
} // namespace

Result<Arguments> RotateEulerRefFrameFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = RotateEulerRefFrameFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(
      SIMPLConversion::Convert2Parameters<SIMPLConversion::FloatVec3p1FilterParameterConverter>(args, json, SIMPL::k_RotationAxisKey, SIMPL::k_RotationAngleKey, k_RotationAxisAngle_Key));
  results.push_back(SIMPLConversion::ConvertParameter<SIMPLConversion::DataArraySelectionFilterParameterConverter>(args, json, SIMPL::k_CellEulerAnglesArrayPathKey, k_EulerAnglesArrayPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace complex
