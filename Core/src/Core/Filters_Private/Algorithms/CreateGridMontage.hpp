#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  CreateGridMontageInputValues inputValues;


  return CreateGridMontage(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT CreateGridMontageInputValues
{
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT CreateGridMontage
{
public:
  CreateGridMontage(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CreateGridMontageInputValues* inputValues);
  ~CreateGridMontage() noexcept;

  CreateGridMontage(const CreateGridMontage&) = delete;
  CreateGridMontage(CreateGridMontage&&) noexcept = delete;
  CreateGridMontage& operator=(const CreateGridMontage&) = delete;
  CreateGridMontage& operator=(CreateGridMontage&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CreateGridMontageInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
