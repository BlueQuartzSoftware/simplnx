#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

namespace complex
{
class ImageGeom;
class IDataArray;

struct COMPLEXCORE_EXPORT WriteVtkRectilinearGridInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath ImageGeometryPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
};

/**
 * @class VtkRectilinearGridWriter
 * @brief This filter ...
 */

class COMPLEXCORE_EXPORT WriteVtkRectilinearGrid
{
public:
  WriteVtkRectilinearGrid(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, WriteVtkRectilinearGridInputValues* inputValues);
  ~WriteVtkRectilinearGrid() noexcept;

  WriteVtkRectilinearGrid(const WriteVtkRectilinearGrid&) = delete;
  WriteVtkRectilinearGrid(WriteVtkRectilinearGrid&&) noexcept = delete;
  WriteVtkRectilinearGrid& operator=(const WriteVtkRectilinearGrid&) = delete;
  WriteVtkRectilinearGrid& operator=(WriteVtkRectilinearGrid&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void writeVtkHeader(FILE* outputFile) const;

  /**
   * @brief This function writes a set of Axis coordinates to that are needed
   * for a Rectilinear Grid based data set.
   * @param outputFile The "C" FILE* pointer to the file being written to.
   * @param axis The name of the Axis that is being written
   * @param type The type of primitive being written (float, int, ...)
   * @param nPoints The total number of points in the array
   * @param min The minimum value of the axis
   * @param max The maximum value of the axis
   * @param step The step value between each point on the axis.
   * @param binary Whether or not to write the vtk file data in binary
   */
  template <typename T>
  Result<> writeCoords(FILE* outputFile, const std::string& axis, const std::string& type, int64 nPoints, T min, T max, T step);

private:
  DataStructure& m_DataStructure;
  const WriteVtkRectilinearGridInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
