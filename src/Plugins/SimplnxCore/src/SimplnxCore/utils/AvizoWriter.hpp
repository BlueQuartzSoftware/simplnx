#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

#include <cstdarg>
#include <cstdio>

namespace nx::core
{

struct SIMPLNXCORE_EXPORT AvizoWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath GeometryPath;
  DataPath FeatureIdsArrayPath;
  StringParameter::ValueType Units;
};

/**
 * @class AvizoWriter
 * @brief Writes an Avizo data file with checked output and stream closure.
 */
class SIMPLNXCORE_EXPORT AvizoWriter
{
public:
  /**
   * @brief Stores the data and options used by the writer.
   * @param dataStructure Contains the geometry and Feature IDs.
   * @param mesgHandler Receives filter messages.
   * @param shouldCancel Signals cancellation between Feature ID windows.
   * @param inputValues Specifies the output path, encoding, and input paths.
   * @pre All referenced arguments outlive this writer.
   */
  AvizoWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues);

  /**
   * @brief Destroys the writer without closing an active stream.
   *
   * execute() owns and closes its stream before this destructor runs.
   */
  virtual ~AvizoWriter() noexcept;

  AvizoWriter(const AvizoWriter&) = delete;
  AvizoWriter(AvizoWriter&&) noexcept = delete;
  AvizoWriter& operator=(const AvizoWriter&) = delete;
  AvizoWriter& operator=(AvizoWriter&&) noexcept = delete;

  /**
   * @brief Writes the file and closes its stream before returning.
   * @return Directory, output, or close errors with earlier warnings retained.
   * @throws std::bad_alloc if a stage or callback cannot allocate memory.
   */
  Result<> execute();

  const std::atomic_bool& getCancel();

protected:
  /**
   * @struct OutputOperations
   * @brief Supplies optional output callbacks for one writer.
   *
   * The writer does not own this bundle. Each null operation uses C stdio.
   * A close callback consumes its FILE even when it reports an error or throws.
   */
  struct OutputOperations
  {
    void* context = nullptr;
    usize (*write)(void*, FILE*, const void*, usize, usize) = nullptr;
    int (*print)(void*, FILE*, const char*, std::va_list) = nullptr;
    int (*flush)(void*, FILE*) = nullptr;
    int (*close)(void*, FILE*) = nullptr;
  };

  /**
   * @brief Sets the callback bundle used by this writer.
   * @param operations Bundle that remains valid through execute(), or null for C stdio.
   */
  void setOutputOperations(const OutputOperations* operations) noexcept;

  /**
   * @brief Writes elements through the selected operation.
   * @param stream Open output stream.
   * @param data Source elements.
   * @param elementSize Size of one element in bytes.
   * @param count Number of elements to write.
   * @return Number of elements reported as written.
   */
  usize writeOutput(FILE* stream, const void* data, usize elementSize, usize count) const;

  /**
   * @brief Prints one formatted record through the selected operation.
   * @param stream Open output stream.
   * @param format C format string.
   * @param args Arguments used once during this call.
   * @return Number of bytes printed, or a negative value on failure.
   */
  int printOutputV(FILE* stream, const char* format, std::va_list args) const;

  /**
   * @brief Flushes the stream through the selected operation.
   * @param stream Open output stream.
   * @return Zero on success; nonzero on failure.
   */
  int flushOutput(FILE* stream) const;

  /**
   * @brief Consumes the stream through the selected close operation.
   * @param stream Open output stream that the caller releases before this call.
   * @return Zero on success; nonzero on failure.
   */
  int closeOutput(FILE* stream) const;

  /**
   * @brief Prints one formatted record and checks its result.
   * @param stream Open output stream.
   * @param format C format string.
   * @return Error with the output path if printing fails.
   */
  Result<> printOutput(FILE* stream, const char* format, ...) const;

  /**
   * @brief Writes the exact element count after checking the byte count.
   * @param stream Open output stream.
   * @param data Source elements.
   * @param elementSize Size of one element in bytes.
   * @param count Number of elements to write.
   * @return Error with the output path and counts if the transfer is incomplete.
   */
  Result<> writeOutputChecked(FILE* stream, const void* data, usize elementSize, usize count) const;

  /**
   * @brief Writes the selected Avizo header.
   * @param outputFile Open stream owned by execute().
   * @return Header warnings or an output error.
   */
  virtual Result<> generateHeader(FILE* outputFile) const = 0;

  /**
   * @brief Writes Feature IDs and any coordinate arrays.
   * @param outputFile Open stream owned by execute().
   * @return Source warnings, source errors, or output errors.
   */
  virtual Result<> writeData(FILE* outputFile) const = 0;

  DataStructure& m_DataStructure;
  const AvizoWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;

private:
  const OutputOperations* m_OutputOperations = nullptr;
};

} // namespace nx::core
