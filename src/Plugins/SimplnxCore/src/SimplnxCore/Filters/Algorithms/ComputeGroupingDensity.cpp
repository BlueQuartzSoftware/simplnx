#include "ComputeGroupingDensity.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <unordered_set>

using namespace nx::core;

namespace
{
template <bool UseNonContiguousNeighbors, bool FindCheckedFeatures>
struct FindDensitySpecializations
{
  static constexpr bool UsingNonContiguousNeighbors = UseNonContiguousNeighbors;
  static constexpr bool FindingCheckedFeatures = FindCheckedFeatures;
};

template <class FindDensitySpecializations = FindDensitySpecializations<true, true>>
class FindDensityGrouping
{
public:
  FindDensityGrouping(const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler, const Int32Array& parentIds, const Float32Array& parentVolumes,
                      const Float32Array& featureVolumes, const Int32NeighborList& contiguousNL, Float32Array& groupingDensities, const Int32NeighborList& nonContiguousNL, Int32Array& checkedFeatures)
  : m_ShouldCancel(shouldCancel)
  , m_MessageHandler(mesgHandler)
  , m_ParentIds(parentIds)
  , m_ParentVolumes(parentVolumes)
  , m_FeatureVolumes(featureVolumes)
  , m_ContiguousNL(contiguousNL)
  , m_GroupingDensities(groupingDensities)
  , m_NonContiguousNL(nonContiguousNL)
  , m_CheckedFeatures(checkedFeatures)
  {
  }
  ~FindDensityGrouping() noexcept = default;

  FindDensityGrouping(const FindDensityGrouping&) = delete;            // Copy Constructor Default Implemented
  FindDensityGrouping(FindDensityGrouping&&) = delete;                 // Move Constructor Not Implemented
  FindDensityGrouping& operator=(const FindDensityGrouping&) = delete; // Copy Assignment Not Implemented
  FindDensityGrouping& operator=(FindDensityGrouping&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()()
  {
    // This is feature data, from 2 different Feature Attribute Matrix
    const auto& featureParentIdsRef = m_ParentIds.getDataStoreRef();
    const auto& parentVolumesRef = m_ParentVolumes.getDataStoreRef();
    const auto& featureVolumesRef = m_FeatureVolumes.getDataStoreRef();

    // These are output **Feature** level data arrays
    auto& outCheckedFeaturesRef = m_CheckedFeatures.getDataStoreRef();
    auto& outGroupingDensitiesRef = m_GroupingDensities.getDataStoreRef();

    usize numFeatures = featureVolumesRef.getNumberOfTuples();
    usize numParents = parentVolumesRef.getNumberOfTuples();

    float32 totalFeatureCheckVolume = 0.0f;
    float32 curParentVolume = 0.0f;
    std::unordered_set<int32> totalFeatureCheckList = {};

    std::vector<float32> checkedFeatureVolumes = {0.0f};
    if constexpr(FindDensitySpecializations::FindingCheckedFeatures)
    {
      // Default value-initialized to zeroes: https://en.cppreference.com/w/cpp/named_req/DefaultInsertable
      checkedFeatureVolumes.resize(numFeatures);
    }
    MessageHelper messageHelper(m_MessageHandler);
    auto throttledMessenger = messageHelper.createThrottledMessenger(
        [numParents](usize currentParentId) { return fmt::format("{}/{} {}%", currentParentId, numParents, CalculatePercentComplete(currentParentId, numParents)); }, std::chrono::milliseconds(1000));

    // Start the Parent Outer Loop
    for(usize currentParentId = 1; currentParentId < numParents; currentParentId++)
    {
      throttledMessenger.sendMessage(currentParentId);

      if(m_ShouldCancel)
      {
        return {};
      }

      // Loop on each feature.
      for(usize currentFeatureId = 1; currentFeatureId < numFeatures; currentFeatureId++)
      {
        // If the currentParentId is the same as the parentIds[currentFeatureId] and we have not added it to the `totalCheckList`
        // then increment the volumes
        if(featureParentIdsRef[currentFeatureId] == currentParentId)
        {
          if(!totalFeatureCheckList.contains(static_cast<int32>(currentFeatureId)))
          {
            totalFeatureCheckVolume += m_FeatureVolumes[currentFeatureId];      // Increment the checked volume by aggregating volumes from each feature that made up the parent feature
            totalFeatureCheckList.insert(static_cast<int32>(currentFeatureId)); // This is to the list of checked features

            if constexpr(FindDensitySpecializations::FindingCheckedFeatures)
            {
              if(parentVolumesRef[currentParentId] > checkedFeatureVolumes[currentFeatureId])
              {
                checkedFeatureVolumes[currentFeatureId] = parentVolumesRef[currentParentId];
                outCheckedFeaturesRef[currentFeatureId] = static_cast<int32>(currentParentId);
              }
            }
          }
          processNeighborListData(m_ContiguousNL, currentFeatureId, currentParentId, totalFeatureCheckList, totalFeatureCheckVolume, parentVolumesRef, checkedFeatureVolumes, outCheckedFeaturesRef);
          if constexpr(FindDensitySpecializations::UsingNonContiguousNeighbors)
          {
            processNeighborListData(m_NonContiguousNL, currentFeatureId, currentParentId, totalFeatureCheckList, totalFeatureCheckVolume, parentVolumesRef, checkedFeatureVolumes,
                                    outCheckedFeaturesRef);
          }
        }
      } // END OF FEATURE ID LOOP

      curParentVolume = parentVolumesRef[currentParentId];
      if(totalFeatureCheckVolume == 0.0f)
      {
        outGroupingDensitiesRef[currentParentId] = -1.0f;
      }
      else
      {
        outGroupingDensitiesRef[currentParentId] = (curParentVolume / totalFeatureCheckVolume);
      }
      totalFeatureCheckList.clear();
      totalFeatureCheckVolume = 0.0f;
    } // END OF PARENT ID LOOP

    return {};
  }

  void processNeighborListData(const NeighborList<int32>& neighborList, usize currentFeatureId, usize currentParentId, std::unordered_set<int32>& totalFeatureCheckList,
                               float32& totalFeatureCheckVolume, const AbstractDataStore<float>& parentVolumesRef, std::vector<float32>& checkedFeatureVolumes,
                               AbstractDataStore<int>& outCheckedFeaturesRef)
  {
    auto featureNeighbors = neighborList.at(currentFeatureId);
    auto numNeighbors = static_cast<int32>(featureNeighbors.size());

    for(int32 neighborIdx = 0; neighborIdx < numNeighbors; neighborIdx++)
    {
      auto neighborId = featureNeighbors.at(neighborIdx);

      // If the current neighbor is NOT in the check list...
      if(!totalFeatureCheckList.contains(neighborId))
      {
        // update the volumes and the check list
        totalFeatureCheckVolume += m_FeatureVolumes[neighborId]; // Increment the total volume for this neighbor
        totalFeatureCheckList.insert(neighborId);
        if constexpr(FindDensitySpecializations::FindingCheckedFeatures)
        {
          if(parentVolumesRef[currentParentId] > checkedFeatureVolumes[neighborId])
          {
            checkedFeatureVolumes[neighborId] = parentVolumesRef[currentParentId];
            outCheckedFeaturesRef[neighborId] = static_cast<int32>(currentParentId);
          }
        }
      }
    }
  }

private:
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  const Int32Array& m_ParentIds;
  const Float32Array& m_ParentVolumes;
  const Float32Array& m_FeatureVolumes;
  const Int32NeighborList& m_ContiguousNL;
  Float32Array& m_GroupingDensities;
  const Int32NeighborList& m_NonContiguousNL;
  Int32Array& m_CheckedFeatures;
};
} // namespace

// -----------------------------------------------------------------------------
ComputeGroupingDensity::ComputeGroupingDensity(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                               ComputeGroupingDensityInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
const std::atomic_bool& ComputeGroupingDensity::getCancel()
{
  return m_ShouldCancel;
}

// -----------------------------------------------------------------------------
Result<> ComputeGroupingDensity::operator()()
{
  const auto& parentIds = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->ParentIdsPath);
  const auto& parentVolumes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->ParentVolumesPath);
  const auto& featureVolumes = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->VolumesPath);
  const auto& contiguousNL = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->ContiguousNLPath);
  auto& groupingDensities = m_DataStructure.getDataRefAs<Float32Array>(m_InputValues->GroupingDensitiesPath);

  // These may or may not be empty depending on the parameters
  // The filter created some temporary hidden data array and neighbor list that may or may not
  // get used for this. This setup does ensure the next 2 lines will actually return something.
  const auto& nonContiguousNL = m_DataStructure.getDataRefAs<NeighborList<int32>>(m_InputValues->NonContiguousNLPath);
  auto& checkedFeatures = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CheckedFeaturesPath);

  if(m_InputValues->UseNonContiguousNeighbors)
  {
    if(m_InputValues->FindCheckedFeatures)
    {
      return ::FindDensityGrouping<FindDensitySpecializations<true, true>>(getCancel(), m_MessageHandler, parentIds, parentVolumes, featureVolumes, contiguousNL, groupingDensities, nonContiguousNL,
                                                                           checkedFeatures)();
    }
    return ::FindDensityGrouping<FindDensitySpecializations<true, false>>(getCancel(), m_MessageHandler, parentIds, parentVolumes, featureVolumes, contiguousNL, groupingDensities, nonContiguousNL,
                                                                          checkedFeatures)();
  }

  if(m_InputValues->FindCheckedFeatures)
  {
    return ::FindDensityGrouping<FindDensitySpecializations<false, true>>(getCancel(), m_MessageHandler, parentIds, parentVolumes, featureVolumes, contiguousNL, groupingDensities, nonContiguousNL,
                                                                          checkedFeatures)();
  }

  return ::FindDensityGrouping<FindDensitySpecializations<false, false>>(getCancel(), m_MessageHandler, parentIds, parentVolumes, featureVolumes, contiguousNL, groupingDensities, nonContiguousNL,
                                                                         checkedFeatures)();
}
