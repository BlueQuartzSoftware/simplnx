#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Utilities/ThrottledMessageHandler.hpp"

#include <mutex>
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/GeometrySelectionParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT RequireMinimumSizeFeaturesInputValues
{
  BoolParameter::ValueType ApplySinglePhase;
  ArraySelectionParameter::ValueType FeatureIdsPath;
  ArraySelectionParameter::ValueType FeaturePhasesPath;
  GeometrySelectionParameter::ValueType InputImageGeometryPath;
  Int64Parameter::ValueType MinAllowedFeaturesSize;
  ArraySelectionParameter::ValueType FeatureNumCellsPath;
  Int32Parameter::ValueType PhaseNumber;
};

/**
 * @class RequireMinimumSizeFeatures
 * @brief This algorithm implements support code for the RequireMinimumSizeFeaturesFilter
 */

class SIMPLNXCORE_EXPORT RequireMinimumSizeFeatures
{
public:
  RequireMinimumSizeFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequireMinimumSizeFeaturesInputValues* inputValues);
  ~RequireMinimumSizeFeatures() noexcept;

  RequireMinimumSizeFeatures(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures(RequireMinimumSizeFeatures&&) noexcept = delete;
  RequireMinimumSizeFeatures& operator=(const RequireMinimumSizeFeatures&) = delete;
  RequireMinimumSizeFeatures& operator=(RequireMinimumSizeFeatures&&) noexcept = delete;

  Result<> operator()();

  /**
   * @brief Thread-safe progress update. Safe to call from the parallel per-array workers.
   * @param message Fully rendered progress text for one array
   */
  void sendThreadSafeProgressMessage(const std::string& message);

protected:
  /**
   *
   * @param dimensions
   * @param featureNumCellsStoreRef
   */
  void assignBadVoxels(SizeVec3 dimensions, const Int32AbstractDataStore& featureNumCellsStoreRef);

  /**
   *
   * @param featureIdsStoreRef
   * @param featureNumCellsStoreRef
   * @param featurePhases
   * @param phaseNumber
   * @param applyToSinglePhase
   * @param minAllowedFeatureSize
   * @param errorReturn
   * @return
   */
  std::vector<bool> removeSmallFeatures(Int32AbstractDataStore& featureIdsStoreRef, const Int32AbstractDataStore& featureNumCellsStoreRef, const Int32AbstractDataStore* featurePhases,
                                        int32_t phaseNumber, bool applyToSinglePhase, int64 minAllowedFeatureSize, Error& errorReturn);

private:
  DataStructure& m_DataStructure;
  const RequireMinimumSizeFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
  mutable std::mutex m_ProgressMessage_Mutex;
  ThrottledMessageHandler m_Throttle;
};

} // namespace nx::core
