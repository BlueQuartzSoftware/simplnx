#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TriangleAreaFilterInputValues inputValues;

  inputValues.SurfaceMeshTriangleAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);

  return TriangleAreaFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT TriangleAreaFilterInputValues
{
  DataPath SurfaceMeshTriangleAreasArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT TriangleAreaFilter
{
public:
  TriangleAreaFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleAreaFilterInputValues* inputValues);
  ~TriangleAreaFilter() noexcept;

  TriangleAreaFilter(const TriangleAreaFilter&) = delete;
  TriangleAreaFilter(TriangleAreaFilter&&) noexcept = delete;
  TriangleAreaFilter& operator=(const TriangleAreaFilter&) = delete;
  TriangleAreaFilter& operator=(TriangleAreaFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TriangleAreaFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
