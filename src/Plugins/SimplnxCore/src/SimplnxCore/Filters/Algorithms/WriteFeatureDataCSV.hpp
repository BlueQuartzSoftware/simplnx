#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/AttributeMatrixSelectionParameter.hpp"
#include "simplnx/Parameters/BoolParameter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteFeatureDataCSVInputValues
{
  AttributeMatrixSelectionParameter::ValueType CellFeatureAttributeMatrixPath;
  ChoicesParameter::ValueType DelimiterIndex;
  FileSystemPathParameter::ValueType FeatureDataFile;
  BoolParameter::ValueType WriteNeighborlistData;
  BoolParameter::ValueType WriteNumFeaturesLine;
};

/**
 * @class WriteFeatureDataCSV
 * @brief This algorithm implements support code for the WriteFeatureDataCSVFilter
 */

class SIMPLNXCORE_EXPORT WriteFeatureDataCSV
{
public:
  WriteFeatureDataCSV(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteFeatureDataCSVInputValues* inputValues);
  ~WriteFeatureDataCSV() noexcept;

  WriteFeatureDataCSV(const WriteFeatureDataCSV&) = delete;
  WriteFeatureDataCSV(WriteFeatureDataCSV&&) noexcept = delete;
  WriteFeatureDataCSV& operator=(const WriteFeatureDataCSV&) = delete;
  WriteFeatureDataCSV& operator=(WriteFeatureDataCSV&&) noexcept = delete;

  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteFeatureDataCSVInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
