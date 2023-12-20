#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFileInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  float32 Weight;
  int32 Sigma;
  ChoicesParameter::ValueType Delimiter;
  bool ConvertToDegrees;
  bool UseMask;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath MaskArrayPath;
};

/**
 * @class WriteStatsGenOdfAngleFile
 * @brief This filter will generate a synthetic microstructure with an ODF that matches (as closely as possible) an existing experimental data set or other data set that is being mimicked..
 */

class ORIENTATIONANALYSIS_EXPORT WriteStatsGenOdfAngleFile
{
public:
  WriteStatsGenOdfAngleFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteStatsGenOdfAngleFileInputValues* inputValues);
  ~WriteStatsGenOdfAngleFile() noexcept;

  WriteStatsGenOdfAngleFile(const WriteStatsGenOdfAngleFile&) = delete;
  WriteStatsGenOdfAngleFile(WriteStatsGenOdfAngleFile&&) noexcept = delete;
  WriteStatsGenOdfAngleFile& operator=(const WriteStatsGenOdfAngleFile&) = delete;
  WriteStatsGenOdfAngleFile& operator=(WriteStatsGenOdfAngleFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  int determineOutputLineCount(const Int32Array& cellPhases, const BoolArray* mask, usize totalPoints, int32 phase) const;
  Result<> writeOutputFile(std::ofstream& out, const Int32Array& cellPhases, const BoolArray* mask, int32 lineCount, usize totalPoints, int32 phase) const;

private:
  DataStructure& m_DataStructure;
  const WriteStatsGenOdfAngleFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
