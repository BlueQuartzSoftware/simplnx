#include "ComputeFeatureReferenceCAxisMisorientations.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"

#include <EbsdLib/Core/EbsdDataArray.hpp>
#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

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
   * Cache ensemble-level crystalStructures locally (tiny array, avoids
   * per-element OOC overhead during the main cell loop)
   */
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const usize numCrystalStructures = crystalStructures.getNumberOfTuples();
  std::vector<uint32> crystalStructuresLocal(numCrystalStructures);
  crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresLocal.data(), numCrystalStructures));

  bool allPhasesHexagonal = true;
  bool noPhasesHexagonal = true;
  for(usize i = 1; i < numCrystalStructures; ++i)
  {
    const auto crystalStructureType = crystalStructuresLocal[i];
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
   * Get DataStore references for bulk I/O
   */
  // Input Cell Data (DataStore refs for copyIntoBuffer)
  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& quatsStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const auto& cellPhasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();

  // Input Feature Data — cache avgCAxes locally (feature-level, small)
  const auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);
  const usize totalFeatures = avgCAxes.getNumberOfTuples();
  const usize avgCAxesSize = totalFeatures * 3;
  std::vector<float32> avgCAxesLocal(avgCAxesSize);
  avgCAxes.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(avgCAxesLocal.data(), avgCAxesSize));

  // Output Cell Data (DataStore ref for copyFromBuffer)
  auto& cellRefCAxisMisStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceCAxisMisorientationsArrayPath).getDataStoreRef();

  // Output Feature Data
  auto& featAvgCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgCAxisMisorientationsArrayPath);
  featAvgCAxisMis.fill(0.0f);
  auto& featStdevCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureStdevCAxisMisorientationsArrayPath);
  featStdevCAxisMis.fill(0.0f);

  const usize numQuatComps = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getNumberOfComponents();

  std::vector<usize> counts(totalFeatures, 0ULL);
  std::vector<float32> avgMisorientations(totalFeatures, 0.0f);

  SizeVec3 uDims = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath).getDimensions();

  const auto xPoints = static_cast<int64>(uDims[0]);
  const auto yPoints = static_cast<int64>(uDims[1]);
  const auto zPoints = static_cast<int64>(uDims[2]);
  const usize sliceSize = static_cast<usize>(xPoints * yPoints);
  const usize quatSliceSize = sliceSize * numQuatComps;

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};

  // Z-slice buffers for cell-level arrays (avoids per-element OOC access)
  std::vector<int32> featureIdSlice(sliceSize);
  std::vector<int32> cellPhaseSlice(sliceSize);
  std::vector<float32> quatSlice(quatSliceSize);
  std::vector<float32> outputSlice(sliceSize, 0.0f);

  /* **************************************************************************
   * Loop over all cells in the ImageGeometry, one Z-slice at a time.
   * Each slice is bulk-read from the DataStore, processed, and the output
   * is bulk-written back.
   */
  for(int64 plane = 0; plane < zPoints; plane++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize sliceOffset = static_cast<usize>(plane) * sliceSize;

    // Bulk-read this Z-slice of input cell data
    featureIdsStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(featureIdSlice.data(), sliceSize));
    cellPhasesStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(cellPhaseSlice.data(), sliceSize));
    quatsStore.copyIntoBuffer(sliceOffset * numQuatComps, nonstd::span<float32>(quatSlice.data(), quatSliceSize));

    for(int64 row = 0; row < yPoints; row++)
    {
      for(int64 col = 0; col < xPoints; col++)
      {
        const usize localIdx = static_cast<usize>(row * xPoints + col);
        const usize quatLocalIdx = localIdx * numQuatComps;
        const int32 cellFeatureId = featureIdSlice[localIdx];
        const int32 cellPhase = cellPhaseSlice[localIdx];
        const uint32 crystalStructureType = crystalStructuresLocal[cellPhase];
        const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;

        // Make sure the cell is Hexagonal Laue class, the featureId and phases are valid
        // INVALID featureIds have a value of ZERO
        // INVALID phases have a value of ZERO
        if(isHex && cellFeatureId > 0 && cellPhase > 0)
        {
          // Create the OrientationMatrix from the Quaternion
          ebsdlib::OrientationMatrixDType oMatrix =
              ebsdlib::QuaternionDType(quatSlice[quatLocalIdx], quatSlice[quatLocalIdx + 1], quatSlice[quatLocalIdx + 2], quatSlice[quatLocalIdx + 3]).toOrientationMatrix();
          // Transpose the OM and multiply by cAxis to rotate cAxis
          Eigen::Vector3d c1 = oMatrix.transpose() * cAxis;

          // normalize so that the magnitude is 1
          c1.normalize();

          // normalize the features average C-Axis
          const usize avgCAxesIdx = static_cast<usize>(cellFeatureId) * 3;
          Eigen::Vector3d avgCAxisMis = {avgCAxesLocal[avgCAxesIdx], avgCAxesLocal[avgCAxesIdx + 1], avgCAxesLocal[avgCAxesIdx + 2]};
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

          outputSlice[localIdx] = static_cast<float32>(w);
          counts[cellFeatureId]++;
          avgMisorientations[cellFeatureId] += static_cast<float32>(w);
        }
        else
        {
          outputSlice[localIdx] = 0.0f;
        }
      }
    }

    // Bulk-write this Z-slice of output cell data
    cellRefCAxisMisStore.copyFromBuffer(sliceOffset, nonstd::span<const float32>(outputSlice.data(), sliceSize));
  }

  // Loop over all the features from the feature attribute matrix and compute the
  // average C Axis Misorientation for each feature
  for(usize featureId = 1; featureId < totalFeatures; featureId++)
  {
    if(featureId % 1000 == 0)
    {
      m_MessageHandler(IFilter::Message::Type::Info, fmt::format("Working On Feature {} of {}", featureId, totalFeatures));
    }
    if(m_ShouldCancel)
    {
      return {};
    }

    // Compute the average value of the misorientations between each feature's cell
    // and the average C-Axis for that feature
    featAvgCAxisMis[featureId] = avgMisorientations[featureId] / static_cast<float32>(counts[featureId]);
  }

  // These 2 loops compute the population standard deviation of those misorientations for
  // each feature. Re-read cell data one Z-slice at a time.
  std::vector<double> stdevs(totalFeatures, 0.0);
  for(int64 plane = 0; plane < zPoints; plane++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize sliceOffset = static_cast<usize>(plane) * sliceSize;
    featureIdsStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(featureIdSlice.data(), sliceSize));
    cellRefCAxisMisStore.copyIntoBuffer(sliceOffset, nonstd::span<float32>(outputSlice.data(), sliceSize));

    for(usize localIdx = 0; localIdx < sliceSize; localIdx++)
    {
      const int32 featureId = featureIdSlice[localIdx];
      double diff = outputSlice[localIdx] - featAvgCAxisMis.getValue(featureId);
      stdevs[featureId] += (diff * diff);
    }
  }

  // Finish computing the standard deviation in this loop
  for(usize featureId = 1; featureId < totalFeatures; featureId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    featStdevCAxisMis[featureId] = std::sqrt(stdevs[featureId] / static_cast<double>(counts[featureId]));
  }

  return {};
}
