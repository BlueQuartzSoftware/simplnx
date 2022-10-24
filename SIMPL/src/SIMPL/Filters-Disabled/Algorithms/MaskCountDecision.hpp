#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  MaskCountDecisionInputValues inputValues;


  return MaskCountDecision(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT MaskCountDecisionInputValues
{
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT MaskCountDecision
{
public:
  MaskCountDecision(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, MaskCountDecisionInputValues* inputValues);
  ~MaskCountDecision() noexcept;

  MaskCountDecision(const MaskCountDecision&) = delete;
  MaskCountDecision(MaskCountDecision&&) noexcept = delete;
  MaskCountDecision& operator=(const MaskCountDecision&) = delete;
  MaskCountDecision& operator=(MaskCountDecision&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const MaskCountDecisionInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
