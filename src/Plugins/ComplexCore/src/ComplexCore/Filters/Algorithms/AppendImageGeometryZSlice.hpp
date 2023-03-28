#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/DataGroupSelectionParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT AppendImageGeometryZSliceInputValues
{
  DataPath InputGeometryPath;
  DataPath DestinationGeometryPath;
  bool CheckResolution;
  bool SaveAsNewGeometry;
};

/**
 * @class AppendImageGeometryZSlice
 * @brief This filter allows the user to append an Image Geometry onto the "end" of another Image Geometry.
 */
class COMPLEXCORE_EXPORT AppendImageGeometryZSlice
{
public:
  AppendImageGeometryZSlice(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AppendImageGeometryZSliceInputValues* inputValues);
  ~AppendImageGeometryZSlice() noexcept;

  AppendImageGeometryZSlice(const AppendImageGeometryZSlice&) = delete;
  AppendImageGeometryZSlice(AppendImageGeometryZSlice&&) noexcept = delete;
  AppendImageGeometryZSlice& operator=(const AppendImageGeometryZSlice&) = delete;
  AppendImageGeometryZSlice& operator=(AppendImageGeometryZSlice&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AppendImageGeometryZSliceInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
