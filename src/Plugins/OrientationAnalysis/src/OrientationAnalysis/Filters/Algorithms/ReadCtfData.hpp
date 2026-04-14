#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include <EbsdLib/IO/HKL/CtfPhase.h>
#include <EbsdLib/IO/HKL/CtfReader.h>

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ReadCtfDataInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
  bool DegreesToRadians;
  bool EdaxHexagonalAlignment;
};

struct ORIENTATIONANALYSIS_EXPORT Ctf_Private_Data
{
  std::array<size_t, 3> dims = {0, 0, 0};
  std::array<float, 3> resolution = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> origin = {0.0F, 0.0F, 0.0F};
  std::vector<ebsdlib::CtfPhase::Pointer> phases;
  int32_t units = 0;
};

/**
 * @brief The ReadCtfDataPrivate class is a private implementation of the ReadCtfData class
 */
class ORIENTATIONANALYSIS_EXPORT ReadCtfDataPrivate
{
public:
  ReadCtfDataPrivate() = default;
  ~ReadCtfDataPrivate() = default;

  ReadCtfDataPrivate(const ReadCtfDataPrivate&) = delete;            // Copy Constructor Not Implemented
  ReadCtfDataPrivate(ReadCtfDataPrivate&&) = delete;                 // Move Constructor Not Implemented
  ReadCtfDataPrivate& operator=(const ReadCtfDataPrivate&) = delete; // Copy Assignment Not Implemented
  ReadCtfDataPrivate& operator=(ReadCtfDataPrivate&&) = delete;      // Move Assignment Not Implemented

  Ctf_Private_Data m_Data;

  std::string m_InputFile_Cache;
  fs::file_time_type m_TimeStamp_Cache;
};

/**
 * @class ReadCtfData
 * @brief Algorithm that reads a single .ctf (Oxford/HKL) EBSD file into an Image Geometry.
 *
 * Parses the .ctf file using EbsdLib's CtfReader, then transfers the parsed data
 * into the DataStructure's cell-level and ensemble-level arrays.
 *
 * @section ooc_summary OOC Optimization Summary
 * All data transfer uses copyFromBuffer() bulk writes. Euler angles are interleaved from
 * 3 source arrays using chunked buffers with optional hex correction and degree-to-radian
 * conversion. Crystal structures are cached locally for the per-cell hex correction check.
 */
class ORIENTATIONANALYSIS_EXPORT ReadCtfData
{
public:
  ReadCtfData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadCtfDataInputValues* inputValues);
  ~ReadCtfData() noexcept;

  ReadCtfData(const ReadCtfData&) = delete;
  ReadCtfData(ReadCtfData&&) noexcept = delete;
  ReadCtfData& operator=(const ReadCtfData&) = delete;
  ReadCtfData& operator=(ReadCtfData&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ReadCtfDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief
   * @param reader
   * @return Error code.
   */
  std::pair<int32, std::string> loadMaterialInfo(ebsdlib::CtfReader* reader) const;

  /**
   * @brief
   * @param reader
   */
  void copyRawEbsdData(ebsdlib::CtfReader* reader) const;
};

} // namespace nx::core
