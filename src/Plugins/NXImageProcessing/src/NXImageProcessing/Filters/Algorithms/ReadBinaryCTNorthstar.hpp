#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Common/Array.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace fs = std::filesystem;

namespace nx::core
{

/**
 * @struct ReadBinaryCTNorthstarInputValues
 * @brief Stores header-derived file layout, crop bounds, units, and output paths.
 */
struct NXIMAGEPROCESSING_EXPORT ReadBinaryCTNorthstarInputValues
{
  FileSystemPathParameter::ValueType input_header_file;
  DataPath image_geometry_path;
  DataPath density_array_path;
  std::vector<std::pair<fs::path, usize>> data_file_paths;
  SizeVec3Type original_geometry_dims;
  SizeVec3Type imported_geometry_dims;
  bool import_subvolume{};
  IntVec3Type start_voxel_coord;
  IntVec3Type end_voxel_coord;
  ChoicesParameter::ValueType length_unit{};
};

/**
 * @class ReadBinaryCTNorthstar
 * @brief Imports North Star Imaging binary CT density files.
 *
 * The reader initializes the complete destination with a sentinel. It then seeks
 * to selected source rows and writes each imported row through checked bulk I/O.
 */
class NXIMAGEPROCESSING_EXPORT ReadBinaryCTNorthstar
{
public:
  /**
   * @brief Creates a North Star binary CT reader.
   * @param dataStructure Receives density values and geometry units.
   * @param messageHandler Receives slice progress.
   * @param shouldCancel Stops before later source slices when true.
   * @param inputValues Specifies validated file layout and crop settings. The caller
   * must keep this object alive for the reader lifetime.
   */
  ReadBinaryCTNorthstar(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, ReadBinaryCTNorthstarInputValues* inputValues);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadBinaryCTNorthstar() noexcept = default;

  ReadBinaryCTNorthstar(const ReadBinaryCTNorthstar&) = delete;
  ReadBinaryCTNorthstar(ReadBinaryCTNorthstar&&) noexcept = delete;
  ReadBinaryCTNorthstar& operator=(const ReadBinaryCTNorthstar&) = delete;
  ReadBinaryCTNorthstar& operator=(ReadBinaryCTNorthstar&&) noexcept = delete;

  /**
   * @brief Initializes and imports all selected density rows.
   * @return File, seek, read, or destination-write error, or success after cancellation.
   *
   * Cancellation can retain sentinel values and rows imported before the stop.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadBinaryCTNorthstarInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
