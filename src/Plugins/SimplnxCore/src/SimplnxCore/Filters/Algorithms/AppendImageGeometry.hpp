#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AppendImageGeometryInputValues
{
  std::vector<DataPath> InputGeometriesPaths;
  DataPath DestinationGeometryPath;
  DataPath NewGeometryPath;
  bool CheckResolution;
  bool MirrorGeometry;
  bool SaveAsNewGeometry;
  CopyFromArray::Direction Direction;
};

/**
 * @class AppendImageGeometry
 * @brief This filter allows the user to append an Image Geometry onto the "end" of another Image Geometry.
 */
class SIMPLNXCORE_EXPORT AppendImageGeometry
{
public:
  AppendImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AppendImageGeometryInputValues* inputValues);
  ~AppendImageGeometry() noexcept;

  AppendImageGeometry(const AppendImageGeometry&) = delete;
  AppendImageGeometry(AppendImageGeometry&&) noexcept = delete;
  AppendImageGeometry& operator=(const AppendImageGeometry&) = delete;
  AppendImageGeometry& operator=(AppendImageGeometry&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AppendImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
