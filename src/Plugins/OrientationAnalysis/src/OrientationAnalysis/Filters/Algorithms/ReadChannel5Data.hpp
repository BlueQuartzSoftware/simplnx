#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/DataStructure/IDataArray.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

#include "EbsdLib/IO/HKL/CprReader.h"

#include <filesystem>
namespace fs = std::filesystem;

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT ReadChannel5DataInputValues
{
  FileSystemPathParameter::ValueType InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
  bool EdaxHexagonalAlignment;
  bool CreateCompatibleArrays;
};

struct ORIENTATIONANALYSIS_EXPORT Ang_Private_Data
{
  std::array<size_t, 3> dims = {0, 0, 0};
  std::array<float, 3> resolution = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> origin = {0.0F, 0.0F, 0.0F};
  std::vector<CtfPhase::Pointer> phases;
  int32_t units = 0;
};

/**
 * @brief The ReadChannel5DataPrivate class is a private implementation of the ReadChannel5Data class
 */
class ORIENTATIONANALYSIS_EXPORT ReadChannel5DataPrivate
{
public:
  ReadChannel5DataPrivate() = default;
  ~ReadChannel5DataPrivate() = default;

  ReadChannel5DataPrivate(const ReadChannel5DataPrivate&) = delete;            // Copy Constructor Not Implemented
  ReadChannel5DataPrivate(ReadChannel5DataPrivate&&) = delete;                 // Move Constructor Not Implemented
  ReadChannel5DataPrivate& operator=(const ReadChannel5DataPrivate&) = delete; // Copy Assignment Not Implemented
  ReadChannel5DataPrivate& operator=(ReadChannel5DataPrivate&&) = delete;      // Move Assignment Not Implemented

  Ang_Private_Data m_Data;

  std::string m_InputFile_Cache;
  fs::file_time_type m_TimeStamp_Cache;
};

/**
 * @class ReadChannel5Data
 * @brief This filter will read a single .ang file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the intermediate
 * .h5ebsd file.
 */
class ORIENTATIONANALYSIS_EXPORT ReadChannel5Data
{
public:
  ReadChannel5Data(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadChannel5DataInputValues* inputValues);
  ~ReadChannel5Data() noexcept;

  ReadChannel5Data(const ReadChannel5Data&) = delete;            // Copy Constructor Not Implemented
  ReadChannel5Data(ReadChannel5Data&&) = delete;                 // Move Constructor Not Implemented
  ReadChannel5Data& operator=(const ReadChannel5Data&) = delete; // Copy Assignment Not Implemented
  ReadChannel5Data& operator=(ReadChannel5Data&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadChannel5DataInputValues* m_InputValues = nullptr;

  /**
   * @brief
   * @param reader
   * @return Error code.
   */
  std::pair<int32, std::string> loadMaterialInfo(CprReader* reader) const;

  /**
   * @brief
   * @param reader
   */
  void copyRawEbsdData(CprReader* reader) const;
};

} // namespace nx::core
