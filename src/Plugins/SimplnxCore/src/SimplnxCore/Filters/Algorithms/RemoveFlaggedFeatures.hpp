#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

enum class Functionality : uint64
{
  Remove = 0,
  Extract = 1,
  ExtractThenRemove = 2,
};

struct SIMPLNXCORE_EXPORT RemoveFlaggedFeaturesInputValues
{
  bool FillRemovedFeatures;
  uint64 ExtractFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath FlaggedFeaturesArrayPath;
  DataPath ImageGeometryPath;
  DataPath TempBoundsPath;
  std::string CreatedImageGeometryPrefix;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT RemoveFlaggedFeatures
{
public:
  RemoveFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedFeaturesInputValues* inputValues);
  ~RemoveFlaggedFeatures() noexcept;

  RemoveFlaggedFeatures(const RemoveFlaggedFeatures&) = delete;
  RemoveFlaggedFeatures(RemoveFlaggedFeatures&&) noexcept = delete;
  RemoveFlaggedFeatures& operator=(const RemoveFlaggedFeatures&) = delete;
  RemoveFlaggedFeatures& operator=(RemoveFlaggedFeatures&&) noexcept = delete;

  Result<> operator()();

  void sendThreadSafeProgressMessage(size_t counter);

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  // Threadsafe Progress Message
  mutable std::mutex m_ProgressMessage_Mutex;
  std::chrono::steady_clock::time_point m_InitialTime = std::chrono::steady_clock::now();
  size_t m_TotalElements = 0;
  size_t m_ProgressCounter = 0;
};

} // namespace nx::core
