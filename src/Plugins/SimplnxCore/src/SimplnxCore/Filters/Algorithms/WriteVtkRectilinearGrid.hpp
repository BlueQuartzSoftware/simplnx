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
 * @struct WriteVtkRectilinearGridInputValues
 * @brief Stores output format, geometry path, and selected arrays.
 */
struct SIMPLNXCORE_EXPORT WriteVtkRectilinearGridInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath ImageGeometryPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
};

/**
 * @class WriteVtkRectilinearGrid
 * @brief Writes ImageGeom data as a legacy VTK RECTILINEAR_GRID file.
 *
 * Coordinate arrays are generated from image origin and spacing. Selected data
 * arrays use bounded typed pages through WriteVtkDataArrayFunctor.
 */
class SIMPLNXCORE_EXPORT WriteVtkRectilinearGrid
{
public:
  /**
   * @brief Creates a legacy VTK rectilinear-grid writer.
   * @param dataStructure Provides image metadata and selected arrays.
   * @param mesgHandler Receives per-array messages.
   * @param shouldCancel Is retained but not inspected.
   * @param inputValues Specifies validated output settings. The caller must keep
   * this object alive for the writer lifetime.
   */
  WriteVtkRectilinearGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteVtkRectilinearGridInputValues* inputValues);
  /**
   * @brief Destroys the non-owning writer.
   */
  ~WriteVtkRectilinearGrid() noexcept;

  WriteVtkRectilinearGrid(const WriteVtkRectilinearGrid&) = delete;
  WriteVtkRectilinearGrid(WriteVtkRectilinearGrid&&) noexcept = delete;
  WriteVtkRectilinearGrid& operator=(const WriteVtkRectilinearGrid&) = delete;
  WriteVtkRectilinearGrid& operator=(WriteVtkRectilinearGrid&&) noexcept = delete;

  /**
   * @brief Writes the header, coordinates, and selected cell arrays.
   * @return File-open, coordinate-write, source-read, or binary-write error.
   *
   * The writer replaces the destination directly and does not inspect cancellation.
   * Header and ASCII FILE write status is not reported.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

  /**
   * @brief Writes the configured VTK rectilinear-grid header.
   * @param outputFile Receives header text.
   *
   * The method does not report FILE write status.
   */
  void writeVtkHeader(FILE* outputFile) const;

  /**
   * @brief Writes regularly spaced coordinates for one axis.
   * @tparam T Specifies the coordinate scalar type.
   * @param outputFile Receives coordinates.
   * @param axis Specifies the VTK axis keyword.
   * @param type Specifies the VTK scalar token.
   * @param nPoints Specifies coordinate count.
   * @param min Specifies the first coordinate.
   * @param step Specifies coordinate increment.
   * @return Binary-write error, or success.
   *
   * ASCII FILE write status is not reported.
   */
  template <typename T>
  Result<> writeCoords(FILE* outputFile, const std::string& axis, const std::string& type, int64 nPoints, T min, T step);

private:
  DataStructure& m_DataStructure;
  const WriteVtkRectilinearGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
