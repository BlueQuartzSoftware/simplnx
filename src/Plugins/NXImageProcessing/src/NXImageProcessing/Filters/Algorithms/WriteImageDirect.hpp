#pragma once

#include "ImageProcessing/Filters/Algorithms/WriteImage.hpp"

namespace nx::core
{
/**
 * @class WriteImageDirect
 * @brief Writes image slices with the original direct extraction algorithm for in-memory source arrays.
 */
class IMAGEPROCESSING_EXPORT WriteImageDirect
{
public:
  /**
   * @brief Creates a direct image writer.
   * @param dataStructure Contains the source Image Geometry and Data Array.
   * @param messageHandler Receives slice progress messages.
   * @param shouldCancel Stops output between slices when set.
   * @param inputValues Contains the output settings and source paths.
   */
  WriteImageDirect(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const WriteImageInputValues& inputValues);

  /** @brief Destroys the direct image writer. */
  ~WriteImageDirect() noexcept;

  WriteImageDirect(const WriteImageDirect&) = delete;
  WriteImageDirect(WriteImageDirect&&) noexcept = delete;
  WriteImageDirect& operator=(const WriteImageDirect&) = delete;
  WriteImageDirect& operator=(WriteImageDirect&&) noexcept = delete;

  /**
   * @brief Writes all slices for the selected plane.
   * @return Error if slice extraction, compression, or file output fails.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const WriteImageInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
