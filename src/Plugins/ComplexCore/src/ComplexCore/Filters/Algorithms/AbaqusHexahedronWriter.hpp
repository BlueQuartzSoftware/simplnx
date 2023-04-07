#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/ArraySelectionParameter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/NumberParameter.hpp"
#include "complex/Parameters/StringParameter.hpp"

namespace complex
{

struct COMPLEXCORE_EXPORT AbaqusHexahedronWriterInputValues
{
  int32 HourglassStiffness;
  StringParameter::ValueType JobName;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType FilePrefix;
  DataPath FeatureIdsArrayPath;
  DataPath ImageGeometryPath;
};

/**
 * @class ConditionalSetValue
 * @brief This filter replaces values in the target array with a user specified value
 * where a bool mask array specifies.
 */

class COMPLEXCORE_EXPORT AbaqusHexahedronWriter
{
public:
  AbaqusHexahedronWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AbaqusHexahedronWriterInputValues* inputValues);
  ~AbaqusHexahedronWriter() noexcept;

  AbaqusHexahedronWriter(const AbaqusHexahedronWriter&) = delete;
  AbaqusHexahedronWriter(AbaqusHexahedronWriter&&) noexcept = delete;
  AbaqusHexahedronWriter& operator=(const AbaqusHexahedronWriter&) = delete;
  AbaqusHexahedronWriter& operator=(AbaqusHexahedronWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const AbaqusHexahedronWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
