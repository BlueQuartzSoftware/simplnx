#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  RequiredZThicknessInputValues inputValues;


  return RequiredZThickness(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT RequiredZThicknessInputValues
{
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT RequiredZThickness
{
public:
  RequiredZThickness(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RequiredZThicknessInputValues* inputValues);
  ~RequiredZThickness() noexcept;

  RequiredZThickness(const RequiredZThickness&) = delete;
  RequiredZThickness(RequiredZThickness&&) noexcept = delete;
  RequiredZThickness& operator=(const RequiredZThickness&) = delete;
  RequiredZThickness& operator=(RequiredZThickness&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RequiredZThicknessInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
