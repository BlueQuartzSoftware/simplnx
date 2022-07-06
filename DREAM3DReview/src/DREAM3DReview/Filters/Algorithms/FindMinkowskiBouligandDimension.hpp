#pragma once

#include "DREAM3DReview/DREAM3DReview_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindMinkowskiBouligandDimensionInputValues inputValues;

  inputValues.MaskArrayPath = filterArgs.value<DataPath>(k_MaskArrayPath_Key);
  inputValues.AttributeMatrixName = filterArgs.value<DataPath>(k_AttributeMatrixName_Key);
  inputValues.MinkowskiBouligandDimensionArrayName = filterArgs.value<DataPath>(k_MinkowskiBouligandDimensionArrayName_Key);

  return FindMinkowskiBouligandDimension(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct DREAM3DREVIEW_EXPORT FindMinkowskiBouligandDimensionInputValues
{
  DataPath MaskArrayPath;
  DataPath AttributeMatrixName;
  DataPath MinkowskiBouligandDimensionArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class DREAM3DREVIEW_EXPORT FindMinkowskiBouligandDimension
{
public:
  FindMinkowskiBouligandDimension(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  FindMinkowskiBouligandDimensionInputValues* inputValues);
  ~FindMinkowskiBouligandDimension() noexcept;

  FindMinkowskiBouligandDimension(const FindMinkowskiBouligandDimension&) = delete;
  FindMinkowskiBouligandDimension(FindMinkowskiBouligandDimension&&) noexcept = delete;
  FindMinkowskiBouligandDimension& operator=(const FindMinkowskiBouligandDimension&) = delete;
  FindMinkowskiBouligandDimension& operator=(FindMinkowskiBouligandDimension&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindMinkowskiBouligandDimensionInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
