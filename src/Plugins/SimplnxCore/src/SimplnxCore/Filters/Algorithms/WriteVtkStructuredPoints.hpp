#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/MultiArraySelectionParameter.hpp"

namespace nx::core
{
class ImageGeom;
class IDataArray;

/**
 * @struct WriteVtkStructuredPointsInputValues
 * @brief Stores output format, geometry path, and selected arrays.
 */
struct SIMPLNXCORE_EXPORT WriteVtkStructuredPointsInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath ImageGeometryPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
};

/**
 * @class WriteVtkStructuredPoints
 * @brief Streams selected Image Geometry cell arrays to a legacy VTK structured-points file.
 *
 * In-memory arrays use a direct bounded writer. Out-of-core arrays use sequential
 * copyIntoBuffer() chunks. Binary byte swapping is confined to the chunk buffer,
 * so the source DataStore is never modified.
 */
class SIMPLNXCORE_EXPORT WriteVtkStructuredPoints
{
public:
  /**
   * @brief Creates a legacy VTK structured-points writer.
   * @param dataStructure Provides image metadata and selected arrays.
   * @param mesgHandler Receives per-array messages.
   * @param shouldCancel Stops value chunks when true.
   * @param inputValues Specifies validated output settings. The caller must keep
   * this object alive for the writer lifetime.
   */
  WriteVtkStructuredPoints(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteVtkStructuredPointsInputValues* inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteVtkStructuredPoints() noexcept;

  WriteVtkStructuredPoints(const WriteVtkStructuredPoints&) = delete;
  WriteVtkStructuredPoints(WriteVtkStructuredPoints&&) noexcept = delete;
  WriteVtkStructuredPoints& operator=(const WriteVtkStructuredPoints&) = delete;
  WriteVtkStructuredPoints& operator=(WriteVtkStructuredPoints&&) noexcept = delete;

  /**
   * @brief Writes the VTK header and all selected cell arrays.
   * @return Merged source-read and stream-write errors, or success after cancellation.
   *
   * The writer truncates the destination before processing. Cancellation or an
   * error can leave a partial file. Later arrays are attempted after an error.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Writes the configured VTK header to a C FILE stream.
   * @param outputFile Receives header text.
   * @warning This declaration has no definition in the current library.
   */
  void writeVtkHeader(FILE* outputFile) const;

  /**
   * @brief Writes regularly spaced coordinates for one rectilinear-grid axis.
   * @tparam T Specifies the coordinate scalar type.
   * @param outputFile Receives coordinates.
   * @param axis Specifies the VTK axis keyword.
   * @param type Specifies the VTK scalar token.
   * @param nPoints Specifies coordinate count.
   * @param min Specifies the first coordinate.
   * @param max Is unused by the declaration.
   * @param step Specifies coordinate increment.
   * @return Stream error or success.
   * @warning This declaration has no definition in the current library.
   */
  template <typename T>
  Result<> writeCoords(FILE* outputFile, const std::string& axis, const std::string& type, int64 nPoints, T min, T max, T step);

private:
  DataStructure& m_DataStructure;
  const WriteVtkStructuredPointsInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
