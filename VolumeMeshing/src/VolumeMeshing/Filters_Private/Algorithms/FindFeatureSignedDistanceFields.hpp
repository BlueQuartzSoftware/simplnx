#pragma once

#include "VolumeMeshing/VolumeMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindFeatureSignedDistanceFieldsInputValues inputValues;

  inputValues.FeatureIdsArrayPath = filterArgs.value<DataPath>(k_FeatureIdsArrayPath_Key);
  inputValues.FeatureAttributeMatrixPath = filterArgs.value<DataPath>(k_FeatureAttributeMatrixPath_Key);
  inputValues.SignedDistanceFieldsPrefix = filterArgs.value<StringParameter::ValueType>(k_SignedDistanceFieldsPrefix_Key);

  return FindFeatureSignedDistanceFields(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct VOLUMEMESHING_EXPORT FindFeatureSignedDistanceFieldsInputValues
{
  DataPath FeatureIdsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  StringParameter::ValueType SignedDistanceFieldsPrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class VOLUMEMESHING_EXPORT FindFeatureSignedDistanceFields
{
public:
  FindFeatureSignedDistanceFields(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel,
                                  FindFeatureSignedDistanceFieldsInputValues* inputValues);
  ~FindFeatureSignedDistanceFields() noexcept;

  FindFeatureSignedDistanceFields(const FindFeatureSignedDistanceFields&) = delete;
  FindFeatureSignedDistanceFields(FindFeatureSignedDistanceFields&&) noexcept = delete;
  FindFeatureSignedDistanceFields& operator=(const FindFeatureSignedDistanceFields&) = delete;
  FindFeatureSignedDistanceFields& operator=(FindFeatureSignedDistanceFields&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindFeatureSignedDistanceFieldsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
