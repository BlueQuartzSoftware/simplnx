#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT LosAlamosFFTWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  DataPath FeatureIdsArrayPath;
  DataPath CellEulerAnglesArrayPath;
  DataPath CellPhasesArrayPath;
  DataPath ImageGeomPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT LosAlamosFFTWriter
{
public:
  LosAlamosFFTWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, LosAlamosFFTWriterInputValues* inputValues);
  ~LosAlamosFFTWriter() noexcept;

  LosAlamosFFTWriter(const LosAlamosFFTWriter&) = delete;
  LosAlamosFFTWriter(LosAlamosFFTWriter&&) noexcept = delete;
  LosAlamosFFTWriter& operator=(const LosAlamosFFTWriter&) = delete;
  LosAlamosFFTWriter& operator=(LosAlamosFFTWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const LosAlamosFFTWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
