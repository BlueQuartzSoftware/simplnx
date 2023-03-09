#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ConvertHexGridToSquareGridInputValues inputValues;

  inputValues.HexGridStack = filterArgs.value<<<<NOT_IMPLEMENTED>>>>(k_HexGridStack_Key);

  return ConvertHexGridToSquareGrid(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ConvertHexGridToSquareGridInputValues
{
  <<<NOT_IMPLEMENTED>>> HexGridStack;
  /*[x]*/
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ConvertHexGridToSquareGrid
{
public:
  ConvertHexGridToSquareGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertHexGridToSquareGridInputValues* inputValues);
  ~ConvertHexGridToSquareGrid() noexcept;

  ConvertHexGridToSquareGrid(const ConvertHexGridToSquareGrid&) = delete;
  ConvertHexGridToSquareGrid(ConvertHexGridToSquareGrid&&) noexcept = delete;
  ConvertHexGridToSquareGrid& operator=(const ConvertHexGridToSquareGrid&) = delete;
  ConvertHexGridToSquareGrid& operator=(ConvertHexGridToSquareGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertHexGridToSquareGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
