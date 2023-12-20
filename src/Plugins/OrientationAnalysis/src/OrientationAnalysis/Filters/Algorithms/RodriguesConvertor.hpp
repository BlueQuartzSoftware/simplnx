#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT RodriguesConvertorInputValues
{
  DataPath RodriguesDataArrayPath;
  DataPath OutputDataArrayPath;
  bool DeleteOriginalData;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT RodriguesConvertor
{
public:
  RodriguesConvertor(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RodriguesConvertorInputValues* inputValues);
  ~RodriguesConvertor() noexcept;

  RodriguesConvertor(const RodriguesConvertor&) = delete;
  RodriguesConvertor(RodriguesConvertor&&) noexcept = delete;
  RodriguesConvertor& operator=(const RodriguesConvertor&) = delete;
  RodriguesConvertor& operator=(RodriguesConvertor&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RodriguesConvertorInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
