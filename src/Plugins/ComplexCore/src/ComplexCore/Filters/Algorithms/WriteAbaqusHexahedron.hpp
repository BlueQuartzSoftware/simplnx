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

struct COMPLEXCORE_EXPORT WriteAbaqusHexahedronInputValues
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

class COMPLEXCORE_EXPORT WriteAbaqusHexahedron
{
public:
  WriteAbaqusHexahedron(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteAbaqusHexahedronInputValues* inputValues);
  ~WriteAbaqusHexahedron() noexcept;

  WriteAbaqusHexahedron(const WriteAbaqusHexahedron&) = delete;
  WriteAbaqusHexahedron(WriteAbaqusHexahedron&&) noexcept = delete;
  WriteAbaqusHexahedron& operator=(const WriteAbaqusHexahedron&) = delete;
  WriteAbaqusHexahedron& operator=(WriteAbaqusHexahedron&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const WriteAbaqusHexahedronInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
