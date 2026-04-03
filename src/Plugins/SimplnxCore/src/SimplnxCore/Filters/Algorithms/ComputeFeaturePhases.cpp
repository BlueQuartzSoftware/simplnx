#include "ComputeFeaturePhases.hpp"

#include "simplnx/DataStructure/AttributeMatrix.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

#include <map>
#include <set>

using namespace nx::core;

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
  auto featurePhasesArrayPath = m_InputValues->CellFeaturesAttributeMatrixPath.createChildPath(m_InputValues->FeaturePhasesArrayName);

  const auto& cellPhases = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellPhasesArrayPath)->getDataStoreRef();
  const auto& featureIdsArray = m_DataStructure.getDataRefAs<Int32Array>(m_InputValues->FeatureIdsPath);
  const auto& featureIds = featureIdsArray.getDataStoreRef();
  auto& featurePhases = m_DataStructure.getDataAs<Int32Array>(featurePhasesArrayPath)->getDataStoreRef();

  // Validate the featurePhases array is the proper size
  auto validateNumFeatResult = ValidateFeatureIdsToFeatureAttributeMatrixIndexing(m_DataStructure, m_InputValues->CellFeaturesAttributeMatrixPath, featureIdsArray, false, m_MessageHandler);
  if(validateNumFeatResult.invalid())
  {
    return validateNumFeatResult;
  }

  usize totalPoints = featureIds.getNumberOfTuples();
  std::map<int32, int32> featureMap;
  std::set<int32> warnFeatures;

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(totalPoints);
  progressHelper.setProgressMessageTemplate("Computing Feature Phases: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize i = 0; i < totalPoints; i++)
  {
    if(m_ShouldCancel)
    {
      return {};
    }

    int32 gnum = featureIds[i];
    featureMap.insert({gnum, cellPhases[i]});

    int32 curPhaseVal = featureMap[gnum];
    if(curPhaseVal != cellPhases[i])
    {
      warnFeatures.insert(gnum);
    }
    featurePhases[gnum] = cellPhases[i];
    progressMessenger.sendProgressMessage(1);
  }

  Result<> result;
  if(!warnFeatures.empty())
  {
    std::string warnStr = "Elements from some features did not all have the same phase ID. The last phase ID copied into each feature will be used. Effected Phase Features: ";
    usize position = 0;
    for(auto value : warnFeatures)
    {
      warnStr.append(std::to_string(value));
      if(++position != warnFeatures.size())
      {
        warnStr.append(", ");
      }
    }
    result.warnings().push_back(Warning{-500, std::move(warnStr)});
  }

  return result;
}
