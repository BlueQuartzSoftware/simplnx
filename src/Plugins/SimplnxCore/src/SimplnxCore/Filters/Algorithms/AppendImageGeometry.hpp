#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/DataGroupSelectionParameter.hpp"
#include "simplnx/Utilities/DataArrayUtilities.hpp"

namespace nx::core
{

/**
 * @struct AppendImageGeometryInputValues
 * @brief Defines source geometries, append policy, defaults, and destination.
 */
struct SIMPLNXCORE_EXPORT AppendImageGeometryInputValues
{
  std::vector<DataPath> InputGeometriesPaths;
  DataPath DestinationGeometryPath;
  DataPath NewGeometryPath;
  std::string DefaultValue;
  bool CheckResolution;
  bool MirrorGeometry;
  bool SaveAsNewGeometry;
  CopyFromArray::Direction Direction;
};

/**
 * @class AppendImageGeometry
 * @brief Appends image geometries along one axis or creates a combined geometry.
 *
 * The algorithm processes one destination cell array per parallel task. Missing
 * source arrays use temporary arrays filled with DefaultValue. SaveAsNewGeometry
 * leaves the destination geometry unchanged. Otherwise, the destination geometry
 * is resized before array-copy tasks are scheduled.
 *
 * Cancellation returns success and can leave a partial destination. Already
 * scheduled tasks continue to completion. CopyFromArray does not return its
 * internal copy errors through this API.
 */
class SIMPLNXCORE_EXPORT AppendImageGeometry
{
public:
  /**
   * @brief Initializes image-geometry append or combine work.
   * @param dataStructure Provides source and destination objects.
   * @param mesgHandler Receives per-array status messages.
   * @param shouldCancel Signals cancellation between array tasks.
   * @param inputValues Defines geometry paths, direction, mirroring, and defaults.
   * @pre All arguments outlive this executor.
   */
  AppendImageGeometry(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AppendImageGeometryInputValues* inputValues);
  ~AppendImageGeometry() noexcept;

  AppendImageGeometry(const AppendImageGeometry&) = delete;
  AppendImageGeometry(AppendImageGeometry&&) noexcept = delete;
  AppendImageGeometry& operator=(const AppendImageGeometry&) = delete;
  AppendImageGeometry& operator=(AppendImageGeometry&&) noexcept = delete;

  /**
   * @brief Combines matching cell arrays and fills missing source arrays.
   * @return Default-array creation errors and warnings for missing arrays.
   * @pre Source dimensions are compatible in axes that are not appended.
   * @pre Matching arrays have compatible types and component shapes.
   * @pre Direction is X, Y, or Z and all referenced geometries exist.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const AppendImageGeometryInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
