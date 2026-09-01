#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <EbsdLib/IO/HKL/CprReader.h>

#include <filesystem>
namespace fs = std::filesystem;

namespace nx::core
{

/**
 * @struct ReadChannel5DataInputValues
 * @brief Identifies Channel 5 reader inputs.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadChannel5DataInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
  bool EdaxHexagonalAlignment;
  bool CreateCompatibleArrays;
};

/**
 * @struct Ang_Private_Data
 * @brief Stores parsed Channel 5 metadata.
 */
struct ORIENTATIONANALYSIS_EXPORT Ang_Private_Data
{
  std::array<size_t, 3> dims = {0, 0, 0};
  std::array<float, 3> resolution = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> origin = {0.0F, 0.0F, 0.0F};
  std::vector<ebsdlib::CtfPhase::Pointer> phases;
  int32_t units = 0;
};

/**
 * @class ReadChannel5DataPrivate
 * @brief Caches Channel 5 reader metadata.
 */
class ORIENTATIONANALYSIS_EXPORT ReadChannel5DataPrivate
{
public:
  ReadChannel5DataPrivate() = default;
  ~ReadChannel5DataPrivate() = default;

  ReadChannel5DataPrivate(const ReadChannel5DataPrivate&) = delete;
  ReadChannel5DataPrivate(ReadChannel5DataPrivate&&) = delete;
  ReadChannel5DataPrivate& operator=(const ReadChannel5DataPrivate&) = delete;
  ReadChannel5DataPrivate& operator=(ReadChannel5DataPrivate&&) = delete;

  Ang_Private_Data m_Data;

  std::string m_InputFile_Cache;
  fs::file_time_type m_TimeStamp_Cache;
};

/**
 * @class ReadChannel5Data
 * @brief Reads an Oxford Channel 5 .cpr/.crc pair into an Image Geometry.
 *
 * EbsdLib owns full reader buffers. Simplnx transfers fields in bounded chunks
 * and does not add a second cell-sized staging buffer. Destination bulk-write
 * Result values are not inspected.
 */
class ORIENTATIONANALYSIS_EXPORT ReadChannel5Data
{
public:
  /**
   * @brief Initializes Channel 5 file reading.
   * @param dataStructure Provides output arrays.
   * @param msgHandler Supplies progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Identifies input and output paths.
   * @pre dataStructure, msgHandler, shouldCancel, and inputValues outlive this
   *      executor.
   */
  ReadChannel5Data(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadChannel5DataInputValues* inputValues);
  /**
   * @brief Destroys the Channel 5 reader.
   */
  ~ReadChannel5Data() noexcept;

  ReadChannel5Data(const ReadChannel5Data&) = delete;
  ReadChannel5Data(ReadChannel5Data&&) = delete;
  ReadChannel5Data& operator=(const ReadChannel5Data&) = delete;
  ReadChannel5Data& operator=(ReadChannel5Data&&) = delete;

  /**
   * @brief Reads Channel 5 data into output arrays.
   * @return Success, or an EbsdLib reader error.
   *
   * When a cancellation checkpoint observes the signal, the function returns success. Data copied before that checkpoint remains in the output arrays. Later data is not copied.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadChannel5DataInputValues* m_InputValues = nullptr;

  /**
   * @brief Loads ensemble data from the parsed reader.
   * @param reader Provides parsed Channel 5 data.
   * @return Reader error code and message.
   */
  std::pair<int32, std::string> loadMaterialInfo(ebsdlib::CprReader* reader) const;

  /**
   * @brief Transfers parsed cell data through bounded bulk I/O.
   * @param reader Provides parsed Channel 5 data.
   */
  void copyRawEbsdData(ebsdlib::CprReader* reader) const;
};

} // namespace nx::core
