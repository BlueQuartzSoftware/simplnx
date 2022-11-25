#pragma once

#include "Core/Core_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.

*/

namespace complex
{

struct CORE_EXPORT FindTriangleGeomSizesInputValues
{
  DataPath TriangleGeometryPath;
  DataPath FaceLabelsArrayPath;
  DataPath FeatureAttributeMatrixPath;
  DataPath VolumesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class CORE_EXPORT FindTriangleGeomSizes
{
public:
  FindTriangleGeomSizes(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindTriangleGeomSizesInputValues* inputValues);
  ~FindTriangleGeomSizes() noexcept;

  FindTriangleGeomSizes(const FindTriangleGeomSizes&) = delete;
  FindTriangleGeomSizes(FindTriangleGeomSizes&&) noexcept = delete;
  FindTriangleGeomSizes& operator=(const FindTriangleGeomSizes&) = delete;
  FindTriangleGeomSizes& operator=(FindTriangleGeomSizes&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindTriangleGeomSizesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
