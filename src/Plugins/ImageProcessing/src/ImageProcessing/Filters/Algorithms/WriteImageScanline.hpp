#pragma once

#include "ImageProcessing/Filters/Algorithms/WriteImage.hpp"

namespace nx::core
{
/**
 * @class WriteImageScanline
 * @brief Writes image slices from bounded, caller-owned source groups.
 */
class IMAGEPROCESSING_EXPORT WriteImageScanline
{
public:
  /**
   * @brief Creates a bounded image writer.
   * @param dataStructure Contains the source Image Geometry and Data Array.
   * @param messageHandler Accepted for dispatch compatibility. This writer does not send messages.
   * @param shouldCancel Stops output between slices when set.
   * @param inputValues Contains the output settings and source paths.
   */
  WriteImageScanline(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues);

  /** @brief Destroys the bounded image writer. */
  ~WriteImageScanline() noexcept;

  WriteImageScanline(const WriteImageScanline&) = delete;
  WriteImageScanline(WriteImageScanline&&) noexcept = delete;
  WriteImageScanline& operator=(const WriteImageScanline&) = delete;
  WriteImageScanline& operator=(WriteImageScanline&&) noexcept = delete;

  /**
   * @brief Writes all slices for the selected plane.
   * @return Error if source I/O, compression, or file output fails.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
};
} // namespace nx::core
