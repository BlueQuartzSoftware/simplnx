#pragma once

#include "ProgWorkshop/ProgWorkshop_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  Lesson6InputValues inputValues;

  inputValues.GeometrySelection = filterArgs.value<DataPath>(k_GeometrySelection_Key);

  return Lesson6(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct PROGWORKSHOP_EXPORT Lesson6InputValues
{
  DataPath GeometrySelection;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class PROGWORKSHOP_EXPORT Lesson6
{
public:
  Lesson6(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, Lesson6InputValues* inputValues);
  ~Lesson6() noexcept;

  Lesson6(const Lesson6&) = delete;
  Lesson6(Lesson6&&) noexcept = delete;
  Lesson6& operator=(const Lesson6&) = delete;
  Lesson6& operator=(Lesson6&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const Lesson6InputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
