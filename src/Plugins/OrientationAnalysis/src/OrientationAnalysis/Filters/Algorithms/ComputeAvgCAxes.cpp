#include "ComputeAvgCAxes.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"
#include "simplnx/Utilities/Math/MatrixMath.hpp"

#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/OrientationMatrix.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
ComputeAvgCAxes::ComputeAvgCAxes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeAvgCAxesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeAvgCAxes::~ComputeAvgCAxes() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeAvgCAxes::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeAvgCAxes::operator()()
{

  // Figure out if all phases are either Hexagonal-Low 6/m or Hexagonal-High 6/mmm Laue Phases
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < crystalStructures.size(); ++i)
  {
    const auto crystalStructureType = crystalStructures[i];
    const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
    allPhasesHexagonal = allPhasesHexagonal && isHex;
    noPhasesHexagonal = noPhasesHexagonal && !isHex;
  }

  // If NONE of the phases are hexagonal then bail out now with an error
  if(noPhasesHexagonal)
  {
    return MakeErrorResult(-76402, "No phases that have a crystal symmetry of Hexagonal (6/mmm or 6/m) were found.");
  }

  Result<> result;

  // Throw a warning for any NON-Hex Laue Phases
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back({-76403, "Non Hexagonal phases were found. All calculations for non Hexagonal phases will be skipped and a NaN value inserted."});
  }

  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize totalFeatures = avgCAxes.getNumberOfTuples();

  const Eigen::Vector3d cAxis{0.0f, 0.0f, 1.0f};
  Eigen::Vector3d c1{0.0f, 0.0f, 0.0f};

  std::vector<int32> counter(totalFeatures, 0);

  // Loop over each cell
  for(usize i = 0; i < totalPoints; i++)
  {
    int32 currentFeatureId = featureIds[i];
    // If the featureId for a given cell is valid ( > 0) then analyze that value
    if(currentFeatureId > 0)
    {
      const int32 currentCellPhase = cellPhases[i];                          // Get the current cell phase
      const auto crystalStructureType = crystalStructures[currentCellPhase]; // Get the CrystalStructure, i.e., Laue class of the cell
      const usize cAxesIndex = 3 * currentFeatureId;

      // Ensure the Laue class is correct, otherwise mark the values with a NaN and continue
      if(crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_High && crystalStructureType != ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        avgCAxes[cAxesIndex] = NAN;
        avgCAxes[cAxesIndex + 1] = NAN;
        avgCAxes[cAxesIndex + 2] = NAN;
        continue;
      }

      counter[currentFeatureId]++; // Increment the count
      const usize quatIndex = i * 4;

      // Create the 3x3 Orientation Matrix from the Quaternion. This represents a passive rotation matrix
      ebsdlib::OrientationMatrixDType oMatrix = ebsdlib::QuaternionDType(quats[quatIndex], quats[quatIndex + 1], quats[quatIndex + 2], quats[quatIndex + 3]).toOrientationMatrix();

      // Convert the passive rotation matrix to an active rotation matrix by taking the transpose
      // Multiply the active transformation matrix by the C-Axis (as Miller Index). This actively rotates
      // the crystallographic C-Axis (which is along the <0,0,1> direction) into the physical sample
      // reference frame
      c1 = oMatrix.transpose() * cAxis;

      // normalize so that the magnitude is 1
      c1.normalize();

      // Compute the running average c-axis and normalize the result
      Eigen::Vector3d curCAxis{0.0f, 0.0f, 0.0f};
      curCAxis[0] = avgCAxes[cAxesIndex] / static_cast<float32>(counter[currentFeatureId]);
      curCAxis[1] = avgCAxes[cAxesIndex + 1] / static_cast<float32>(counter[currentFeatureId]);
      curCAxis[2] = avgCAxes[cAxesIndex + 2] / static_cast<float32>(counter[currentFeatureId]);
      curCAxis.normalize();

      // Ensure that angle between the current point's sample reference frame C-Axis
      // and the running average sample C-Axis is positive
      float64 w = ImageRotationUtilities::CosBetweenVectors(c1, curCAxis);
      if(w < 0.0)
      {
        c1 *= -1.0f;
      }

      // Continue summing up the rotations
      float value = avgCAxes[cAxesIndex] + c1[0];
      avgCAxes[cAxesIndex] = value;

      value = avgCAxes[cAxesIndex + 1] + c1[1];
      avgCAxes[cAxesIndex + 1] = value;

      value = avgCAxes[cAxesIndex + 2] + c1[2];
      avgCAxes[cAxesIndex + 2] = value;
    }
  }

  for(size_t i = 1; i < totalFeatures; i++)
  {
    const usize tupleIndex = i * 3;
    float32 avgCAxesValue = avgCAxes[tupleIndex];
    if(std::isnan(avgCAxesValue))
    {
      continue;
    }
    // If we got passed the last check this could happen if the cell points were
    // masked out? Maybe?
    if(counter[i] == 0)
    {
      avgCAxes[tupleIndex] = 0;
      avgCAxes[tupleIndex + 1] = 0;
      avgCAxes[tupleIndex + 2] = 1;
    }
    else
    {
      // Compute the final average c-axis value
      float value = avgCAxes[3 * i];
      value /= static_cast<float>(counter[i]);
      avgCAxes[3 * i] = value;

      value = avgCAxes[3 * i + 1];
      value /= static_cast<float>(counter[i]);
      avgCAxes[3 * i + 1] = value;

      value = avgCAxes[3 * i + 2];
      value /= static_cast<float>(counter[i]);
      avgCAxes[3 * i + 2] = value;
    }
  }
  return result;
}
