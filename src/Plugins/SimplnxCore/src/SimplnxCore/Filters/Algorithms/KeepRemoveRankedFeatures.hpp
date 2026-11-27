#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
/**
 * @brief Whether the ranked features are the ones to keep or the ones to remove.
 */
enum class RankOperation : uint64
{
  Keep = 0,
  Remove = 1,
};

/**
 * @brief Which end of the sorted population the selection is taken from.
 */
enum class RankDirection : uint64
{
  Largest = 0,
  Smallest = 1,
};

/**
 * @brief How many features the selection covers.
 */
enum class RankCriterion : uint64
{
  FeatureCount = 0,
  PercentOfCount = 1,
  PercentOfSum = 2,
};

struct SIMPLNXCORE_EXPORT KeepRemoveRankedFeaturesInputValues
{
  uint64 operation{};
  uint64 rank_from{};
  uint64 criterion{};
  uint64 num_features{};
  float64 percent{};
  bool fill_removed_features{};
  DataPath image_geometry_path;
  DataPath feature_ids_array_path;
  DataPath ranking_array_path;
  MultiArraySelectionParameter::ValueType ignored_data_array_paths;
};

/**
 * @class KeepRemoveRankedFeatures
 * @brief Keeps or removes a count or percentage of features ranked by a scalar feature array.
 */
class SIMPLNXCORE_EXPORT KeepRemoveRankedFeatures
{
public:
  KeepRemoveRankedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, KeepRemoveRankedFeaturesInputValues* inputValues);
  ~KeepRemoveRankedFeatures() noexcept = default;

  KeepRemoveRankedFeatures(const KeepRemoveRankedFeatures&) = delete;
  KeepRemoveRankedFeatures(KeepRemoveRankedFeatures&&) noexcept = delete;
  KeepRemoveRankedFeatures& operator=(const KeepRemoveRankedFeatures&) = delete;
  KeepRemoveRankedFeatures& operator=(KeepRemoveRankedFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const KeepRemoveRankedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
