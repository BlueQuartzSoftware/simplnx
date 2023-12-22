#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT GroupFeaturesInputValues
{
  bool UseNonContiguousNeighbors;
  DataPath NonContiguousNeighborListArrayPath;
  DataPath ContiguousNeighborListArrayPath;
  bool m_PatchGrouping = false;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT GroupFeatures
{
public:
  GroupFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GroupFeaturesInputValues* inputValues);
  ~GroupFeatures() noexcept;

  GroupFeatures(const GroupFeatures&) = delete;
  GroupFeatures(GroupFeatures&&) noexcept = delete;
  GroupFeatures& operator=(const GroupFeatures&) = delete;
  GroupFeatures& operator=(GroupFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  virtual int getSeed(int32 newFid);
  virtual bool determineGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid);
  virtual bool growPatch(int32 currentPatch);
  virtual bool growGrouping(int32 referenceFeature, int32 neighborFeature, int32 newFid);

protected:
  DataStructure& m_DataStructure;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  void execute();

private:
  const GroupFeaturesInputValues* m_GroupInputValues = nullptr;
};

} // namespace nx::core
