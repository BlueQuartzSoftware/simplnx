#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

/**
 * @struct WriteSPParksSitesInputValues
 * @brief Stores output and source geometry paths.
 */
struct SIMPLNXCORE_EXPORT WriteSPParksSitesInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath ImageGeomPath;
};

/**
 * @class WriteSPParksSites
 * @brief Writes ImageGeom Feature IDs in SPPARKS sites format.
 *
 * Feature IDs use approximately 1 MiB source pages. Site records are formatted
 * individually into a directly opened destination stream.
 */
class SIMPLNXCORE_EXPORT WriteSPParksSites
{
public:
  /**
   * @brief Creates an SPPARKS sites writer.
   * @param dataStructure Provides image metadata and Feature IDs.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops before later site records when true.
   * @param inputValues Specifies validated output and source paths. The caller
   * must keep this object alive for the writer lifetime.
   */
  WriteSPParksSites(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteSPParksSitesInputValues* inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteSPParksSites() noexcept;

  WriteSPParksSites(const WriteSPParksSites&) = delete;
  WriteSPParksSites(WriteSPParksSites&&) noexcept = delete;
  WriteSPParksSites& operator=(const WriteSPParksSites&) = delete;
  WriteSPParksSites& operator=(WriteSPParksSites&&) noexcept = delete;

  /**
   * @brief Writes the SPPARKS header and all site records.
   * @return Directory, file-open, or Feature-ID read error, or success after cancellation.
   *
   * Stream write, flush, and close status are not inspected. Cancellation or
   * failure can leave a partial destination file.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteSPParksSitesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
