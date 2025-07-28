#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ComputeDifferencesMapInputValues
{
  ArrayCreationParameter::ValueType DifferenceMapArrayPath;
  ArraySelectionParameter::ValueType FirstInputArrayPath;
  ArraySelectionParameter::ValueType SecondInputArrayPath;
};

/**
 * @class ComputeDifferencesMap
 * @brief This algorithm implements support code for the ComputeDifferencesMapFilter
 */

class SIMPLNXCORE_EXPORT ComputeDifferencesMap
{
public:
  ComputeDifferencesMap(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ComputeDifferencesMapInputValues* inputValues);
  ~ComputeDifferencesMap() noexcept;

  ComputeDifferencesMap(const ComputeDifferencesMap&) = delete;
  ComputeDifferencesMap(ComputeDifferencesMap&&) noexcept = delete;
  ComputeDifferencesMap& operator=(const ComputeDifferencesMap&) = delete;
  ComputeDifferencesMap& operator=(ComputeDifferencesMap&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ComputeDifferencesMapInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
