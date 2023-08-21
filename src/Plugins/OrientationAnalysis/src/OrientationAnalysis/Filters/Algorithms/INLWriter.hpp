#pragma once

#include "OrientationAnalysis/OrientationAnalysis_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

namespace complex
{

struct ORIENTATIONANALYSIS_EXPORT INLWriterInputValues
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

class ORIENTATIONANALYSIS_EXPORT INLWriter
{
public:
  INLWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, INLWriterInputValues* inputValues);
  ~INLWriter() noexcept;

  INLWriter(const INLWriter&) = delete;
  INLWriter(INLWriter&&) noexcept = delete;
  INLWriter& operator=(const INLWriter&) = delete;
  INLWriter& operator=(INLWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const INLWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
