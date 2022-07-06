#pragma once

#include "SurfaceMeshing/SurfaceMeshing_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"

/**
* This is example code to put in the Execute Method of the filter.
  ReverseTriangleWindingInputValues inputValues;

  inputValues.SurfaceDataContainerName = filterArgs.value<DataPath>(k_SurfaceDataContainerName_Key);

  return ReverseTriangleWinding(dataStructure, messageHandler, shouldCancel, &inputValues)();
*/

namespace complex
{

struct SURFACEMESHING_EXPORT ReverseTriangleWindingInputValues
{
  DataPath SurfaceDataContainerName;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class SURFACEMESHING_EXPORT ReverseTriangleWinding
{
public:
  ReverseTriangleWinding(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReverseTriangleWindingInputValues* inputValues);
  ~ReverseTriangleWinding() noexcept;

  ReverseTriangleWinding(const ReverseTriangleWinding&) = delete;
  ReverseTriangleWinding(ReverseTriangleWinding&&) noexcept = delete;
  ReverseTriangleWinding& operator=(const ReverseTriangleWinding&) = delete;
  ReverseTriangleWinding& operator=(ReverseTriangleWinding&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReverseTriangleWindingInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
