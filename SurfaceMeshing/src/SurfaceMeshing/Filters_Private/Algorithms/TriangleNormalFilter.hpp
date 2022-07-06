#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  TriangleNormalFilterInputValues inputValues;

  inputValues.SurfaceMeshTriangleNormalsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTriangleNormalsArrayPath_Key);

  return TriangleNormalFilter(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT TriangleNormalFilterInputValues
{
  DataPath SurfaceMeshTriangleNormalsArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT TriangleNormalFilter
{
public:
  TriangleNormalFilter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, TriangleNormalFilterInputValues* inputValues);
  ~TriangleNormalFilter() noexcept;

  TriangleNormalFilter(const TriangleNormalFilter&) = delete;
  TriangleNormalFilter(TriangleNormalFilter&&) noexcept = delete;
  TriangleNormalFilter& operator=(const TriangleNormalFilter&) = delete;
  TriangleNormalFilter& operator=(TriangleNormalFilter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const TriangleNormalFilterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
