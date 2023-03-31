#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindLargestCrossSectionsInputValues
{
  ChoicesParameter::ValueType Plane;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath LargestCrossSectionsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindLargestCrossSections
{
public:
  FindLargestCrossSections(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindLargestCrossSectionsInputValues* inputValues);
  ~FindLargestCrossSections() noexcept;

  FindLargestCrossSections(const FindLargestCrossSections&) = delete;
  FindLargestCrossSections(FindLargestCrossSections&&) noexcept = delete;
  FindLargestCrossSections& operator=(const FindLargestCrossSections&) = delete;
  FindLargestCrossSections& operator=(FindLargestCrossSections&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindLargestCrossSectionsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
