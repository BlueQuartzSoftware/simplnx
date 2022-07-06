#pragma once

#include "UCSBUtilities/UCSBUtilities_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  FindBoundaryAreasInputValues inputValues;

  inputValues.SurfaceMeshTriangleAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshTriangleAreasArrayPath_Key);
  inputValues.SurfaceMeshFeatureFaceIdsArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshFeatureFaceIdsArrayPath_Key);
  inputValues.SurfaceMeshBoundaryAreasArrayPath = filterArgs.value<DataPath>(k_SurfaceMeshBoundaryAreasArrayPath_Key);

  return FindBoundaryAreas(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct UCSBUTILITIES_EXPORT FindBoundaryAreasInputValues
{
  DataPath SurfaceMeshTriangleAreasArrayPath;
  DataPath SurfaceMeshFeatureFaceIdsArrayPath;
  DataPath SurfaceMeshBoundaryAreasArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class UCSBUTILITIES_EXPORT FindBoundaryAreas
{
public:
  FindBoundaryAreas(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, FindBoundaryAreasInputValues* inputValues);
  ~FindBoundaryAreas() noexcept;

  FindBoundaryAreas(const FindBoundaryAreas&) = delete;
  FindBoundaryAreas(FindBoundaryAreas&&) noexcept = delete;
  FindBoundaryAreas& operator=(const FindBoundaryAreas&) = delete;
  FindBoundaryAreas& operator=(FindBoundaryAreas&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const FindBoundaryAreasInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
