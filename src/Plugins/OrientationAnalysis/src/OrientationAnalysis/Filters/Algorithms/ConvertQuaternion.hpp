#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ConvertQuaternionInputValues
{
  DataPath QuaternionDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
  ChoicesParameter::ValueType ConversionType;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ConvertQuaternion
{
public:
  ConvertQuaternion(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertQuaternionInputValues* inputValues);
  ~ConvertQuaternion() noexcept;

  ConvertQuaternion(const ConvertQuaternion&) = delete;
  ConvertQuaternion(ConvertQuaternion&&) noexcept = delete;
  ConvertQuaternion& operator=(const ConvertQuaternion&) = delete;
  ConvertQuaternion& operator=(ConvertQuaternion&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertQuaternionInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
