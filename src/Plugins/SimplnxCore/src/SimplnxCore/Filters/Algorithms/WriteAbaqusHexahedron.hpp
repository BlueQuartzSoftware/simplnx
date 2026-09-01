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
 * @brief Stores output, job, material, geometry, Feature ID, and dummy-node selections.
 */
struct SIMPLNXCORE_EXPORT WriteAbaqusHexahedronInputValues
{
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
 * @brief Writes an ImageGeom as five related Abaqus input files.
 *
 * The files contain nodes, C3D8 elements, grain sections, grain element sets,
 * and a master include. Feature zero and negative Feature IDs do not enter grain
 * sets. Sections and empty sets span every positive ID through the maximum.
 *
 * The resident element-set path groups positive cells in one pass. Its memory is
 * O(maximum Feature ID plus positive cell count). The OOC path writes bounded
 * records to an external sorter, then streams grain-ordered element IDs. Actual
 * OOC input requires an external-sort provider. A forced OOC run on resident
 * data can use an exact bounded fallback that rescans all cells for each grain.
 *
 * Each final file uses its own AtomicFile. Commits occur sequentially, not as one
 * five-file transaction. A later commit failure can leave earlier final files
 * replaced. C stdio return values are not inspected, so a write failure can also
 * produce a committed partial file while this method reports success.
 */
class SIMPLNXCORE_EXPORT WriteAbaqusHexahedron
{
public:
  /**
   * @brief Initializes the Abaqus hexahedron writer.
   * @param dataStructure Contains the ImageGeom and Feature IDs.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Signals cancellation at file and chunk checkpoints.
   * @param inputValues Selects paths, names, material settings, and dummy node.
   * @pre inputValues is not null.
   * @pre All arguments outlive this writer.
   */
  WriteAbaqusHexahedron(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteAbaqusHexahedronInputValues* inputValues);
  /**
   * @brief Destroys the Abaqus hexahedron writer.
   */
  ~WriteAbaqusHexahedron() noexcept;

  WriteAbaqusHexahedron(const WriteAbaqusHexahedron&) = delete;
  WriteAbaqusHexahedron(WriteAbaqusHexahedron&&) noexcept = delete;
  WriteAbaqusHexahedron& operator=(const WriteAbaqusHexahedron&) = delete;
  WriteAbaqusHexahedron& operator=(WriteAbaqusHexahedron&&) noexcept = delete;

  /**
   * @brief Writes temporary files and commits each final file in sequence.
   * @return Temporary-file, source-I/O, external-sort, open, or commit result.
   * @pre FeatureIdsArrayPath is scalar Int32 and matches ImageGeometryPath cells.
   * @pre Dimension and node-count products fit usize and Abaqus integer output fields.
   *
   * Observed cancellation occurs before the commit loop. It returns success and
   * removes all temporary files. A later commit failure does not restore final
   * files committed earlier in that loop.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Forwards an information message to the filter callback.
   * @param message Message to send.
   */
  void sendMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const WriteAbaqusHexahedronInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
