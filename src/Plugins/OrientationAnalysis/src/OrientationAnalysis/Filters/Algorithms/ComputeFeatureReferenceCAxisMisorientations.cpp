#include "ComputeFeatureReferenceCAxisMisorientations.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <EbsdLib/Core/EbsdDataArray.hpp>
#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <algorithm>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

// -----------------------------------------------------------------------------
ComputeFeatureReferenceCAxisMisorientations::ComputeFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                         ComputeFeatureReferenceCAxisMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureReferenceCAxisMisorientations::~ComputeFeatureReferenceCAxisMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeatureReferenceCAxisMisorientations::operator()()
{

  /* **************************************************************************
   * This section performs a sanity check to ensure that at least 1 phase is
   * hexagonal.
   */
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

  if(noPhasesHexagonal)
  {
    return MakeErrorResult(
        -9802, "Finding the feature reference c-axis misorientation requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesHexagonal)
  {
    result.warnings().push_back(
        {-9803,
         "Finding the feature reference c-axis misorientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Calculations for non Hexagonal phases will be skipped."});
  }

  /* **************************************************************************
   * Get References to the Input and output Data Arrays
   */
  // Input Cell Data
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  // Input Feature Data
  const auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);

  // Output Cell Data
  auto& cellRefCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceCAxisMisorientationsArrayPath);
  // Output Feature Data
  auto& featAvgCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgCAxisMisorientationsArrayPath);
  featAvgCAxisMis.fill(0.0f);
  auto& featStdevCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureStdevCAxisMisorientationsArrayPath);
  featStdevCAxisMis.fill(0.0f);

  const usize totalPoints = featureIds.getNumberOfTuples();
  const usize totalFeatures = avgCAxes.getNumberOfTuples();

  const usize numQuatComps = quats.getNumberOfComponents();

  std::vector<usize> counts(totalFeatures, 0ULL);
  std::vector<float32> avgMisorientations(totalFeatures, 0.0f);

  SizeVec3 uDims = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath).getDimensions();

  const auto xPoints = static_cast<int64>(uDims[0]);
  const auto yPoints = static_cast<int64>(uDims[1]);
  const auto zPoints = static_cast<int64>(uDims[2]);

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(zPoints);
  progressHelper.setProgressMessageTemplate("Compute Feature Reference C-Axis Misorientations: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  /* **************************************************************************
   * Loop over all cells in the ImageGeometry
   */
  for(int64 plane = 0; plane < zPoints; plane++)
  {
    for(int64 row = 0; row < yPoints; row++)
    {
      for(int64 col = 0; col < xPoints; col++)
      {
        int64 cellIdx = (plane * xPoints * yPoints) + (row * xPoints) + col;
        const usize quatTupleIndex = cellIdx * numQuatComps;
        const uint32 crystalStructureType = crystalStructures[cellPhases[cellIdx]];
        const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
        int32_t cellFeatureId = featureIds[cellIdx];
        int32_t cellPhase = cellPhases[cellIdx];

        // Make sure the cell is Hexagonal Laue class, the featureId and phases are valid
        // INVALID featureIds have a value of ZERO
        // INVALID phases have a value of ZERO
        if(isHex && cellFeatureId > 0 && cellPhase > 0)
        {
          // Create the OrientationMatrix from the Quaternion
          ebsdlib::OrientationMatrixDType oMatrix =
              ebsdlib::QuaternionDType(quats[quatTupleIndex], quats[quatTupleIndex + 1], quats[quatTupleIndex + 2], quats[quatTupleIndex + 3]).toOrientationMatrix();
          // Transpose the OM and multiply by cAxis to rotate cAxis
          Eigen::Vector3d c1 = oMatrix.transpose() * cAxis;

          // normalize so that the magnitude is 1
          c1.normalize();

          // normalize the features average C-Axis
          Eigen::Vector3d avgCAxisMis = {avgCAxes[3 * cellFeatureId], avgCAxes[3 * cellFeatureId + 1], avgCAxes[3 * cellFeatureId + 2]};
          avgCAxisMis.normalize();

          // Calculate the angle between the current C-Axis and the Feature's Average C-Axis
          float64 w = ImageRotationUtilities::CosBetweenVectors(c1, avgCAxisMis);
          w = std::clamp(w, -1.0, 1.0);
          w = std::acos(w);
          w *= Constants::k_180OverPiD;
          if(w > 90.0)
          {
            w = 180.0 - w;
          }

          cellRefCAxisMis.setValue(cellIdx, static_cast<float32>(w));
          counts[cellFeatureId]++;
          avgMisorientations[cellFeatureId] += static_cast<float32>(w);
        }
        else
        {
          cellRefCAxisMis.setValue(cellIdx, 0.0f);
        }
      }
    }
    progressMessenger.sendProgressMessage(1);
  }

  // Loop over all the features from the feature attribute matrix and compute the
  // average C Axis Misorientation for each feature
  for(usize featureId = 1; featureId < totalFeatures; featureId++)
  {
    // Compute the average value of the misorientations between each feature's cell
    // and the average C-Axis for that feature
    featAvgCAxisMis[featureId] = avgMisorientations[featureId] / static_cast<float32>(counts[featureId]);
  }

  // These 2 loops compute the population standard deviation of those misorientations for
  // each feature.
  std::vector<double> stdevs(totalFeatures, 0.0);
  for(usize cellIdx = 0; cellIdx < totalPoints; cellIdx++)
  {
    const int32 featureId = featureIds[cellIdx];
    double diff = cellRefCAxisMis.getValue(cellIdx) - featAvgCAxisMis.getValue(featureId);
    stdevs[featureId] += (diff * diff);
  }

  // Finish computing the standard deviation in this loop
  for(usize featureId = 1; featureId < totalFeatures; featureId++)
  {
    featStdevCAxisMis[featureId] = std::sqrt(stdevs[featureId] / static_cast<double>(counts[featureId]));
  }

  return {};
}
