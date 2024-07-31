#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AppendImageGeometryZSliceInputValues
{
  std::vector<DataPath> InputGeometriesPaths;
  DataPath DestinationGeometryPath;
  DataPath NewGeometryPath;
  bool CheckResolution;
  bool SaveAsNewGeometry;
  CopyFromArray::Direction Direction;
};

/**
 * @class AppendImageGeometryZSlice
 * @brief This filter allows the user to append an Image Geometry onto the "end" of another Image Geometry.
 */
class SIMPLNXCORE_EXPORT AppendImageGeometryZSlice
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

} // namespace nx::core
