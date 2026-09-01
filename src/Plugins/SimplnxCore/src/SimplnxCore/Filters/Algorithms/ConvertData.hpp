#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"

namespace nx::core
{

/**
 * @struct ConvertDataInputValues
 * @brief Stores the selected source, target type, and output path.
 *
 * RescaleValues is currently not read by this algorithm.
 */
struct SIMPLNXCORE_EXPORT ConvertDataInputValues
{
  DataType ScalarType;
  DataPath SelectedArrayPath;
  DataPath OutputArrayName;
  bool RescaleValues = false;
};

/**
 * @class ConvertData
 * @brief Converts a selected array to a new scalar type while preserving its shape.
 *
 * Uses direct parallel access for in-memory arrays and bounded bulk transfers for
 * disk-backed arrays. Conversion uses C++ casts; RescaleValues is not applied.
 */
class SIMPLNXCORE_EXPORT ConvertData
{
public:
  /**
   * @brief Creates a data-conversion dispatcher.
   * @param dataStructure Provides the selected arrays.
   * @param mesgHandler Receives progress messages.
   * @param shouldCancel Stops later ranges or chunks when true.
   * @param inputValues Specifies validated paths and options. The caller must
   * keep this object alive for the dispatcher lifetime.
   */
  ConvertData(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ConvertDataInputValues* inputValues);
  /**
   * @brief Destroys the non-owning conversion dispatcher.
   */
  ~ConvertData() noexcept;

  ConvertData(const ConvertData&) = delete;
  ConvertData(ConvertData&&) noexcept = delete;
  ConvertData& operator=(const ConvertData&) = delete;
  ConvertData& operator=(ConvertData&&) noexcept = delete;

  /**
   * @brief Converts values to the selected scalar type.
   * @return Error for an unsupported target type, or success after cancellation.
   *
   * Cancellation can retain output values from completed ranges or chunks.
   */
  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const ConvertDataInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
