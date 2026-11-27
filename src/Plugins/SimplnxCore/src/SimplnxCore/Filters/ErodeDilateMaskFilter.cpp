#include "ErodeDilateMaskFilter.hpp"

#include "SimplnxCore/Filters/Algorithms/ErodeDilateMask.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"
#include "simplnx/Utilities/SIMPLConversion.hpp"

using namespace nx::core;

namespace
{
constexpr int32 k_InvalidNumIterationsError = -14701;
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::name() const
{
  return FilterTraits<ErodeDilateMaskFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::className() const
{
  return FilterTraits<ErodeDilateMaskFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ErodeDilateMaskFilter::uuid() const
{
  return FilterTraits<ErodeDilateMaskFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ErodeDilateMaskFilter::humanName() const
{
  return "Erode/Dilate Mask";
}

//------------------------------------------------------------------------------
std::vector<std::string> ErodeDilateMaskFilter::defaultTags() const
{
  return {className(), "Processing", "Cleanup", "Erode", "Dilate"};
}

//------------------------------------------------------------------------------
Parameters ErodeDilateMaskFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});

  // Create the parameter descriptors that are needed for this filter
  params.insert(std::make_unique<ChoicesParameter>(k_Operation_Key, "Operation", "Whether to dilate (0) or erode (1)", 0ULL, detail::k_OperationChoices));
  params.insert(std::make_unique<Int32ParameterType>(k_NumIterations_Key, "Number of Iterations", "Number of erode/dilate iterations to perform", 1));
  params.insert(std::make_unique<BoolParameter>(k_XDirOn_Key, "X Direction", "Whether to erode/dilate in the X direction", true));
  params.insert(std::make_unique<BoolParameter>(k_YDirOn_Key, "Y Direction", "Whether to erode/dilate in the Y direction", true));
  params.insert(std::make_unique<BoolParameter>(k_ZDirOn_Key, "Z Direction", "Whether to erode/dilate in the Z direction", true));

  params.insertSeparator(Parameters::Separator{"Input Cell Data"});
  params.insert(std::make_unique<GeometrySelectionParameter>(k_SelectedImageGeometryPath_Key, "Selected Image Geometry", "The target geometry", DataPath{},
                                                             GeometrySelectionParameter::AllowedTypeSetType{IGeometry::Type::Image}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array Path", "Boolean array where true voxels are used. False voxels are ignored.", DataPath{},
                                                          GetDataArrayOnlyTypes(), ArraySelectionParameter::AllowedDataTypes{DataType::boolean},
                                                          ArraySelectionParameter::AllowedComponentShapesType{{1}}));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ErodeDilateMaskFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointerType ErodeDilateMaskFilter::clone() const
{
  return std::make_unique<ErodeDilateMaskFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ErodeDilateMaskFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& /*messageHandler*/,
                                                              const std::atomic_bool& /*shouldCancel*/, const ExecutionContext& /*executionContext*/) const
{
  auto pMaskArrayPathValue = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  auto pNumIterationsValue = filterArgs.value<int32>(k_NumIterations_Key);

  auto tupleCountResult = ValidateCellTupleCount(dataStructure, filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key), {pMaskArrayPathValue}, -14702);
  if(tupleCountResult.invalid())
  {
    return {.output_actions = ConvertResultTo<OutputActions>(std::move(tupleCountResult), {})};
  }

  // The erode/dilate loop runs NumIterations times, so a value below 1 performs no passes and returns
  // the mask untouched. DREAM3D 6.5.171 rejected that in dataCheck rather than silently no-op'ing, and
  // this guard restores that behavior so a mistyped iteration count is reported to the user.
  if(pNumIterationsValue < 1)
  {
    return {.output_actions = MakeErrorResult<OutputActions>(
                k_InvalidNumIterationsError, fmt::format("Number of Iterations ({}) must be at least 1. Erode/Dilate Mask performs one erode or dilate pass per iteration, so a value less than 1 "
                                                         "would leave the mask array '{}' unmodified.",
                                                         pNumIterationsValue, pMaskArrayPathValue.toString()))};
  }

  nx::core::Result<OutputActions> resultOutputActions;

  // Inform users that the following arrays are going to be modified in place
  // Mask array is going to be modified
  nx::core::MarkDataPathModified(dataStructure, resultOutputActions, pMaskArrayPathValue);

  return {.output_actions = std::move(resultOutputActions)};
}

//------------------------------------------------------------------------------
Result<> ErodeDilateMaskFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* /*pipelineNode*/, const MessageHandler& messageHandler,
                                            const std::atomic_bool& shouldCancel, const ExecutionContext& /*executionContext*/) const
{
  ErodeDilateMaskInputValues inputValues;

  inputValues.operation = filterArgs.value<ChoicesParameter::ValueType>(k_Operation_Key);
  inputValues.num_iterations = filterArgs.value<int32>(k_NumIterations_Key);
  inputValues.x_dir_on = filterArgs.value<bool>(k_XDirOn_Key);
  inputValues.y_dir_on = filterArgs.value<bool>(k_YDirOn_Key);
  inputValues.z_dir_on = filterArgs.value<bool>(k_ZDirOn_Key);
  inputValues.mask_array_path = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.input_image_geometry = filterArgs.value<DataPath>(k_SelectedImageGeometryPath_Key);

  return ErodeDilateMask(dataStructure, messageHandler, shouldCancel, &inputValues)();
}

namespace
{
namespace simpl
{
constexpr StringLiteralType k_DirectionKey = "Direction";
constexpr StringLiteralType k_NumIterationsKey = "NumIterations";
constexpr StringLiteralType k_XDirOnKey = "XDirOn";
constexpr StringLiteralType k_YDirOnKey = "YDirOn";
constexpr StringLiteralType k_ZDirOnKey = "ZDirOn";
constexpr StringLiteralType k_MaskArrayPathKey = "MaskArrayPath";
} // namespace simpl
} // namespace

Result<Arguments> ErodeDilateMaskFilter::FromSIMPLJson(const nlohmann::json& json)
{
  Arguments args = ErodeDilateMaskFilter().getDefaultArguments();

  std::vector<Result<>> results;

  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::ChoiceFilterParameterConverter>(args, json, simpl::k_DirectionKey, k_Operation_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::IntFilterParameterConverter<int32>>(args, json, simpl::k_NumIterationsKey, k_NumIterations_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_XDirOnKey, k_XDirOn_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_YDirOnKey, k_YDirOn_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::BooleanFilterParameterConverter>(args, json, simpl::k_ZDirOnKey, k_ZDirOn_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataArraySelectionFilterParameterConverter>(args, json, simpl::k_MaskArrayPathKey, k_MaskArrayPath_Key));
  results.push_back(simpl_conversion::ConvertParameter<simpl_conversion::DataContainerSelectionFilterParameterConverter>(args, json, simpl::k_MaskArrayPathKey, k_SelectedImageGeometryPath_Key));

  Result<> conversionResult = MergeResults(std::move(results));

  return ConvertResultTo<Arguments>(std::move(conversionResult), std::move(args));
}
} // namespace nx::core
