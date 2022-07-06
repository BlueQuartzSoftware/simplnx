#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TriangleCentroidFilterInputValues inputValues;

  inputValues.SurfaceMeshTriangleCentroidsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTriangleCentroidsArrayPath_Key);

  return TriangleCentroidFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT TriangleCentroidFilterInputValues
{
  DataPath SurfaceMeshTriangleCentroidsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT TriangleCentroidFilter
{
public:
  TriangleCentroidFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleCentroidFilterInputValues* inputValues);
  ~TriangleCentroidFilter() noexcept;

  TriangleCentroidFilter(const TriangleCentroidFilter&) = delete;
  TriangleCentroidFilter(TriangleCentroidFilter&&) noexcept = delete;
  TriangleCentroidFilter& operator=(const TriangleCentroidFilter&) = delete;
  TriangleCentroidFilter& operator=(TriangleCentroidFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TriangleCentroidFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
