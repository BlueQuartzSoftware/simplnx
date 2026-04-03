#include "ComputeFeatureReferenceMisorientations.hpp"

#include "simplnx/Common/Constants.hpp"
#include "simplnx/Common/Numbers.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataGroup.hpp"
#include "simplnx/DataStructure/Geometry/ImageGeom.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <EbsdLib/LaueOps/LaueOps.h>

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeFeatureReferenceMisorientations::ComputeFeatureReferenceMisorientations(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                                               ComputeFeatureReferenceMisorientationsInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeatureReferenceMisorientations::~ComputeFeatureReferenceMisorientations() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeFeatureReferenceMisorientations::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeFeatureReferenceMisorientations::operator()()
{
  DataPath imageGeomPath = m_InputValues->CellPhasesArrayPath.getParent().getParent();
  const ImageGeom& imageGeom = m_DataStructure.getDataRefAs<ImageGeom>(imageGeomPath);

  // Input Arrays
  const auto& cellPhases = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath);
  const auto& featureIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsArrayPath);
  const auto& quats = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->QuatsArrayPath);

  // Get the average quats data array. It will be null unless m_InputValues->ReferenceOrientation = 0
  const auto* avgQuatsPtr = m_DataStructure.getDataAs<Float32Array>(m_InputValues->AvgQuatsArrayPath);

  // Get the Feature AttributeMatrix. It will be null unless m_InputValues->ReferenceOrientation = 1
  const auto* featureAttrMatPtr = m_DataStructure.getDataAs<AttributeMatrix>(m_InputValues->FeatureAttributeMatrixPath);

  const auto& crystalStructures = m_DataStructure.getDataRefAs<UInt32Array>(m_InputValues->CrystalStructuresArrayPath);

  // Output Arrays
  auto& featureReferenceMisorientations = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureReferenceMisorientationsArrayName);
  auto& avgReferenceMisorientation = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->FeatureAvgMisorientationsArrayName);

  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->FeatureAvgMisorientationsArrayName, featureIds, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  std::vector<ebsdlib::LaueOps::Pointer> m_OrientationOps = ebsdlib::LaueOps::GetAllOrientationOps();

  const size_t totalVoxels = featureIds.getNumberOfTuples();

  // Get the total features from the appropriate source..
  size_t totalFeatures = 0;
  if(featureAttrMatPtr != nullptr)
  {
    totalFeatures = featureAttrMatPtr->getNumberOfTuples();
  }
  if(avgQuatsPtr != nullptr)
  {
    totalFeatures = avgQuatsPtr->getNumberOfTuples();
  }
  if(totalFeatures == 0)
  {
    return MakeErrorResult(-34900, "Total features was zero. The filter cannot proceed. Check either the feature attribute matrix or the average quaternions for proper size");
  }

  // Create local storage for teh centers and center distances
  std::vector<size_t> m_Centers(totalFeatures, 0);
  std::vector<float> m_CenterDistances(totalFeatures, 0.0f);

  // If the user selected "Misorientation from Feature Centers"
  if(m_InputValues->ReferenceOrientation == 1)
  {
    const auto& m_GBEuclideanDistances = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->GBEuclideanDistancesArrayPath);
    for(size_t voxelIdx = 0; voxelIdx < totalVoxels; voxelIdx++)
    {
      if(m_ShouldCancel)
      {
        return {};
      }

      int32_t featureId = featureIds[voxelIdx];
      float32 distance = m_GBEuclideanDistances[voxelIdx];
      if(distance >= m_CenterDistances[featureId])
      {
        m_CenterDistances[featureId] = distance; // Save the GB Distance value
        m_Centers[featureId] = voxelIdx;         // Save the voxel index for that value
      }
    }

    const auto& euclideanCellCenters = m_DataStructure.getDataAs<Float32Array>(m_InputValues->FeatureEuclideanCentersPath)->getIDataStoreAs<AbstractDataStore<float32>>();

    for(size_t i = 1; i < totalFeatures; i++)
    {
      usize voxelIdx = m_Centers[i];
      auto cellCenter = imageGeom.getCoordsf(voxelIdx);
      euclideanCellCenters->setTuple(i, cellCenter.data());
    }
  }

  std::vector<float> avgMisorientationSums(totalFeatures, 0.0F);
  std::vector<float> avgMisorientationCounts(totalFeatures, 0.0F);

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(totalVoxels);
  progressHelper.setProgressMessageTemplate("Compute Feature Reference Misorientations: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  featureReferenceMisorientations.fill(0.0f); // Fill all values with Zeros.
  for(int64_t voxelIdx = 0; voxelIdx < totalVoxels; voxelIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(featureIds[voxelIdx] > 0 && cellPhases[voxelIdx] > 0)
    {
      // Get the orientation of the current voxel
      ebsdlib::QuatD q1(quats[voxelIdx * 4 + 0], quats[voxelIdx * 4 + 1], quats[voxelIdx * 4 + 2], quats[voxelIdx * 4 + 3]);
      ebsdlib::QuatD q2;                           // Get this ready to use. It gets filled depending on the kind of reference orientation the user selected
      if(m_InputValues->ReferenceOrientation == 0) // Use Average Quaternions
      {
        const auto featureId = static_cast<size_t>(featureIds[voxelIdx]);
        q2 = ebsdlib::QuatD(avgQuatsPtr->getValue(featureId * 4), avgQuatsPtr->getValue(featureId * 4 + 1), avgQuatsPtr->getValue(featureId * 4 + 2), avgQuatsPtr->getValue(featureId * 4 + 3));
      }
      else if(m_InputValues->ReferenceOrientation == 1) // Use the voxel's orientation that is the farthest from the grain boundary
      {
        auto featureId = static_cast<size_t>(featureIds[voxelIdx]);
        size_t centerVoxelIdx = m_Centers[featureId];
        q2 = ebsdlib::QuatD(quats[centerVoxelIdx * 4 + 0], quats[centerVoxelIdx * 4 + 1], quats[centerVoxelIdx * 4 + 2], quats[centerVoxelIdx * 4 + 3]);
      }

      uint32 laueClass1 = crystalStructures[cellPhases[voxelIdx]];
      ebsdlib::AxisAngleDType axisAngle = m_OrientationOps[laueClass1]->calculateMisorientation(q1, q2);

      // Extract the misorientation, convert it to degrees, and store if for this voxel
      featureReferenceMisorientations[voxelIdx] = static_cast<float>(Constants::k_RadToDegD * axisAngle[3]); // convert to degrees

      // Update our temp storage vectors that will eventually compute the final `average reference misorientation`
      int32_t idx = featureIds[voxelIdx];
      avgMisorientationCounts[idx]++;
      avgMisorientationSums[idx] = avgMisorientationSums[idx] + featureReferenceMisorientations[voxelIdx];
    }
    progressMessenger.sendProgressMessage(1);
  }

  // Update the avgReferenceMisorientation output array
  avgReferenceMisorientation[0] = 0.0f;
  for(size_t featureIdx = 1; featureIdx < totalFeatures; featureIdx++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    if(avgMisorientationCounts[featureIdx] == 0.0f)
    {
      avgReferenceMisorientation[featureIdx] = 0.0f;
    }
    else
    {
      avgReferenceMisorientation[featureIdx] = avgMisorientationSums[featureIdx] / avgMisorientationCounts[featureIdx];
    }
  }
  return {};
}
