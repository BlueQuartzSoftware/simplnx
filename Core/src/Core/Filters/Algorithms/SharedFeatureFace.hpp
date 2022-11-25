#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

namespace complex
{

struct CORE_EXPORT SharedFeatureFaceInputValues
{
  DataPath TriangleGeometryPath;

  DataPath FaceLabelsArrayPath;
  DataPath FeatureFaceIdsArrayPath;
  DataPath GrainBoundaryAttributeMatrixPath;
  DataPath FeatureFaceLabelsArrayPath;
  DataPath FeatureFaceNumTrianglesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT SharedFeatureFace
{
public:
  SharedFeatureFace(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, SharedFeatureFaceInputValues* inputValues);
  ~SharedFeatureFace() noexcept;

  SharedFeatureFace(const SharedFeatureFace&) = delete;
  SharedFeatureFace(SharedFeatureFace&&) noexcept = delete;
  SharedFeatureFace& operator=(const SharedFeatureFace&) = delete;
  SharedFeatureFace& operator=(SharedFeatureFace&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const SharedFeatureFaceInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
