#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT FindBoundaryCellsInputValues
{
  bool IgnoreFeatureZero;
  bool IncludeVolumeBoundary;
  DataPath ImageGeometryPath;
  DataPath FeatureIdsArrayPath;
  DataPath BoundaryCellsArrayName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT FindBoundaryCells
{
public:
  FindBoundaryCells(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryCellsInputValues* inputValues);
  ~FindBoundaryCells() noexcept;

  FindBoundaryCells(const FindBoundaryCells&) = delete;
  FindBoundaryCells(FindBoundaryCells&&) noexcept = delete;
  FindBoundaryCells& operator=(const FindBoundaryCells&) = delete;
  FindBoundaryCells& operator=(FindBoundaryCells&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundaryCellsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
