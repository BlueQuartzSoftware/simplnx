#pragma once

#include "ProgWorkshop/ProgWorkshop_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Lesson7InputValues inputValues;

  inputValues.GeometrySelection = filterArgs.value<DataPath>(k_GeometrySelection_Key);

  return Lesson7(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROGWORKSHOP_EXPORT Lesson7InputValues
{
  DataPath GeometrySelection;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROGWORKSHOP_EXPORT Lesson7
{
public:
  Lesson7(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson7InputValues* inputValues);
  ~Lesson7() noexcept;

  Lesson7(const Lesson7&) = delete;
  Lesson7(Lesson7&&) noexcept = delete;
  Lesson7& operator=(const Lesson7&) = delete;
  Lesson7& operator=(Lesson7&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Lesson7InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
