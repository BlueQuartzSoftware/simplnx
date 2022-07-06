#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TriangleDihedralAngleFilterInputValues inputValues;

  inputValues.SurfaceMeshTriangleDihedralAnglesArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTriangleDihedralAnglesArrayPath_Key);

  return TriangleDihedralAngleFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT TriangleDihedralAngleFilterInputValues
{
  DataPath SurfaceMeshTriangleDihedralAnglesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT TriangleDihedralAngleFilter
{
public:
  TriangleDihedralAngleFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleDihedralAngleFilterInputValues* inputValues);
  ~TriangleDihedralAngleFilter() noexcept;

  TriangleDihedralAngleFilter(const TriangleDihedralAngleFilter&) = delete;
  TriangleDihedralAngleFilter(TriangleDihedralAngleFilter&&) noexcept = delete;
  TriangleDihedralAngleFilter& operator=(const TriangleDihedralAngleFilter&) = delete;
  TriangleDihedralAngleFilter& operator=(TriangleDihedralAngleFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TriangleDihedralAngleFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
