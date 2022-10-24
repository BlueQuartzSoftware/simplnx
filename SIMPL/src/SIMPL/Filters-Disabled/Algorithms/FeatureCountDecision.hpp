#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FeatureCountDecisionInputValues inputValues;


  return FeatureCountDecision(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT FeatureCountDecisionInputValues
{
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT FeatureCountDecision
{
public:
  FeatureCountDecision(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FeatureCountDecisionInputValues* inputValues);
  ~FeatureCountDecision() noexcept;

  FeatureCountDecision(const FeatureCountDecision&) = delete;
  FeatureCountDecision(FeatureCountDecision&&) noexcept = delete;
  FeatureCountDecision& operator=(const FeatureCountDecision&) = delete;
  FeatureCountDecision& operator=(FeatureCountDecision&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FeatureCountDecisionInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
