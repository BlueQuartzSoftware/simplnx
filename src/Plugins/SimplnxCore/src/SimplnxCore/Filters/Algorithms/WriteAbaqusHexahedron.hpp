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

/**
 * @struct WriteAbaqusHexahedronInputValues
 * @brief Contains the values that control the Abaqus hexahedral mesh export.
 */
struct SIMPLNXCORE_EXPORT WriteAbaqusHexahedronInputValues
{
  bool UseReducedIntegration;
  int32 HourglassStiffness;
  StringParameter::ValueType JobName;
  FileSystemPathParameter::ValueType OutputPath;
  StringParameter::ValueType FilePrefix;
  DataPath FeatureIdsArrayPath;
  DataPath ImageGeometryPath;
  bool WriteDummyNode;
};

/**
 * @class WriteAbaqusHexahedron
 * @brief Writes the Abaqus mesh files for an Image Geometry.
 */
class SIMPLNXCORE_EXPORT WriteAbaqusHexahedron
{
public:
  /**
   * @brief Creates the Abaqus hexahedral mesh writer.
   * @param dataStructure Contains the input geometry and feature IDs.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Indicates when the caller requests cancellation.
   * @param inputValues Contains the export settings and input paths. The caller owns this object for the lifetime of the writer.
   */
  WriteAbaqusHexahedron(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteAbaqusHexahedronInputValues* inputValues);
  ~WriteAbaqusHexahedron() noexcept;

  WriteAbaqusHexahedron(const WriteAbaqusHexahedron&) = delete;
  WriteAbaqusHexahedron(WriteAbaqusHexahedron&&) noexcept = delete;
  WriteAbaqusHexahedron& operator=(const WriteAbaqusHexahedron&) = delete;
  WriteAbaqusHexahedron& operator=(WriteAbaqusHexahedron&&) noexcept = delete;

  /**
   * @brief Writes the Abaqus mesh files.
   * @return An error if a file cannot be created, written, or committed.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Sends an informational progress message to the filter caller.
   * @param message Contains the progress text.
   */
  void sendMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const WriteAbaqusHexahedronInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
