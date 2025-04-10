#include "ComputeMisorientations.hpp"

#include "simplnx/Utilities/DataArrayUtilities.hpp"

#include "EbsdLib/LaueOps/LaueOps.h"
#include "simplnx/Common/Constants.hpp"

#include <EbsdLib/Core/OrientationRepresentation.h>
#include <EbsdLib/Core/OrientationTransformation.hpp>

using namespace nx::core;

namespace
{
inline void ComputeMisorientation(const QuatD& q1, const QuatD& q2, Float32Array& outputMisorientations, size_t laueClass, const std::vector<LaueOps::Pointer>& m_OrientationOps, size_t tupleIdx)
{
  OrientationD axisAngle = m_OrientationOps[laueClass]->calculateMisorientation(q1, q2);

  outputMisorientations[tupleIdx * 4 + 0] = axisAngle[0];
  outputMisorientations[tupleIdx * 4 + 1] = axisAngle[1];
  outputMisorientations[tupleIdx * 4 + 2] = axisAngle[2];
  outputMisorientations[tupleIdx * 4 + 3] = axisAngle[3] * Constants::k_180OverPiD; // Convert the output Angle to Degrees.
}

Result<> ComputeUsingArrays(DataStructure& m_DataStructure, const ComputeMisorientationsInputValues* inputValues)
{
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(inputValues->InputPhasesArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(inputValues->InputCrystalStructuresArrayPath);

  const auto& inputOrientations1 = m_DataStructure.getDataRefAs<Float32Array>(inputValues->InputOrientationPath1).getDataStoreRef();
  const auto& inputOrientations2 = m_DataStructure.getDataRefAs<Float32Array>(inputValues->InputOrientationPath2).getDataStoreRef();

  auto& outputMisorientations = m_DataStructure.getDataRefAs<Float32Array>(inputValues->OutputMisorientationsPath);

  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  size_t totalPoints = inputOrientations1.getNumberOfTuples();

  for(int64_t tupleIdx = 0; tupleIdx < totalPoints; tupleIdx++)
  {
    if(cellPhases[tupleIdx] > 0) // We must have a valid phase index.
    {
      size_t laueClass = static_cast<size_t>(crystalStructures[cellPhases[tupleIdx]]);

      // Convert to a Quaternion
      OrientationType orientation1(inputOrientations1[tupleIdx * 3], inputOrientations1[tupleIdx * 3 + 1], inputOrientations1[tupleIdx * 3 + 2]);
      const QuatD q1 = OrientationTransformation::eu2qu<OrientationType, QuatD>(orientation1);
      OrientationType orientation2(inputOrientations2[tupleIdx * 3], inputOrientations2[tupleIdx * 3 + 1], inputOrientations2[tupleIdx * 3 + 2]);
      const QuatD q2 = OrientationTransformation::eu2qu<OrientationType, QuatD>(orientation2);

      ComputeMisorientation(q1, q2, outputMisorientations, laueClass, m_OrientationOps, tupleIdx);
    }
    else
    {
      outputMisorientations[tupleIdx * 4 + 0] = 0.0f;
      outputMisorientations[tupleIdx * 4 + 1] = 0.0f;
      outputMisorientations[tupleIdx * 4 + 2] = 0.0f;
      outputMisorientations[tupleIdx * 4 + 3] = 0.0f;
    }
  }

  return {};
}

Result<> ComputeUsingReferenceOrientation(DataStructure& m_DataStructure, const ComputeMisorientationsInputValues* inputValues)
{
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(inputValues->InputPhasesArrayPath);
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(inputValues->InputCrystalStructuresArrayPath);

  const auto& inputOrientations1 = m_DataStructure.getDataRefAs<Float32Array>(inputValues->InputOrientationPath1).getDataStoreRef();

  auto& outputMisorientations = m_DataStructure.getDataRefAs<Float32Array>(inputValues->OutputMisorientationsPath);

  std::vector<LaueOps::Pointer> m_OrientationOps = LaueOps::GetAllOrientationOps();

  size_t totalPoints = inputOrientations1.getNumberOfTuples();

  using OrientationType = Orientation<float64>;
  Eigen::Vector3d axis(inputValues->ReferenceOrientation[0], inputValues->ReferenceOrientation[1], inputValues->ReferenceOrientation[2]);
  axis.normalize();
  const OrientationType referenceOrientation(axis[0], axis[1], axis[2], inputValues->ReferenceOrientation[3] * Constants::k_PiOver180D);

  const QuatD q2 = OrientationTransformation::ax2qu<OrientationType, QuatD>(referenceOrientation);

  for(int64_t tupleIdx = 0; tupleIdx < totalPoints; tupleIdx++)
  {
    if(cellPhases[tupleIdx] > 0) // We must have a valid phase index.
    {
      size_t phase1 = static_cast<size_t>(crystalStructures[cellPhases[tupleIdx]]);

      // Convert to a Quaternion
      OrientationType orientation1(inputOrientations1[tupleIdx * 3], inputOrientations1[tupleIdx * 3 + 1], inputOrientations1[tupleIdx * 3 + 2]);
      QuatD q1 = OrientationTransformation::eu2qu<OrientationType, QuatD>(orientation1);

      ComputeMisorientation(q1, q2, outputMisorientations, phase1, m_OrientationOps, tupleIdx);
    }
    else
    {
      outputMisorientations[tupleIdx * 4 + 0] = 0.0f;
      outputMisorientations[tupleIdx * 4 + 1] = 0.0f;
      outputMisorientations[tupleIdx * 4 + 2] = 0.0f;
      outputMisorientations[tupleIdx * 4 + 3] = 0.0f;
    }
  }

  return {};
}
} // namespace

// -----------------------------------------------------------------------------
ComputeMisorientations::ComputeMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeMisorientations::~ComputeMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeMisorientations::operator()()
{
  Result<> result;

  if(m_InputValues->ComputationType == compute_misorientations_constants::k_UseArraysIndex)
  {
    result = ComputeUsingArrays(m_DataStructure, m_InputValues);
  }
  else if(m_InputValues->ComputationType == compute_misorientations_constants::k_UseReferenceAxesIndex)
  {
    result = ComputeUsingReferenceOrientation(m_DataStructure, m_InputValues);
  }

  return result;
}
