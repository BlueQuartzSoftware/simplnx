#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace nx::core
{

struct SIMPLNXCORE_EXPORT WriteAbaqusHexahedronInputValues
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

class SIMPLNXCORE_EXPORT WriteAbaqusHexahedron
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

} // namespace nx::core
