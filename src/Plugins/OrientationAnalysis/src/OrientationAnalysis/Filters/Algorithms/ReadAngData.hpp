#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataArray.hpp"
#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/DataStructure/IDataArray.hpp"
#include "complex/Filter/IFilter.hpp"

#include "EbsdLib/IO/TSL/AngReader.h"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT ReadAngDataInputValues
{
  std::filesystem::path InputFile;
  DataPath DataContainerName;
  std::string CellAttributeMatrixName;
  std::string CellEnsembleAttributeMatrixName;
};

struct ORIENTATIONANALYSIS_EXPORT Ang_Private_Data
{
  std::array<size_t, 3> dims = {0, 0, 0};
  std::array<float, 3> resolution = {0.0F, 0.0F, 0.0F};
  std::array<float, 3> origin = {0.0F, 0.0F, 0.0F};
  std::vector<AngPhase::Pointer> phases;
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
 * @brief This filter will read a single .ang file into a new Image Geometry, allowing the immediate use of Filters on the data instead of having to generate the intermediate
 * .h5ebsd file.
 */
class ORIENTATIONANALYSIS_EXPORT ReadAngData
{
public:
  ReadAngData(DataStructure& dataStructure, const IFilter::MessageHandler& msgHandler, const std::atomic_bool& shouldCancel, ReadAngDataInputValues* inputValues);
  ~ReadAngData() noexcept;

  ReadAngData(const ReadAngData&) = delete;            // Copy Constructor Not Implemented
  ReadAngData(ReadAngData&&) = delete;                 // Move Constructor Not Implemented
  ReadAngData& operator=(const ReadAngData&) = delete; // Copy Assignment Not Implemented
  ReadAngData& operator=(ReadAngData&&) = delete;      // Move Assignment Not Implemented

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const ReadAngDataInputValues* m_InputValues = nullptr;

  /**
   * @brief
   * @param reader
   * @return Error code.
   */
  std::pair<int32, std::string> loadMaterialInfo(AngReader* reader) const;

  /**
   * @brief
   * @param reader
   */
  void copyRawEbsdData(AngReader* reader) const;
};

} // namespace complex
