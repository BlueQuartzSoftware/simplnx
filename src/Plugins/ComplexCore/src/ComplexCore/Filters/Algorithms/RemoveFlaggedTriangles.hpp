#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{
struct COMPLEXCORE_EXPORT RemoveFlaggedTrianglesInputValues
{
  DataPath TriangleGeometry;
  DataPath MaskArrayPath;
  DataPath ReducedTriangleGeometry;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */
class COMPLEXCORE_EXPORT RemoveFlaggedTriangles
{
public:
  RemoveFlaggedTriangles(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, RemoveFlaggedTrianglesInputValues* inputValues);
  ~RemoveFlaggedTriangles() noexcept = default;

  RemoveFlaggedTriangles(const RemoveFlaggedTriangles&) = delete;
  RemoveFlaggedTriangles(RemoveFlaggedTriangles&&) noexcept = delete;
  RemoveFlaggedTriangles& operator=(const RemoveFlaggedTriangles&) = delete;
  RemoveFlaggedTriangles& operator=(RemoveFlaggedTriangles&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const RemoveFlaggedTrianglesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace complex
