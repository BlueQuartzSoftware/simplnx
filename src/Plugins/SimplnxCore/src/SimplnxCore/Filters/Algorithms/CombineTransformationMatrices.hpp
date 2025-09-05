#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CombineTransformationMatricesInputValues
{
  MultiArraySelectionParameter::ValueType SelectedPaths;
  ArrayCreationParameter::ValueType OutputPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT CombineTransformationMatrices
{
public:
  CombineTransformationMatrices(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineTransformationMatricesInputValues* inputValues);
  ~CombineTransformationMatrices() noexcept;

  CombineTransformationMatrices(const CombineTransformationMatrices&) = delete;
  CombineTransformationMatrices(CombineTransformationMatrices&&) noexcept = delete;
  CombineTransformationMatrices& operator=(const CombineTransformationMatrices&) = delete;
  CombineTransformationMatrices& operator=(CombineTransformationMatrices&&) noexcept = delete;

  // Error Codes
  enum class ErrorCodes : int32
  {
    EmptyInputArrays = -2350,
    OneInputArray = -2351,
    NonPositiveTupleDimValue = -2352,
    TypeNameMismatch = -2353,
    ComponentShapeMismatch = -2354,
    InputArraysEqualAny = -2355,
    InputArraysUnsupported = -2356,
    WrongElementCount = -2357
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const CombineTransformationMatricesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
