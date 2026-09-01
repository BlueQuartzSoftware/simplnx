#include "ComputeFeatureReferenceCAxisMisorientations.hpp"

#include "OrientationAnalysis/utilities/OrientationUtilities.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/ImageRotationUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <limits>

#include <EbsdLib/Core/EbsdDataArray.hpp>
#include <EbsdLib/Core/Orientation.hpp>
#include <EbsdLib/Orientation/OrientationFwd.hpp>
#include <EbsdLib/Orientation/Quaternion.hpp>

#include <nonstd/span.hpp>

#include <algorithm>
#include <memory>

using namespace nx::core;
using namespace nx::core::OrientationUtilities;

ComputeFeatureReferenceCAxisMisorientations::ComputeFeatureReferenceCAxisMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                                         ComputeFeatureReferenceCAxisMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

ComputeFeatureReferenceCAxisMisorientations::~ComputeFeatureReferenceCAxisMisorientations() noexcept = default;

Result<> ComputeFeatureReferenceCAxisMisorientations::operator()()
{
  // The local ensemble cache avoids repeated cell-loop access.
  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);
  const usize numCrystalStructures = crystalStructures.getNumberOfTuples();
  std::vector<uint32> crystalStructuresLocal(numCrystalStructures);
  Result<> readResult = crystalStructures.getDataStoreRef().copyIntoBuffer(0, nonstd::span<uint32>(crystalStructuresLocal.data(), numCrystalStructures));
  if(readResult.invalid())
  {
    return readResult;
  }

  bool anyPhaseIsHex = false;
  bool allPhasesAreHex = true;
  for(usize i = 1; i < numCrystalStructures; ++i)
  {
    const auto crystalStructureType = crystalStructuresLocal[i];
    const bool isHex = crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_High || crystalStructureType == ebsdlib::CrystalStructure::Hexagonal_Low;
    anyPhaseIsHex = anyPhaseIsHex || isHex;
    allPhasesAreHex = allPhasesAreHex && isHex;
  }

  if(!anyPhaseIsHex)
  {
    return MakeErrorResult(
        -9802, "Finding the feature reference c-axis misorientation requires at least one phase to be Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures but none were found.");
  }

  Result<> result;
  if(!allPhasesAreHex)
  {
    result.warnings().push_back(
        {-9803,
         "Finding the feature reference c-axis misorientation requires Hexagonal-Low 6/m or Hexagonal-High 6/mmm type crystal structures. Calculations for non Hexagonal phases will be skipped."});
  }

  const auto& featureIdsStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath).getDataStoreRef();
  const auto& quatsStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getDataStoreRef();
  const auto& cellPhasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();

  // Feature references stay local because cells access them by feature ID.
  const auto& avgCAxes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->AvgCAxesArrayPath);
  const usize totalFeatures = avgCAxes.getNumberOfTuples();
  const usize avgCAxesSize = totalFeatures * 3;
  std::vector<float32> avgCAxesLocal(avgCAxesSize);
  readResult = avgCAxes.getDataStoreRef().copyIntoBuffer(0, nonstd::span<float32>(avgCAxesLocal.data(), avgCAxesSize));
  if(readResult.invalid())
  {
    return readResult;
  }

  auto& cellRefCAxisMisStore = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceCAxisMisorientationsArrayPath).getDataStoreRef();

  auto& featAvgCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgCAxisMisorientationsArrayPath);
  featAvgCAxisMis.fill(0.0f);
  auto& featStdevCAxisMis = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureStdevCAxisMisorientationsArrayPath);
  featStdevCAxisMis.fill(0.0f);

  const usize numQuatComps = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath).getNumberOfComponents();

  std::vector<usize> counts(totalFeatures, 0);
  std::vector<float32> avgMisorientations(totalFeatures, 0.0f);

  SizeVec3 uDims = m_DataStructure.getDataRefAs<ImageGeom>(m_InputValues->ImageGeometryPath).getDimensions();

  const auto xPoints = static_cast<int64>(uDims[0]);
  const auto yPoints = static_cast<int64>(uDims[1]);
  const auto zPoints = static_cast<int64>(uDims[2]);
  const usize sliceSize = static_cast<usize>(xPoints * yPoints);
  const usize quatSliceSize = sliceSize * numQuatComps;

  const Eigen::Vector3d cAxis{0.0, 0.0, 1.0};

  std::vector<int32> featureIdSlice(sliceSize);
  std::vector<int32> cellPhaseSlice(sliceSize);
  std::vector<float32> quatSlice(quatSliceSize);
  std::vector<float32> outputSlice(sliceSize, 0.0f);

  for(int64 plane = 0; plane < zPoints; plane++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize sliceOffset = static_cast<usize>(plane) * sliceSize;

    readResult = featureIdsStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(featureIdSlice.data(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = cellPhasesStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(cellPhaseSlice.data(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = quatsStore.copyIntoBuffer(sliceOffset * numQuatComps, nonstd::span<float32>(quatSlice.data(), quatSliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }

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

        if(isHex && cellFeatureId > 0 && cellPhase > 0)
        {
          ebsdlib::OrientationMatrixDType oMatrix =
              ebsdlib::QuaternionDType(quatSlice[quatLocalIdx], quatSlice[quatLocalIdx + 1], quatSlice[quatLocalIdx + 2], quatSlice[quatLocalIdx + 3]).toOrientationMatrix();
          // The transposed matrix maps crystal [001] into the sample frame.
          Eigen::Vector3d c1 = oMatrix.transpose() * cAxis;

          c1.normalize();

          const usize avgCAxesIdx = static_cast<usize>(cellFeatureId) * 3;
          Eigen::Vector3d avgCAxisMis = {avgCAxesLocal[avgCAxesIdx], avgCAxesLocal[avgCAxesIdx + 1], avgCAxesLocal[avgCAxesIdx + 2]};
          avgCAxisMis.normalize();

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

    Result<> writeResult = cellRefCAxisMisStore.copyFromBuffer(sliceOffset, nonstd::span<const float32>(outputSlice.data(), sliceSize));
    if(writeResult.invalid())
    {
      return writeResult;
    }
  }

  // Explicit NaN keeps no-contribution features independent of FP settings.
  MessageHelper messageHelper(m_MessageHandler);
  ThrottledMessenger throttledMessenger = messageHelper.createThrottledMessenger();
  std::vector<float32> featureAverages(totalFeatures, 0.0f);
  for(usize featureId = 1; featureId < totalFeatures; featureId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    throttledMessenger.sendThrottledMessage([&] { return fmt::format("Computing per-feature average {:.2f}% completed", CalculatePercentComplete(featureId, totalFeatures)); });

    if(counts[featureId] == 0)
    {
      featureAverages[featureId] = std::numeric_limits<float32>::quiet_NaN();
    }
    else
    {
      featureAverages[featureId] = avgMisorientations[featureId] / static_cast<float32>(counts[featureId]);
    }
  }
  Result<> writeResult = featAvgCAxisMis.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(featureAverages.data(), totalFeatures));
  if(writeResult.invalid())
  {
    return writeResult;
  }

  // A second slice pass calculates population standard deviations.
  std::vector<double> stdevs(totalFeatures, 0.0);
  for(int64 plane = 0; plane < zPoints; plane++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }
    const usize sliceOffset = static_cast<usize>(plane) * sliceSize;
    readResult = featureIdsStore.copyIntoBuffer(sliceOffset, nonstd::span<int32>(featureIdSlice.data(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }
    readResult = cellRefCAxisMisStore.copyIntoBuffer(sliceOffset, nonstd::span<float32>(outputSlice.data(), sliceSize));
    if(readResult.invalid())
    {
      return readResult;
    }

    for(usize localIdx = 0; localIdx < sliceSize; localIdx++)
    {
      const int32 featureId = featureIdSlice[localIdx];
      double diff = outputSlice[localIdx] - featureAverages[featureId];
      stdevs[featureId] += (diff * diff);
    }
  }

  std::vector<float32> featureStdevs(totalFeatures, 0.0f);
  for(usize featureId = 1; featureId < totalFeatures; featureId++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(counts[featureId] == 0)
    {
      featureStdevs[featureId] = std::numeric_limits<float32>::quiet_NaN();
    }
    else
    {
      featureStdevs[featureId] = static_cast<float32>(std::sqrt(stdevs[featureId] / static_cast<double>(counts[featureId])));
    }
  }

  writeResult = featStdevCAxisMis.getDataStoreRef().copyFromBuffer(0, nonstd::span<const float32>(featureStdevs.data(), totalFeatures));
  if(writeResult.invalid())
  {
    return writeResult;
  }

  return result;
}
