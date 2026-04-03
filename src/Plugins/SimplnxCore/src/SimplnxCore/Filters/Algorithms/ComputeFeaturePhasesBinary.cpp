#include "ComputeFeaturePhasesBinary.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/MaskCompareUtilities.hpp"
#include "simplnx/Utilities/MessageHelper.hpp"

using namespace nx::core;

// -----------------------------------------------------------------------------
ComputeFeaturePhasesBinary::ComputeFeaturePhasesBinary(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                                       ComputeFeaturePhasesBinaryInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
ComputeFeaturePhasesBinary::~ComputeFeaturePhasesBinary() noexcept = default;

// -----------------------------------------------------------------------------
Result<> ComputeFeaturePhasesBinary::operator()()
{
  auto& featureIdsStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->FeatureIdsArrayPath)->getDataStoreRef();
  auto& featurePhasesStoreRef = m_DataStructure.getDataAs<Int32Array>(m_InputValues->CellDataAttributeMatrixPath.createChildPath(m_InputValues->FeaturePhasesArrayName))->getDataStoreRef();

  std::unique_ptr<MaskCompareUtilities::MaskCompare> goodVoxelsMask;
  try
  {
    goodVoxelsMask = MaskCompareUtilities::InstantiateMaskCompare(m_DataStructure, m_InputValues->MaskArrayPath);
  } catch(const std::out_of_range& exception)
  {
    // This really should NOT be happening as the path was verified during preflight BUT we may be calling this from
    // somewhere else that is NOT going through the normal nx::core::IFilter API of Preflight and Execute
    std::string message = fmt::format("Mask Array DataPath does not exist or is not of the correct type (Bool | UInt8) {}", m_InputValues->MaskArrayPath.toString());
    return MakeErrorResult(-53800, message);
  }

  usize totalPoints = featureIdsStoreRef.getNumberOfTuples();

  MessageHelper messageHelper(m_MessageHandler);
  auto progressHelper = messageHelper.createProgressMessageHelper();
  progressHelper.setMaxProgresss(totalPoints);
  progressHelper.setProgressMessageTemplate("Computing Feature Phases Binary: {:.1f}% Complete");
  auto progressMessenger = progressHelper.createProgressMessenger(std::chrono::milliseconds(1000));

  for(usize i = 0; i < totalPoints; i++)
  {
    featurePhasesStoreRef[featureIdsStoreRef[i]] = goodVoxelsMask->isTrue(i);
    progressMessenger.sendProgressMessage(1);
  }

  return {};
}
