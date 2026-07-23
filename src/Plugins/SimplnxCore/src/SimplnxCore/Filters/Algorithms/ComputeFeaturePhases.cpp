#include "ComputeFeaturePhases.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

using namespace nx::core;

namespace
{
constexpr usize k_MaxListedFeatures = 15;
}

// -----------------------------------------------------------------------------
ComputeFeaturePhases::ComputeFeaturePhases(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeFeaturePhasesInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeaturePhases::~ComputeFeaturePhases() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeaturePhases::operator()()
{
  const auto featurePhasesArrayPath = m_InputValues->CellFeaturesAttributeMatrixPath.createChildPath(m_InputValues->FeaturePhasesArrayName);

  const auto& cellPhasesStore = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->CellPhasesArrayPath).getDataStoreRef();
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);
  const auto& featureIdsStore = featureIdsArray.getDataStoreRef();
  auto& featurePhasesStore = m_DataStructure.getDataRefAs<Int32Array>(featurePhasesArrayPath).getDataStoreRef();

  // Validate the featurePhases array sizing matches feature index values in featureIds (bounds check) and feature ids are all positive
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->CellFeaturesAttributeMatrixPath, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  const usize totalPoints = featureIdsStore.getNumberOfTuples();
  const usize numFeatures = featurePhasesStore.getNumberOfTuples();
  std::vector<int32> initialFeaturePhase(numFeatures, -1);
  std::vector<bool> warnFeature(numFeatures, false);

  for(usize i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    const int32 featureId = featureIdsStore.getValue(i);
    // Ignore 0, invalid feature
    if(featureId == 0)
    {
      continue;
    }

    const int32 currentPhaseId = cellPhasesStore.getValue(i);

    if(currentPhaseId < 0)
    {
      return MakeErrorResult(-61861, fmt::format("Cell phases contains a negative value. Index: {} | Value: {}", i, currentPhaseId));
    }

    const int32 storedPhaseId = initialFeaturePhase[featureId];
    if(storedPhaseId == -1)
    {
      initialFeaturePhase[featureId] = currentPhaseId;
    }
    else if(storedPhaseId != currentPhaseId)
    {
      warnFeature[featureId] = true;
    }

    featurePhasesStore.setValue(static_cast<usize>(featureId), currentPhaseId);
  }

  Result<> result;

  std::string warnStr = "Elements from some features did not all have the same phase ID. The last phase ID copied into each feature will be used. Affected Phase Features: ";
  usize count = 0;
  // Ignore 0 it is not a valid feature
  for(usize i = 1; i < numFeatures; i++)
  {
    if(warnFeature[i])
    {
      if(count < k_MaxListedFeatures)
      {
        if(count > 0)
        {
          warnStr.append(", ");
        }
        warnStr.append(std::to_string(i));
      }
      count++;
    }
  }

  if(count != 0)
  {
    if(count > k_MaxListedFeatures)
    {
      usize remainder = count - k_MaxListedFeatures;
      warnStr.append(fmt::format(", and {} more {}", remainder, remainder == 1 ? "occurrence" : "occurrences"));
    }
    result.warnings().push_back(Warning{-500, std::move(warnStr)});
  }

  return result;
}
