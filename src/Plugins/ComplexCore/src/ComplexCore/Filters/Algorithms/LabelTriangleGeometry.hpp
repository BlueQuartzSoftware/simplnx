#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT LabelTriangleGeometryInputValues
{
  DataPath TriangleGeomPath;
  DataPath RegionIdsPath;
  DataPath TriangleAMPath;
  DataPath NumTrianglesPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT LabelTriangleGeometry
{
public:
  LabelTriangleGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LabelTriangleGeometryInputValues* inputValues);
  ~LabelTriangleGeometry() noexcept;

  LabelTriangleGeometry(const LabelTriangleGeometry&) = delete;
  LabelTriangleGeometry(LabelTriangleGeometry&&) noexcept = delete;
  LabelTriangleGeometry& operator=(const LabelTriangleGeometry&) = delete;
  LabelTriangleGeometry& operator=(LabelTriangleGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LabelTriangleGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
