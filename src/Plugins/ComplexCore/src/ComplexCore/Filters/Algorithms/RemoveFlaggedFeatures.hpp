#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

namespace complex
{

enum class Functionality : uint64
{
  Remove = 0,
  Extract = 1,
  ExtractThenRemove = 2,
};

struct COMPLEXCORE_EXPORT RemoveFlaggedFeaturesInputValues
{
  bool FillRemovedFeatures;
  uint64 ExtractFeatures;
  DataPath FeatureIdsArrayPath;
  DataPath FlaggedFeaturesArrayPath;
  DataPath ImageGeometryPath;
  std::string CreatedImageGeometryPrefix;
  MultiArraySelectionParameter::ValueType IgnoredDataArrayPaths;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT RemoveFlaggedFeatures
{
public:
  RemoveFlaggedFeatures(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedFeaturesInputValues* inputValues);
  ~RemoveFlaggedFeatures() noexcept;

  RemoveFlaggedFeatures(const RemoveFlaggedFeatures&) = delete;
  RemoveFlaggedFeatures(RemoveFlaggedFeatures&&) noexcept = delete;
  RemoveFlaggedFeatures& operator=(const RemoveFlaggedFeatures&) = delete;
  RemoveFlaggedFeatures& operator=(RemoveFlaggedFeatures&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedFeaturesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
