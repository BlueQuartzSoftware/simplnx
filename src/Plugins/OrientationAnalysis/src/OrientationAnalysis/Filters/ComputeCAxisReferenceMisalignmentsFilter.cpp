#include "ComputeCAxisReferenceMisalignmentsFilter.hpp"

#include "OrientationAnalysis/Filters/Algorithms/ComputeCAxisReferenceMisalignments.hpp"

#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/Actions/CreateArrayAction.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/DataObjectNameParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include <fmt/format.h>

#include <cmath>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace nx::core;

namespace
{
/**
 * @brief Builds the algorithm inputs from the filter arguments, so preflight and execute read the same paths.
 * @param filterArgs Filter arguments.
 * @return Input values, including the output path beside the quaternion array.
 */
ComputeCAxisReferenceMisalignmentsInputValues CreateInputValues(const Arguments& filterArgs)
{
  ComputeCAxisReferenceMisalignmentsInputValues inputValues;
  inputValues.ReferenceDirection = filterArgs.value<VectorFloat32Parameter::ValueType>(ComputeCAxisReferenceMisalignmentsFilter::k_ReferenceDir_Key);
  inputValues.UseMask = filterArgs.value<bool>(ComputeCAxisReferenceMisalignmentsFilter::k_UseMask_Key);
  inputValues.QuatsArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_QuatsArrayPath_Key);
  inputValues.PhasesArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_PhasesArrayPath_Key);
  inputValues.MaskArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_MaskArrayPath_Key);
  inputValues.CrystalStructuresArrayPath = filterArgs.value<DataPath>(ComputeCAxisReferenceMisalignmentsFilter::k_CrystalStructuresArrayPath_Key);
  inputValues.OutputArrayPath = inputValues.QuatsArrayPath.replaceName(filterArgs.value<std::string>(ComputeCAxisReferenceMisalignmentsFilter::k_CAxisReferenceMisalignmentsArrayName_Key));
  return inputValues;
}

/**
 * @brief Names the type of a selected object for a type error.
 * @param object Selected object.
 * @return For a DataArray, its element type (for example "float64"); otherwise the object type name (for example "StringArray").
 */
std::string DescribeObjectType(const DataObject& object)
{
  if(const auto* dataArrayPtr = dynamic_cast<const IDataArray*>(&object); dataArrayPtr != nullptr)
  {
    return DataTypeToString(dataArrayPtr->getDataType()).str();
  }
  return object.getTypeName();
}

/**
 * @brief Makes the error for a path that does not name an array of the required type.
 * @param dataStructure Holds the selected object, if any.
 * @param path Selected path.
 * @param label User-facing name of the input, for the message.
 * @param requiredType Accepted element types, quoted, for the message (for example "'bool' or 'uint8'").
 * @param errorCode Code of the error.
 * @return An error that says whether the path is missing or names the type it has.
 */
Result<> MakeArrayTypeError(const DataStructure& dataStructure, const DataPath& path, std::string_view label, std::string_view requiredType, int32 errorCode)
{
  const auto* objectPtr = dataStructure.getData(path);
  if(objectPtr == nullptr)
  {
    return MakeErrorResult(errorCode, fmt::format("{} path '{}' does not exist. Select an array of type {}.", label, path.toString(), requiredType));
  }
  return MakeErrorResult(
      errorCode, fmt::format("{} path '{}' has type '{}', but type {} is required. Select an array of type {}.", label, path.toString(), DescribeObjectType(*objectPtr), requiredType, requiredType));
}

/**
 * @brief Checks, without throwing, that a path resolves to an ArrayType.
 *
 * ArraySelectionParameter applies its type list only to IDataArray, so a StringArray reaches preflightImpl().
 * @tparam ArrayType Required concrete DataArray type.
 * @param dataStructure Holds the array.
 * @param path Path of the array.
 * @param label User-facing name of the input, for the message.
 * @param typeName Required element type, quoted, for the message (for example "'float32'").
 * @param errorCode Code returned when the path is missing or has another type.
 * @return An error naming the type found, or a valid result.
 */
template <typename ArrayType>
Result<> ValidateArrayType(const DataStructure& dataStructure, const DataPath& path, std::string_view label, std::string_view typeName, int32 errorCode)
{
  if(dataStructure.getDataAs<ArrayType>(path) == nullptr)
  {
    return MakeArrayTypeError(dataStructure, path, label, typeName, errorCode);
  }
  return {};
}

/**
 * @brief Checks that a sibling input has the tuple count of the quaternion array.
 * @param dataStructure Holds both arrays.
 * @param array Sibling input.
 * @param path Path of the sibling input.
 * @param label User-facing name of the sibling input, for the message.
 * @param quatsPath Path of the quaternion array.
 * @param quatsTupleCount Tuple count of the quaternion array.
 * @return An error naming both paths and both counts, or a valid result.
 */
Result<> ValidateTupleCount(const DataStructure& dataStructure, const IDataArray& array, const DataPath& path, std::string_view label, const DataPath& quatsPath, usize quatsTupleCount)
{
  if(!CheckArraysHaveSameTupleCount(dataStructure, {quatsPath, path}))
  {
    return MakeErrorResult(ComputeCAxisReferenceMisalignmentsFilter::k_TupleCountMismatchError,
                           fmt::format("{} array '{}' has {} tuples, but Input Quaternions array '{}' has {} tuples. Select arrays with one tuple per quaternion.", label, path.toString(),
                                       array.getNumberOfTuples(), quatsPath.toString(), quatsTupleCount));
  }
  return {};
}

/**
 * @brief Checks that the reference direction is finite and not near zero. VectorParameter guarantees three components.
 * @param referenceDirection Three sample-frame components.
 * @return k_InvalidReferenceDirectionError for a non-finite component or a near-zero norm, or a valid result.
 */
Result<> ValidateReferenceDirection(const std::vector<float32>& referenceDirection)
{
  const float64 norm = std::hypot(static_cast<float64>(referenceDirection[0]), static_cast<float64>(referenceDirection[1]), static_cast<float64>(referenceDirection[2]));
  if(!std::isfinite(referenceDirection[0]) || !std::isfinite(referenceDirection[1]) || !std::isfinite(referenceDirection[2]) ||
     norm <= ComputeCAxisReferenceMisalignmentsFilter::k_MinReferenceDirectionNorm)
  {
    return MakeErrorResult(ComputeCAxisReferenceMisalignmentsFilter::k_InvalidReferenceDirectionError,
                           fmt::format("Reference Direction [{}, {}, {}] must contain finite components and have length greater than {}. Enter a nonzero sample-frame direction, such as [0, 0, 1].",
                                       referenceDirection[0], referenceDirection[1], referenceDirection[2], ComputeCAxisReferenceMisalignmentsFilter::k_MinReferenceDirectionNorm));
  }
  return {};
}

/**
 * @brief Makes the input checks that the parameters cannot make.
 *
 * Checks that the reference direction is finite and not near zero.
 * Checks that each selected array is a DataArray of the required type.
 * Checks the mask only when UseMask is set.
 * Phases and the enabled mask must have one tuple per quaternion.
 * The parameters already check existence, element types of DataArrays and component shapes.
 * Array lookups do not throw.
 * @param dataStructure Holds the selected arrays.
 * @param inputValues Identifies the arrays and the reference direction. OutputArrayPath is not read.
 * @return The error of the first failed check, or a valid result.
 */
Result<> ValidateInputs(const DataStructure& dataStructure, const ComputeCAxisReferenceMisalignmentsInputValues& inputValues)
{
  if(auto result = ValidateReferenceDirection(inputValues.ReferenceDirection); result.invalid())
  {
    return result;
  }

  const DataPath& quatsPathRef = inputValues.QuatsArrayPath;
  if(auto result = ValidateArrayType<Float32Array>(dataStructure, quatsPathRef, "Input Quaternions", "'float32'", ComputeCAxisReferenceMisalignmentsFilter::k_InvalidQuatsArrayError); result.invalid())
  {
    return result;
  }
  const usize quatsTupleCount = dataStructure.getDataRefAs<Float32Array>(quatsPathRef).getNumberOfTuples();

  const DataPath& phasesPathRef = inputValues.PhasesArrayPath;
  if(auto result = ValidateArrayType<Int32Array>(dataStructure, phasesPathRef, "Phases", "'int32'", ComputeCAxisReferenceMisalignmentsFilter::k_InvalidPhasesArrayError); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateTupleCount(dataStructure, dataStructure.getDataRefAs<Int32Array>(phasesPathRef), phasesPathRef, "Phases", quatsPathRef, quatsTupleCount); result.invalid())
  {
    return result;
  }

  if(inputValues.UseMask)
  {
    const DataPath& maskPathRef = inputValues.MaskArrayPath;
    const auto* maskPtr = dataStructure.getDataAs<IDataArray>(maskPathRef);
    if(maskPtr == nullptr || (maskPtr->getDataType() != DataType::boolean && maskPtr->getDataType() != DataType::uint8))
    {
      return MakeArrayTypeError(dataStructure, maskPathRef, "Mask", "'bool' or 'uint8'", ComputeCAxisReferenceMisalignmentsFilter::k_InvalidMaskArrayError);
    }
    if(auto result = ValidateTupleCount(dataStructure, *maskPtr, maskPathRef, "Mask", quatsPathRef, quatsTupleCount); result.invalid())
    {
      return result;
    }
  }

  return ValidateArrayType<UInt32Array>(dataStructure, inputValues.CrystalStructuresArrayPath, "Crystal Structures", "'uint32'",
                                        ComputeCAxisReferenceMisalignmentsFilter::k_InvalidCrystalStructuresArrayError);
}
} // namespace

namespace nx::core
{
//------------------------------------------------------------------------------
std::string ComputeCAxisReferenceMisalignmentsFilter::name() const
{
  return FilterTraits<ComputeCAxisReferenceMisalignmentsFilter>::name.str();
}

//------------------------------------------------------------------------------
std::string ComputeCAxisReferenceMisalignmentsFilter::className() const
{
  return FilterTraits<ComputeCAxisReferenceMisalignmentsFilter>::className;
}

//------------------------------------------------------------------------------
Uuid ComputeCAxisReferenceMisalignmentsFilter::uuid() const
{
  return FilterTraits<ComputeCAxisReferenceMisalignmentsFilter>::uuid;
}

//------------------------------------------------------------------------------
std::string ComputeCAxisReferenceMisalignmentsFilter::humanName() const
{
  return "Compute C-Axis Reference Misalignments";
}

//------------------------------------------------------------------------------
std::vector<std::string> ComputeCAxisReferenceMisalignmentsFilter::defaultTags() const
{
  return {className(), "Statistics", "Crystallography", "C-Axis", "Misalignment", "Hexagonal"};
}

//------------------------------------------------------------------------------
Parameters ComputeCAxisReferenceMisalignmentsFilter::parameters() const
{
  Parameters params;

  params.insertSeparator(Parameters::Separator{"Input Parameter(s)"});
  params.insert(std::make_unique<VectorFloat32Parameter>(k_ReferenceDir_Key, "Reference Direction", "Sample-frame direction to compare with the crystal c-axis. Must be finite and nonzero.",
                                                         std::vector<float32>{0.0F, 0.0F, 1.0F}, std::vector<std::string>{"X", "Y", "Z"}));

  params.insertSeparator(Parameters::Separator{"Optional Data Mask"});
  params.insertLinkableParameter(std::make_unique<BoolParameter>(k_UseMask_Key, "Use Mask Array", "Whether to assign NaN to tuples where the mask is false or zero", false));
  params.insert(std::make_unique<ArraySelectionParameter>(k_MaskArrayPath_Key, "Mask Array", "Boolean or uint8 array. True or nonzero tuples are included.", DataPath(),
                                                          ArraySelectionParameter::AllowedTypes{DataType::boolean, DataType::uint8}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  // Associate the Linkable Parameter(s) to the children parameters that they control
  params.linkParameters(k_UseMask_Key, k_MaskArrayPath_Key, true);

  params.insertSeparator(Parameters::Separator{"Input Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_QuatsArrayPath_Key, "Input Quaternions (<x, y, z>, w)",
                                                          "Quaternions in (<x, y, z>, w) order. Select cell Quats, feature AvgQuats, or another quaternion array.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::float32}, ArraySelectionParameter::AllowedComponentShapes{{4}}));
  params.insert(std::make_unique<ArraySelectionParameter>(k_PhasesArrayPath_Key, "Phases", "Ensemble index for each quaternion tuple. Must have the same tuple count as Input Quaternions.", DataPath{},
                                                          ArraySelectionParameter::AllowedTypes{DataType::int32}, ArraySelectionParameter::AllowedComponentShapes{{1}}));
  params.insertSeparator(Parameters::Separator{"Input Ensemble Data"});
  params.insert(std::make_unique<ArraySelectionParameter>(k_CrystalStructuresArrayPath_Key, "Crystal Structures", "Enumeration representing the crystal structure for each Ensemble",
                                                          DataPath({"Ensemble Data", "CrystalStructures"}), ArraySelectionParameter::AllowedTypes{DataType::uint32},
                                                          ArraySelectionParameter::AllowedComponentShapes{{1}}));

  params.insertSeparator(Parameters::Separator{"Output Data"});
  params.insert(std::make_unique<DataObjectNameParameter>(k_CAxisReferenceMisalignmentsArrayName_Key, "C-Axis Reference Misalignments",
                                                          "Output angles in degrees, in [0, 90]. Created beside Input Quaternions.", "CAxisReferenceMisalignments"));

  return params;
}

//------------------------------------------------------------------------------
IFilter::VersionType ComputeCAxisReferenceMisalignmentsFilter::parametersVersion() const
{
  return 1;
}

//------------------------------------------------------------------------------
IFilter::UniquePointer ComputeCAxisReferenceMisalignmentsFilter::clone() const
{
  return std::make_unique<ComputeCAxisReferenceMisalignmentsFilter>();
}

//------------------------------------------------------------------------------
IFilter::PreflightResult ComputeCAxisReferenceMisalignmentsFilter::preflightImpl(const DataStructure& dataStructure, const Arguments& filterArgs, const MessageHandler& messageHandler,
                                                                                 const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const ComputeCAxisReferenceMisalignmentsInputValues inputValues = CreateInputValues(filterArgs);
  Result<> validationResult = ValidateInputs(dataStructure, inputValues);
  if(validationResult.invalid())
  {
    return {ConvertResultTo<OutputActions>(std::move(validationResult), {})};
  }
  std::vector<std::pair<std::string_view, DataPath>> inputPaths = {
      {"Input Quaternions", inputValues.QuatsArrayPath}, {"Phases", inputValues.PhasesArrayPath}, {"Crystal Structures", inputValues.CrystalStructuresArrayPath}};
  if(inputValues.UseMask)
  {
    inputPaths.emplace_back("Mask", inputValues.MaskArrayPath);
  }
  for(const auto& inputRef : inputPaths)
  {
    if(inputValues.OutputArrayPath == inputRef.second)
    {
      return {MakeErrorResult<OutputActions>(k_OutputPathCollisionError,
                                             fmt::format("The output array path '{}' is the path of the selected {} array. Change the C-Axis Reference Misalignments name '{}' to a "
                                                         "name that no input array uses.",
                                                         inputValues.OutputArrayPath.toString(), inputRef.first, inputValues.OutputArrayPath.getTargetName()))};
    }
  }
  const auto& quatsRef = dataStructure.getDataRefAs<Float32Array>(inputValues.QuatsArrayPath);
  OutputActions actions;
  actions.appendAction(std::make_unique<CreateArrayAction>(DataType::float32, quatsRef.getIDataStoreRef().getTupleShape(), ShapeType{1}, inputValues.OutputArrayPath));
  return {std::move(actions)};
}

//------------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignmentsFilter::executeImpl(DataStructure& dataStructure, const Arguments& filterArgs, const PipelineFilter* pipelineNode, const MessageHandler& messageHandler,
                                                               const std::atomic_bool& shouldCancel, const ExecutionContext& executionContext) const
{
  const ComputeCAxisReferenceMisalignmentsInputValues inputValues = CreateInputValues(filterArgs);
  return ComputeCAxisReferenceMisalignments(dataStructure, messageHandler, shouldCancel, &inputValues)();
}
} // namespace nx::core
