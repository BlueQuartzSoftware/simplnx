#include "ComputeAvgCAxes.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/Math/GeometryMath.hpp"

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
  avgCAxes.fill(0.0f); // Initialize all output values to ZERO defensively.

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize totalFeatures = avgCAxes.getNumberOfTuples();

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};

  std::vector<int32> cellCount(totalFeatures, 0);

  m_MessageHandler.sendInfoMessage("Computing cell contributions");

  // Loop over each cell
  for(usize i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    int32 currentFeatureId = featureIds[i];
    // If the featureId for a given cell is valid ( > 0) then analyze that value
    if(currentFeatureId > 0)
    {
      const int32 currentCellPhase = cellPhases[i];                             // Get the current cell phase
      const auto currentCrystalStructure = crystalStructures[currentCellPhase]; // Get the CrystalStructure, i.e., Laue class of the cell
      const usize cAxesIndex = 3 * currentFeatureId;

      // If the Laue class is not Hexagonal, then continue to the next cell
      if(currentCrystalStructure != ebsdlib::CrystalStructure::Hexagonal_High && currentCrystalStructure != ebsdlib::CrystalStructure::Hexagonal_Low)
      {
        continue;
      }

      cellCount[currentFeatureId]++; // Increment the counter if we are the appropriate Laue class.
      const usize quatIndex = i * 4;

      // Create the 3x3 Orientation Matrix from the Quaternion. This represents a passive rotation matrix
      ebsdlib::OrientationMatrixDType oMatrix = ebsdlib::QuaternionDType(quats[quatIndex], quats[quatIndex + 1], quats[quatIndex + 2], quats[quatIndex + 3]).toOrientationMatrix();

      // Convert the passive rotation matrix to an active rotation matrix by taking the transpose
      // Multiply the active transformation matrix by the C-Axis (as Miller Index). This actively rotates
      // the crystallographic C-Axis (which is along the <0,0,1> direction) into the physical sample
      // reference frame
      Eigen::Vector3d cellCAxis = oMatrix.transpose() * cAxis;

      // normalize so that the magnitude is 1
      cellCAxis.normalize();

      // Compute the running average c-axis and normalize the result
      Eigen::Vector3d runningCAxisAvg{avgCAxes[cAxesIndex] / static_cast<float32>(cellCount[currentFeatureId]), avgCAxes[cAxesIndex + 1] / static_cast<float32>(cellCount[currentFeatureId]),
                                      avgCAxes[cAxesIndex + 2] / static_cast<float32>(cellCount[currentFeatureId])};
      runningCAxisAvg.normalize();

      // Ensure that angle between the current point's sample reference frame C-Axis
      // and the running average sample C-Axis is positive
      float64 cosAngle = ImageRotationUtilities::CosBetweenVectors(cellCAxis, runningCAxisAvg);
      if(cosAngle < 0.0)
      {
        cellCAxis *= -1.0f;
      }

      // Accumulate per-component into the float32 output (Eigen math is double; narrow on store).
      avgCAxes[cAxesIndex] = static_cast<float32>(avgCAxes[cAxesIndex] + cellCAxis[0]);
      avgCAxes[cAxesIndex + 1] = static_cast<float32>(avgCAxes[cAxesIndex + 1] + cellCAxis[1]);
      avgCAxes[cAxesIndex + 2] = static_cast<float32>(avgCAxes[cAxesIndex + 2] + cellCAxis[2]);
    }
  }

  // Now that each feature's Axis is summed up, compute the final average C-Axis
  m_MessageHandler.sendInfoMessage("Computing final feature average C-Axis values");

  for(usize currentFeatureId = 0; currentFeatureId < totalFeatures; currentFeatureId++)
  {
    if(m_ShouldCancel)
    {
      return result;
    }

    const usize cAxesIndex = 3 * currentFeatureId;
    if(cellCount[currentFeatureId] == 0)
    {
      // Feature is either non-hexagonal or has no assigned voxels; either way, no meaningful average exists.
      avgCAxes[cAxesIndex] = NAN;
      avgCAxes[cAxesIndex + 1] = NAN;
      avgCAxes[cAxesIndex + 2] = NAN;
    }
    else
    {
      // Divide the accumulated sum by the cell count, then normalize so the
      // output is a unit-magnitude C-axis direction. The antipodal-flip rule
      // guarantees |sum| >= sqrt(cellCount), so the divided vector's magnitude
      // is >= 1/sqrt(cellCount) > 0 -- no near-zero guard needed.
      Eigen::Vector3d finalAvg{avgCAxes[cAxesIndex] / static_cast<float64>(cellCount[currentFeatureId]), avgCAxes[cAxesIndex + 1] / static_cast<float64>(cellCount[currentFeatureId]),
                               avgCAxes[cAxesIndex + 2] / static_cast<float64>(cellCount[currentFeatureId])};
      finalAvg.normalize();
      avgCAxes[cAxesIndex] = static_cast<float32>(finalAvg[0]);
      avgCAxes[cAxesIndex + 1] = static_cast<float32>(finalAvg[1]);
      avgCAxes[cAxesIndex + 2] = static_cast<float32>(finalAvg[2]);
    }
  }
  return result;
}
