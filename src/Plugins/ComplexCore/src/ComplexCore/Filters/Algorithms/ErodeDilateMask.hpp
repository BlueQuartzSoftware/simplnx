#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"

namespace
{
const std::string k_ErodeString("Erode");
const std::string k_DilateString("Dilate");
const complex::ChoicesParameter::Choices k_OperationChoices = {k_ErodeString, k_DilateString};

const complex::ChoicesParameter::ValueType k_ErodeIndex = 0ULL;
const complex::ChoicesParameter::ValueType k_DilateIndex = 1ULL;
} // namespace

namespace complex
{

struct COMPLEXCORE_EXPORT ErodeDilateMaskInputValues
{
  ChoicesParameter::ValueType Operation;
  int32 NumIterations;
  bool XDirOn;
  bool YDirOn;
  bool ZDirOn;
  DataPath MaskArrayPath;
  DataPath InputImageGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT ErodeDilateMask
{
public:
  ErodeDilateMask(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ErodeDilateMaskInputValues* inputValues);
  ~ErodeDilateMask() noexcept;

  ErodeDilateMask(const ErodeDilateMask&) = delete;
  ErodeDilateMask(ErodeDilateMask&&) noexcept = delete;
  ErodeDilateMask& operator=(const ErodeDilateMask&) = delete;
  ErodeDilateMask& operator=(ErodeDilateMask&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ErodeDilateMaskInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
