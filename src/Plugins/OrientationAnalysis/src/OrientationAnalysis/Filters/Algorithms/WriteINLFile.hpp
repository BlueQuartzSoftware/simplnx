#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{

struct ORIENTATIONANALYSIS_EXPORT WriteINLFileInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath ImageGeomPath;
  DataPath FeatureIdsArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CrystalStructuresArrayPath;
  DataPath MaterialNameArrayPath;
  DataPath NumFeaturesArrayPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class ORIENTATIONANALYSIS_EXPORT WriteINLFile
{
public:
  WriteINLFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteINLFileInputValues* inputValues);
  ~WriteINLFile() noexcept;

  WriteINLFile(const WriteINLFile&) = delete;
  WriteINLFile(WriteINLFile&&) noexcept = delete;
  WriteINLFile& operator=(const WriteINLFile&) = delete;
  WriteINLFile& operator=(WriteINLFile&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const WriteINLFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
