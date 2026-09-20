#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/AvizoWriter.hpp"

namespace nx::core
{
/**
 * @class WriteAvizoUniformCoordinate
 * @brief Writes Avizo Feature IDs with uniform-grid coordinate metadata.
 *
 * Feature IDs use 65,536-value source buffers. The header derives its bounding
 * box from origin plus spacing times dimensions. Binary output uses native
 * endianness and identifies it in the header. ASCII output inserts a newline
 * after 21 Feature IDs.
 *
 * DataStore read results and C stdio return values are not inspected. A source
 * or file-write failure can produce stale or partial output while returning
 * success. Cancellation is checked between Feature ID chunks and closes the
 * partial file with a success result.
 */
class SIMPLNXCORE_EXPORT WriteAvizoUniformCoordinate : public AvizoWriter
{
public:
  /**
   * @brief Initializes the uniform-coordinate Avizo writer.
   * @param dataStructure Contains the ImageGeom and Feature IDs.
   * @param mesgHandler Preserves the common writer constructor signature.
   * @param shouldCancel Signals cancellation between Feature ID chunks.
   * @param inputValues Selects path, encoding, geometry, Feature IDs, and units.
   * @pre All arguments outlive this writer.
   */
  WriteAvizoUniformCoordinate(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);
  /**
   * @brief Destroys the uniform-coordinate Avizo writer.
   */
  ~WriteAvizoUniformCoordinate() noexcept override;

  WriteAvizoUniformCoordinate(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate(WriteAvizoUniformCoordinate&&) noexcept = delete;
  WriteAvizoUniformCoordinate& operator=(const WriteAvizoUniformCoordinate&) = delete;
  WriteAvizoUniformCoordinate& operator=(WriteAvizoUniformCoordinate&&) noexcept = delete;

  /**
   * @brief Creates the output path and writes the Avizo file.
   * @return Directory or file-open result from AvizoWriter.
   *
   * Cancellation and data-write failures are not distinguishable from success.
   */
  Result<> operator()();

protected:
  /**
   * @brief Writes the uniform-coordinate Avizo header.
   * @param outputFile Open binary-mode output stream.
   * @return Success. C stdio failures are not inspected.
   * @pre outputFile is not null.
   */
  Result<> generateHeader(FILE* outputFile) const override;

  /**
   * @brief Writes Feature IDs in binary or ASCII form.
   * @param outputFile Open binary-mode output stream.
   * @return Success after completion or cancellation.
   * @pre outputFile is not null.
   */
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
