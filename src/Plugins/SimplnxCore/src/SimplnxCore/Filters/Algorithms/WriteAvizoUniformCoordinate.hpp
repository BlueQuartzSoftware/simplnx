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
 * The writer checks source reads and each output operation. A failure stops
 * later reads and returns an error. Cancellation is checked between Feature ID
 * windows. The filter wrapper rejects
 * publication after cancellation.
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
   * @return Directory, source-read, output, or close status from AvizoWriter.
   */
  Result<> operator()();

protected:
  /**
   * @brief Writes the uniform-coordinate Avizo header.
   * @param outputFile Open binary-mode output stream.
   * @return Error if a header output operation fails.
   * @pre outputFile is not null.
   */
  Result<> generateHeader(FILE* outputFile) const override;

  /**
   * @brief Writes Feature IDs in binary or ASCII form.
   * @param outputFile Open binary-mode output stream.
   * @return Source warnings and any read or output error. Cancellation ends the current output.
   * @pre outputFile is not null.
   */
  Result<> writeData(FILE* outputFile) const override;
};

} // namespace nx::core
