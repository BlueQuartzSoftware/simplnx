#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "simplnx/Common/Aliases.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ChoicesParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"

namespace nx::core
{
/**
 * @brief Identifies an unsupported requested numeric type.
 */
inline constexpr int32 k_UnsupportedScalarType = -1070;

/**
 * @struct ReadRawBinaryInputValues
 * @brief Stores file layout, numeric type, byte order, and destination path.
 */
struct SIMPLNXCORE_EXPORT ReadRawBinaryInputValues
{
  FileSystemPathParameter::ValueType inputFileValue;
  NumericType scalarTypeValue;
  ShapeType componentDimsValue;
  ChoicesParameter::ValueType endianValue;
  uint64 skipHeaderBytesValue;
  DataPath createdAttributeArrayPathValue;
};

/**
 * @class ReadRawBinary
 * @brief Imports a typed raw binary payload through bounded pages.
 *
 * Optional byte swapping changes only local page buffers. Trailing file bytes
 * are permitted. The destination component count must match componentDimsValue.
 */
class SIMPLNXCORE_EXPORT ReadRawBinary
{
public:
  /**
   * @brief Creates a raw binary reader.
   * @param dataStructure Receives imported values.
   * @param inputValues Specifies validated file layout and destination settings.
   * @param shouldCancel Stops before the import starts when true.
   * @param mesgHandler Is retained but not used.
   */
  ReadRawBinary(DataStructure& dataStructure, const ReadRawBinaryInputValues& inputValues, const std::atomic_bool& shouldCancel, const IFilter::MessageHandler& mesgHandler);
  /**
   * @brief Destroys the non-owning reader.
   */
  ~ReadRawBinary() noexcept;

  ReadRawBinary(const ReadRawBinary&) = delete;
  ReadRawBinary(ReadRawBinary&&) noexcept = delete;
  ReadRawBinary& operator=(const ReadRawBinary&) = delete;
  ReadRawBinary& operator=(ReadRawBinary&&) noexcept = delete;

  /**
   * @brief Validates destination shape and imports the payload.
   * @return Shape, size, type, file, or destination-write error, or success after cancellation.
   *
   * Cancellation is not checked after paged import starts.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadRawBinaryInputValues& m_InputValues;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

  /**
   * @brief Dispatches the configured scalar type to paged import.
   * @return Shape, size, type, file, or destination-write error, or success.
   */
  Result<> execute();
};
} // namespace nx::core
