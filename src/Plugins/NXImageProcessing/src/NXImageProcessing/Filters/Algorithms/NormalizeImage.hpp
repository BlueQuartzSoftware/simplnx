#pragma once

#include "NXImageProcessing/NXImageProcessing_export.hpp"

#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{
/**
 * @struct NormalizeImageInputValues
 * @brief Selects the scalar image input and its float64 output.
 */
struct NXIMAGEPROCESSING_EXPORT NormalizeImageInputValues
{
  DataPath InputArrayPath;
  DataPath OutputArrayPath;
};

/**
 * @class NormalizeImage
 * @brief Normalizes scalar values with the global mean and sample standard deviation.
 */
class NXIMAGEPROCESSING_EXPORT NormalizeImage
{
public:
  /**
   * @brief Initializes normalization without taking ownership of its inputs.
   * @param dataStructure Contains the input and output arrays.
   * @param messageHandler Receives progress messages.
   * @param shouldCancel Signals cancellation.
   * @param inputValues Selects the arrays and outlives this executor.
   * @pre All referenced arguments outlive this executor.
   */
  NormalizeImage(DataStructure& dataStructure, const IFilter::MessageHandler& messageHandler, const std::atomic_bool& shouldCancel, const NormalizeImageInputValues* inputValues);
  ~NormalizeImage() noexcept;
  NormalizeImage(const NormalizeImage&) = delete;
  NormalizeImage(NormalizeImage&&) noexcept = delete;
  NormalizeImage& operator=(const NormalizeImage&) = delete;
  NormalizeImage& operator=(NormalizeImage&&) noexcept = delete;

  /**
   * @brief Computes statistics and writes normalized float64 values.
   * @return An array transfer error, or success.
   * @pre The input is a numeric DataArray with component shape {1}.
   * @pre The output array exists and matches the input tuple shape.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const IFilter::MessageHandler& m_MessageHandler;
  const std::atomic_bool& m_ShouldCancel;
  const NormalizeImageInputValues* m_InputValues;
};
} // namespace nx::core
