#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Utilities/ImageIO/IImageIO.hpp"

namespace nx::core
{
/**
 * @class MhaImageIO
 * @brief Writes compressed, attached two-dimensional MetaImage files.
 *
 * The backend supports the numeric types and component counts accepted by
 * ITKImageWriterFilter. Read MHA/MetaImage File owns MHA input support.
 */
class SIMPLNX_EXPORT MhaImageIO final : public IImageIO
{
public:
  MhaImageIO() = default;
  ~MhaImageIO() noexcept override = default;

  MhaImageIO(const MhaImageIO&) = delete;
  MhaImageIO(MhaImageIO&&) noexcept = delete;
  MhaImageIO& operator=(const MhaImageIO&) = delete;
  MhaImageIO& operator=(MhaImageIO&&) noexcept = delete;

  /**
   * @brief Reports that the backend does not own MHA input.
   * @param filePath Identifies the requested MHA file.
   * @return An error that directs the user to Read MHA/MetaImage File.
   */
  Result<ImageMetadata> readMetadata(const std::filesystem::path& filePath) const override;

  /**
   * @brief Reports that the backend does not own MHA input.
   * @param filePath Identifies the requested MHA file.
   * @param buffer Is not modified.
   * @param pageIndex Identifies the requested page for the error message.
   * @return An error that directs the user to Read MHA/MetaImage File.
   */
  Result<> readPixelData(const std::filesystem::path& filePath, std::span<uint8> buffer, usize pageIndex = 0) const override;

  /**
   * @brief Reports that the backend does not own MHA input.
   * @param filePath Identifies the requested MHA file.
   * @param callback Is not called.
   * @param pageIndex Identifies the requested page for the error message.
   * @return An error that directs the user to Read MHA/MetaImage File.
   */
  Result<> readPixelDataRows(const std::filesystem::path& filePath, const ReadRowCallback& callback, usize pageIndex = 0) const override;

  /**
   * @brief Writes one compressed, attached MHA image.
   * @param filePath Identifies the output MHA file.
   * @param buffer Contains packed row-major pixel data.
   * @param metadata Specifies image dimensions, type, components, origin, and spacing.
   * @return An error if validation, compression, or file output fails.
   */
  Result<> writePixelData(const std::filesystem::path& filePath, std::span<const uint8> buffer, const ImageMetadata& metadata) const override;

  /**
   * @brief Returns the numeric types supported by MHA output.
   * @return Supported numeric types.
   */
  std::set<DataType> supportedWriteDataTypes() const override;

  /**
   * @brief Returns the component counts accepted by the ITK writer contract.
   * @return Supported component counts.
   */
  std::set<usize> supportedWriteComponentCounts() const override;
};
} // namespace nx::core
