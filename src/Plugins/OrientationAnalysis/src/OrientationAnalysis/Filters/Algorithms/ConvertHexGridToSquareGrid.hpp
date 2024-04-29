#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArrayCreationParameter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/GeneratedFileListParameter.hpp"
#include "simplnx/Parameters/VectorParameter.hpp"

namespace nx::core
{
struct ORIENTATIONANALYSIS_EXPORT ConvertHexGridToSquareGridInputValues
{
  bool MultiFile;
  VectorFloat32Parameter::ValueType XYSpacing;
  FileSystemPathParameter::ValueType InputPath;
  FileSystemPathParameter::ValueType OutputPath;
  GeneratedFileListParameter::ValueType InputFileListInfo;
  std::string OutputFilePrefix;
};

/**
 * @class
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

} // namespace nx::core
