#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <EbsdLib/IO/TSL/AngReader.h>

namespace nx::core
{

/**
 * @brief Input values for the ReadAngData algorithm.
 */
struct ORIENTATIONANALYSIS_EXPORT ReadAngDataInputValues
{
  std::filesystem::path InputFile;             ///< Path to the .ang EBSD data file.
  DataPath DataContainerName;                  ///< Path to the output DataContainer (ImageGeom).
  std::string CellAttributeMatrixName;         ///< Name of the cell-level AttributeMatrix.
  std::string CellEnsembleAttributeMatrixName; ///< Name of the ensemble-level AttributeMatrix.
};

struct ORIENTATIONANALYSIS_EXPORT Ang_Private_Data
{
  std::array<size_t, 3> dims = {0, 0, 0};
  std::array<float, 3> resolution = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> origin = {0.0F, 0.0F, 0.0F};
  std::vector<ebsdlib::AngPhase::Pointer> phases;
  int32_t units = 0;
};

/**
 * @brief The ReadAngDataPrivate class is a private implementation of the ReadAngData class
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngDataPrivate
{
public:
  ReadAngDataPrivate() = default;
  ~ReadAngDataPrivate() = default;

  ReadAngDataPrivate(const ReadAngDataPrivate&) = delete;            // Copy Constructor Not Implemented
  ReadAngDataPrivate(ReadAngDataPrivate&&) = delete;                 // Move Constructor Not Implemented
  ReadAngDataPrivate& operator=(const ReadAngDataPrivate&) = delete; // Copy Assignment Not Implemented
  ReadAngDataPrivate& operator=(ReadAngDataPrivate&&) = delete;      // Move Assignment Not Implemented

  Ang_Private_Data m_Data;

  std::string m_InputFile_Cache;
  fs::file_time_type m_TimeStamp_Cache;
};

/**
 * @class ReadAngData
 * @brief Algorithm that reads a single .ang EBSD file into an Image Geometry.
 *
 * Parses the .ang file using EbsdLib's AngReader, then transfers the parsed data
 * into the DataStructure's cell-level and ensemble-level arrays.
 *
 * @section ooc_summary OOC Optimization Summary
 * All data transfer from the EbsdLib reader buffers into the DataStructure uses
 * copyFromBuffer() bulk writes instead of per-element operator[] access. Euler angles
 * (3 separate source arrays interleaved into 1 destination) use a chunked buffer approach
 * to bound memory while maintaining bulk I/O efficiency. See copyRawEbsdData() for details.
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngData
{
public:
  ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues);
  ~ReadAngData() noexcept;

  ReadAngData(const ReadAngData&) = delete;
  ReadAngData(ReadAngData&&) = delete;
  ReadAngData& operator=(const ReadAngData&) = delete;
  ReadAngData& operator=(ReadAngData&&) = delete;

  /**
   * @brief Executes the algorithm: reads the .ang file and populates the DataStructure.
   * @return Result<> indicating success or an EbsdLib error.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadAngDataInputValues* m_InputValues = nullptr;

  /**
   * @brief Loads phase/crystal structure information from the reader into ensemble-level arrays.
   * @param reader The EbsdLib AngReader that has already parsed the file.
   * @return Pair of (error code, error message). Error code 0 indicates success.
   */
  std::pair<int32, std::string> loadMaterialInfo(ebsdlib::AngReader* reader) const;

  /**
   * @brief Transfers raw EBSD data from reader buffers to DataStructure arrays using OOC-safe bulk I/O.
   * @param reader The EbsdLib AngReader that has already parsed the file.
   */
  void copyRawEbsdData(ebsdlib::AngReader* reader) const;
};

} // namespace nx::core
