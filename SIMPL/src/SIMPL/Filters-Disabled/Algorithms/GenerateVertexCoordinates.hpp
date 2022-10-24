#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  GenerateVertexCoordinatesInputValues inputValues;

  inputValues.SelectedDataContainerName = filterArgs.value<DataPath>(k_SelectedDataContainerName_Key);
  inputValues.CoordinateArrayPath = filterArgs.value<DataPath>(k_CoordinateArrayPath_Key);

  return GenerateVertexCoordinates(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct CORE_EXPORT GenerateVertexCoordinatesInputValues
{
  DataPath SelectedDataContainerName;
  DataPath CoordinateArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT GenerateVertexCoordinates
{
public:
  GenerateVertexCoordinates(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, GenerateVertexCoordinatesInputValues* inputValues);
  ~GenerateVertexCoordinates() noexcept;

  GenerateVertexCoordinates(const GenerateVertexCoordinates&) = delete;
  GenerateVertexCoordinates(GenerateVertexCoordinates&&) noexcept = delete;
  GenerateVertexCoordinates& operator=(const GenerateVertexCoordinates&) = delete;
  GenerateVertexCoordinates& operator=(GenerateVertexCoordinates&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const GenerateVertexCoordinatesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
