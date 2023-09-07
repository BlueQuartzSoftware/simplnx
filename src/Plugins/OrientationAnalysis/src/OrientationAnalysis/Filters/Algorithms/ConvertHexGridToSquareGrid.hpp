#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArrayCreationParameter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/BoolParameter.hpp"
#include "complex/Parameters/ChoicesParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/GeneratedFileListParameter.hpp"
#include "complex/Parameters/VectorParameter.hpp"

namespace complex
{
struct ORIENTATIONANALYSIS_EXPORT ConvertHexGridToSquareGridInputValues
{
  bool MultiFile;
  VectorFloat64Parameter::ValueType XYSpacing;
  FileSystemPathParameter::ValueType InputPath;
  FileSystemPathParameter::ValueType OutputPath;
  GeneratedFileListParameter::ValueType InputFileListInfo;
  std::string OutputFilePrefix;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT ConvertHexGridToSquareGrid
{
public:
  ConvertHexGridToSquareGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertHexGridToSquareGridInputValues* inputValues);
  ~ConvertHexGridToSquareGrid() noexcept;

  ConvertHexGridToSquareGrid(const ConvertHexGridToSquareGrid&) = delete;
  ConvertHexGridToSquareGrid(ConvertHexGridToSquareGrid&&) noexcept = delete;
  ConvertHexGridToSquareGrid& operator=(const ConvertHexGridToSquareGrid&) = delete;
  ConvertHexGridToSquareGrid& operator=(ConvertHexGridToSquareGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertHexGridToSquareGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
