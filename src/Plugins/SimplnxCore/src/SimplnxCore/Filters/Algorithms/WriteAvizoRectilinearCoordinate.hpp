#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/AvizoWriter.hpp"

namespace nx::core
{
/**
 * @class WriteAvizoRectilinearCoordinate
 * @brief Writes Avizo Feature IDs and explicit rectilinear axis coordinates.
 *
 * Feature IDs use 65,536-value source buffers. Coordinates use one vector whose
 * size is the current axis dimension. Binary output uses native endianness and
 * identifies it in the header. ASCII Feature IDs insert a newline after 21 values.
 *
 * DataStore read results and C stdio return values are not inspected. A source
 * or file-write failure can produce stale or partial output while returning
 * success. Cancellation is checked between Feature ID chunks. It returns success
 * and closes a file that does not include the remaining data or coordinates.
 */
class SIMPLNXCORE_EXPORT WriteAvizoRectilinearCoordinate : public AvizoWriter
{
public:
  /**
   * @brief Initializes the rectilinear Avizo writer.
   * @param dataStructure Contains the ImageGeom and Feature IDs.
   * @param mesgHandler Preserves the common writer constructor signature.
   * @param shouldCancel Signals cancellation between Feature ID chunks.
   * @param inputValues Selects path, encoding, geometry, Feature IDs, and units.
   * @pre All arguments outlive this writer.
   */
  WriteAvizoRectilinearCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  /**
   * @brief Destroys the rectilinear Avizo writer.
   */
  ~WriteAvizoRectilinearCoordinate() noexcept override;

  WriteAvizoRectilinearCoordinate(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate(WriteAvizoRectilinearCoordinate&&) noexcept = delete;
  WriteAvizoRectilinearCoordinate& operator=(const WriteAvizoRectilinearCoordinate&) = delete;
  WriteAvizoRectilinearCoordinate& operator=(WriteAvizoRectilinearCoordinate&&) noexcept = delete;

  /**
   * @brief Creates the output path and writes the Avizo file.
   * @return Directory or file-open result from AvizoWriter.
   *
   * Cancellation and data-write failures are not distinguishable from success.
   */
  Result<> operator()();

protected:
  /**
   * @brief Writes the rectilinear Avizo header.
   * @param outputFile Open binary-mode output stream.
   * @return Success. C stdio failures are not inspected.
   * @pre outputFile is not null.
   */
  Result<> generateHeader(FILE* outputFile) const override;

  /**
   * @brief Writes Feature IDs and X, Y, then Z coordinate arrays.
   * @param outputFile Open binary-mode output stream.
   * @return Success after completion or cancellation.
   * @pre outputFile is not null.
   */
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
