#include "ComputeCAxisReferenceMisalignments.hpp"

#include "simplnx/Common/Numbers.hpp"
#include "simplnx/Common/TypesUtility.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <EbsdLib/Core/EbsdLibConstants.h>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <fmt/ranges.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

using namespace nx::core;

namespace
{
constexpr usize k_ChunkTuples = 4096;
constexpr std::string_view k_ProgressLabel = "Computing C-Axis Reference Misalignments";

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
 * @brief Checks that an array has exactly the required component shape.
 * @param array Array to check.
 * @param path Path of the array, for the message.
 * @param label User-facing name of the input, for the message.
 * @param componentShape Required component shape.
 * @param errorCode Code returned when the shape differs.
 * @return An error naming both shapes, or a valid result.
 */
Result<> ValidateComponentShape(const IDataArray& array, const DataPath& path, std::string_view label, const ShapeType& componentShape, int32 errorCode)
{
  if(array.getComponentShape() != componentShape)
  {
    return MakeErrorResult(errorCode, fmt::format("{} array '{}' has component shape {{{}}}, but component shape {{{}}} is required. Select an array with component shape {{{}}}.", label,
                                                  path.toString(), fmt::join(array.getComponentShape(), ", "), fmt::join(componentShape, ", "), fmt::join(componentShape, ", ")));
  }
  return {};
}

/**
 * @brief Checks, without throwing, that a path resolves to an ArrayType with the required component shape.
 * @tparam ArrayType Required concrete DataArray type.
 * @param dataStructure Holds the array.
 * @param path Path of the array.
 * @param label User-facing name of the input, for the messages.
 * @param typeName Required element type, quoted, for the messages (for example "'float32'").
 * @param componentShape Required component shape.
 * @param typeErrorCode Code returned when the path is missing or has another type.
 * @param shapeErrorCode Code returned when the component shape differs.
 * @return The first failed check, or a valid result.
 */
template <typename ArrayType>
Result<> ValidateArray(const DataStructure& dataStructure, const DataPath& path, std::string_view label, std::string_view typeName, const ShapeType& componentShape, int32 typeErrorCode,
                       int32 shapeErrorCode)
{
  const auto* arrayPtr = dataStructure.getDataAs<ArrayType>(path);
  if(arrayPtr == nullptr)
  {
    return MakeArrayTypeError(dataStructure, path, label, typeName, typeErrorCode);
  }
  return ValidateComponentShape(*arrayPtr, path, label, componentShape, shapeErrorCode);
}

/**
 * @brief Checks that a sibling input has the tuple count of the quaternion array.
 * @param dataStructure Holds both arrays.
 * @param array Sibling input.
 * @param path Path of the sibling input.
 * @param label User-facing name of the sibling input, for the message.
 * @param quatsPath Path of the quaternion array.
 * @param quatsTupleCount Tuple count of the quaternion array.
 * @param errorCode Code returned when the counts differ.
 * @return An error naming both paths and both counts, or a valid result.
 */
Result<> ValidateTupleCount(const DataStructure& dataStructure, const IDataArray& array, const DataPath& path, std::string_view label, const DataPath& quatsPath, usize quatsTupleCount,
                            int32 errorCode)
{
  if(!CheckArraysHaveSameTupleCount(dataStructure, {quatsPath, path}))
  {
    return MakeErrorResult(errorCode, fmt::format("{} array '{}' has {} tuples, but Input Quaternions array '{}' has {} tuples. Select arrays with one tuple per quaternion.", label, path.toString(),
                                                  array.getNumberOfTuples(), quatsPath.toString(), quatsTupleCount));
  }
  return {};
}
} // namespace

// -----------------------------------------------------------------------------
ComputeCAxisReferenceMisalignments::ComputeCAxisReferenceMisalignments(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel,
                                                                       const ComputeCAxisReferenceMisalignmentsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_MessageHandler(messageHandler)
, m_ShouldCancel(shouldCancel)
, m_InputValues(inputValues)
{
}

// -----------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignments::ValidateReferenceDirection(const std::vector<float32>& referenceDirection)
{
  if(referenceDirection.size() != 3)
  {
    return MakeErrorResult(k_ReferenceDirectionSizeError, fmt::format("Reference Direction [{}] has {} components. Enter exactly three sample-frame components [X, Y, Z].",
                                                                      fmt::join(referenceDirection, ", "), referenceDirection.size()));
  }
  const float64 norm = std::hypot(static_cast<float64>(referenceDirection[0]), static_cast<float64>(referenceDirection[1]), static_cast<float64>(referenceDirection[2]));
  if(!std::isfinite(referenceDirection[0]) || !std::isfinite(referenceDirection[1]) || !std::isfinite(referenceDirection[2]) || norm <= k_MinReferenceDirectionNorm)
  {
    return MakeErrorResult(k_InvalidReferenceDirectionError,
                           fmt::format("Reference Direction [{}, {}, {}] must contain finite components and have length greater than {}. Enter a nonzero sample-frame direction, such as [0, 0, 1].",
                                       referenceDirection[0], referenceDirection[1], referenceDirection[2], k_MinReferenceDirectionNorm));
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignments::ValidateInputs(const DataStructure& dataStructure, const ComputeCAxisReferenceMisalignmentsInputValues& inputValues)
{
  if(auto result = ValidateReferenceDirection(inputValues.ReferenceDirection); result.invalid())
  {
    return result;
  }

  const DataPath& quatsPathRef = inputValues.QuatsArrayPath;
  if(auto result = ValidateArray<Float32Array>(dataStructure, quatsPathRef, "Input Quaternions", "'float32'", ShapeType{4}, k_InvalidQuatsArrayError, k_QuatsComponentShapeError); result.invalid())
  {
    return result;
  }
  const usize quatsTupleCount = dataStructure.getDataRefAs<Float32Array>(quatsPathRef).getNumberOfTuples();

  const DataPath& phasesPathRef = inputValues.PhasesArrayPath;
  if(auto result = ValidateArray<Int32Array>(dataStructure, phasesPathRef, "Phases", "'int32'", ShapeType{1}, k_InvalidPhasesArrayError, k_PhasesComponentShapeError); result.invalid())
  {
    return result;
  }
  if(auto result = ValidateTupleCount(dataStructure, dataStructure.getDataRefAs<Int32Array>(phasesPathRef), phasesPathRef, "Phases", quatsPathRef, quatsTupleCount, k_TupleCountMismatchError);
     result.invalid())
  {
    return result;
  }

  if(inputValues.UseMask)
  {
    const DataPath& maskPathRef = inputValues.MaskArrayPath;
    const auto* maskPtr = dataStructure.getDataAs<IDataArray>(maskPathRef);
    if(maskPtr == nullptr || (maskPtr->getDataType() != DataType::boolean && maskPtr->getDataType() != DataType::uint8))
    {
      return MakeArrayTypeError(dataStructure, maskPathRef, "Mask", "'bool' or 'uint8'", k_InvalidMaskArrayError);
    }
    if(auto result = ValidateComponentShape(*maskPtr, maskPathRef, "Mask", ShapeType{1}, k_MaskComponentShapeError); result.invalid())
    {
      return result;
    }
    if(auto result = ValidateTupleCount(dataStructure, *maskPtr, maskPathRef, "Mask", quatsPathRef, quatsTupleCount, k_TupleCountMismatchError); result.invalid())
    {
      return result;
    }
  }

  return ValidateArray<UInt32Array>(dataStructure, inputValues.CrystalStructuresArrayPath, "Crystal Structures", "'uint32'", ShapeType{1}, k_InvalidCrystalStructuresArrayError,
                                    k_CrystalStructuresComponentShapeError);
}

// -----------------------------------------------------------------------------
Result<> ComputeCAxisReferenceMisalignments::operator()()
{
  if(auto validation = ValidateInputs(m_DataStructure, *m_InputValues); validation.invalid())
  {
    return validation;
  }
  const auto& quatsStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const usize total = quatsStoreRef.getNumberOfTuples();
  if(auto validation = ValidateArray<Float32Array>(m_DataStructure, m_InputValues->OutputArrayPath, "Output", "'float32'", ShapeType{1}, k_InvalidOutputArrayError, k_OutputComponentShapeError);
     validation.invalid())
  {
    return validation;
  }
  auto& outputStoreRef = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->OutputArrayPath).getDataStoreRef();
  if(outputStoreRef.getNumberOfTuples() != total)
  {
    return MakeErrorResult(k_OutputTupleCountMismatchError,
                           fmt::format("Output array '{}' has {} tuples, but Input Quaternions array '{}' has {} tuples. Create the output array with one tuple per quaternion.",
                                       m_InputValues->OutputArrayPath.toString(), outputStoreRef.getNumberOfTuples(), m_InputValues->QuatsArrayPath.toString(), total));
  }

  const auto& referenceValuesRef = m_InputValues->ReferenceDirection;
  Eigen::Vector3d referenceDirection(referenceValuesRef[0], referenceValuesRef[1], referenceValuesRef[2]);
  referenceDirection.normalize();

  Result<> result;
  auto ioSucceeded = [&result](Result<> ioResult) {
    result = MergeResults(std::move(result), std::move(ioResult));
    return result.valid();
  };

  // Cache only the small ensemble array. All tuple-level buffers have a fixed bound.
  const auto& crystalStructuresStoreRef = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath).getDataStoreRef();
  std::vector<uint32> crystalStructures(crystalStructuresStoreRef.getNumberOfTuples());
  if(!ioSucceeded(crystalStructuresStoreRef.copyIntoBuffer(0, nonstd::span<uint32>(crystalStructures.data(), crystalStructures.size()))))
  {
    return result;
  }

  const auto& phasesStoreRef = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->PhasesArrayPath).getDataStoreRef();

  // ValidateInputs guarantees that an enabled mask is exactly one of these types.
  const BoolArray* boolMaskPtr = nullptr;
  const UInt8Array* byteMaskPtr = nullptr;
  if(m_InputValues->UseMask)
  {
    boolMaskPtr = m_DataStructure.getDataAs<BoolArray>(m_InputValues->MaskArrayPath);
    if(boolMaskPtr == nullptr)
    {
      byteMaskPtr = m_DataStructure.getDataAs<UInt8Array>(m_InputValues->MaskArrayPath);
    }
  }

  std::vector<float32> quats(k_ChunkTuples * 4);
  std::vector<int32> phases(k_ChunkTuples);
  std::vector<float32> output(k_ChunkTuples);
  std::vector<uint8> byteMaskBuffer(byteMaskPtr != nullptr ? k_ChunkTuples : 0);
  // Project-rule exception: std::vector<bool> is bit-packed and cannot provide the contiguous bool* that bulk copyIntoBuffer needs.
  std::unique_ptr<bool[]> boolMaskBufferPtr = boolMaskPtr != nullptr ? std::make_unique<bool[]>(k_ChunkTuples) : nullptr;

  usize nonHexagonalCount = 0;
  ThrottledMessageHandler progress(m_MessageHandler);
  for(usize offset = 0; offset < total; offset += k_ChunkTuples)
  {
    if(m_ShouldCancel)
    {
      return result;
    }
    const usize count = std::min(k_ChunkTuples, total - offset);
    if(!ioSucceeded(quatsStoreRef.copyIntoBuffer(offset * 4, nonstd::span<float32>(quats.data(), count * 4))) ||
       !ioSucceeded(phasesStoreRef.copyIntoBuffer(offset, nonstd::span<int32>(phases.data(), count))))
    {
      return result;
    }
    if(boolMaskPtr != nullptr && !ioSucceeded(boolMaskPtr->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<bool>(boolMaskBufferPtr.get(), count))))
    {
      return result;
    }
    if(byteMaskPtr != nullptr && !ioSucceeded(byteMaskPtr->getDataStoreRef().copyIntoBuffer(offset, nonstd::span<uint8>(byteMaskBuffer.data(), count))))
    {
      return result;
    }

    for(usize tupleIdx = 0; tupleIdx < count; ++tupleIdx)
    {
      output[tupleIdx] = std::numeric_limits<float32>::quiet_NaN();
      if((boolMaskPtr != nullptr && !boolMaskBufferPtr[tupleIdx]) || (byteMaskPtr != nullptr && byteMaskBuffer[tupleIdx] == 0))
      {
        continue;
      }
      const int32 phase = phases[tupleIdx];
      if(phase < 0 || static_cast<usize>(phase) >= crystalStructures.size())
      {
        return MergeResults(std::move(result),
                            MakeErrorResult(k_PhaseOutOfRangeError, fmt::format("Phases array '{}' has value {} at tuple index {}, but Crystal Structures array '{}' has {} tuples. "
                                                                                "Valid phase indices are in [0, {}). Correct this phase value or select the matching Crystal Structures array.",
                                                                                m_InputValues->PhasesArrayPath.toString(), phase, offset + tupleIdx,
                                                                                m_InputValues->CrystalStructuresArrayPath.toString(), crystalStructures.size(), crystalStructures.size())));
      }
      const uint32 crystalStructure = crystalStructures[phase];
      if(crystalStructure != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructure != ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        if(crystalStructure != ebsdlib::CrystalStructure::UnknownCrystalStructure)
        {
          ++nonHexagonalCount;
        }
        continue;
      }
      const usize quatIdx = tupleIdx * 4;
      ebsdlib::QuaternionDType quat(quats[quatIdx], quats[quatIdx + 1], quats[quatIdx + 2], quats[quatIdx + 3]);
      const float64 quatLength = quat.length();
      if(!std::isfinite(quatLength) || quatLength == 0.0)
      {
        continue; // Output stays NaN: this quaternion is not a rotation.
      }
      quat = quat.normalize().getPositiveOrientation();
      const ebsdlib::OrientationMatrixDType oMatrix = quat.toOrientationMatrix();
      // The transpose maps crystal [001] into the sample frame. Opposite c-axes represent the same axis.
      Eigen::Vector3d cAxis = oMatrix.transpose() * Eigen::Vector3d(0.0, 0.0, 1.0);
      cAxis.normalize();
      const float64 cosine = std::clamp(std::abs(cAxis.dot(referenceDirection)), 0.0, 1.0);
      output[tupleIdx] = static_cast<float32>(std::acos(cosine) * (180.0 / numbers::pi));
    }
    if(!ioSucceeded(outputStoreRef.copyFromBuffer(offset, nonstd::span<const float32>(output.data(), count))))
    {
      return result;
    }
    progress.updateCount(k_ProgressLabel, offset + count, total);
  }
  progress.flush();

  if(nonHexagonalCount != 0)
  {
    result.warnings().push_back({k_NonHexagonalPhasesWarning, fmt::format("{} tuples in '{}' have non-hexagonal phases and received NaN. Only Hexagonal_High (6/mmm) and Hexagonal_Low (6/m) "
                                                                          "phases are supported. To suppress this warning, enable Use Mask Array with a mask that excludes the non-hexagonal tuples.",
                                                                          nonHexagonalCount, m_InputValues->QuatsArrayPath.toString())});
  }
  return result;
}
