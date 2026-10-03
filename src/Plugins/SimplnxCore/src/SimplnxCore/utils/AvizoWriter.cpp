#include "AvizoWriter.hpp"

#include "simplnx/Common/ScopeGuard.hpp"
#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/Utilities/FilterUtilities.hpp"

#include <cerrno>
#include <exception>
#include <limits>
#include <new>
#include <string>
#include <system_error>
#include <utility>

using namespace nx::core;

namespace
{
std::string SystemErrorText(int errorNumber)
{
  if(errorNumber == 0)
  {
    return "No system error was reported";
  }
  return fmt::format("System error {}: {}", errorNumber, std::error_code(errorNumber, std::generic_category()).message());
}
} // namespace

// -----------------------------------------------------------------------------
AvizoWriter::AvizoWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, AvizoWriterInputValues* inputValues)
: m_DataStructure(dataStructure)
, m_InputValues(inputValues)
, m_ShouldCancel(shouldCancel)
, m_MessageHandler(mesgHandler)
{
}

// -----------------------------------------------------------------------------
AvizoWriter::~AvizoWriter() noexcept = default;

// -----------------------------------------------------------------------------
const std::atomic_bool& AvizoWriter::getCancel()
{
  return m_ShouldCancel;
}

void AvizoWriter::setOutputOperations(const OutputOperations* operations) noexcept
{
  m_OutputOperations = operations;
}

usize AvizoWriter::writeOutput(FILE* stream, const void* data, usize elementSize, usize count) const
{
  if(m_OutputOperations != nullptr && m_OutputOperations->write != nullptr)
  {
    return m_OutputOperations->write(m_OutputOperations->context, stream, data, elementSize, count);
  }
  return std::fwrite(data, elementSize, count, stream);
}

int AvizoWriter::printOutputV(FILE* stream, const char* format, std::va_list args) const
{
  if(m_OutputOperations != nullptr && m_OutputOperations->print != nullptr)
  {
    return m_OutputOperations->print(m_OutputOperations->context, stream, format, args);
  }
  return std::vfprintf(stream, format, args);
}

int AvizoWriter::flushOutput(FILE* stream) const
{
  if(m_OutputOperations != nullptr && m_OutputOperations->flush != nullptr)
  {
    return m_OutputOperations->flush(m_OutputOperations->context, stream);
  }
  return std::fflush(stream);
}

int AvizoWriter::closeOutput(FILE* stream) const
{
  if(m_OutputOperations != nullptr && m_OutputOperations->close != nullptr)
  {
    return m_OutputOperations->close(m_OutputOperations->context, stream);
  }
  return std::fclose(stream);
}

Result<> AvizoWriter::printOutput(FILE* stream, const char* format, ...) const
{
  std::va_list args;
  va_start(args, format);
  const auto endArgs = MakeScopeGuard([&args]() noexcept { va_end(args); });
  errno = 0;
  const int printed = printOutputV(stream, format, args);
  const int errorNumber = errno;
  if(printed < 0)
  {
    return MakeErrorResult(-5831, fmt::format("Cannot write text to Avizo output '{}'. {}.", m_InputValues->OutputFile.string(), SystemErrorText(errorNumber)));
  }
  return {};
}

Result<> AvizoWriter::writeOutputChecked(FILE* stream, const void* data, usize elementSize, usize count) const
{
  if(elementSize != 0 && count > std::numeric_limits<usize>::max() / elementSize)
  {
    return MakeErrorResult(
        -5833, fmt::format("Cannot write {} elements of {} bytes each to Avizo output '{}': the byte count exceeds the platform limit.", count, elementSize, m_InputValues->OutputFile.string()));
  }

  const usize expectedBytes = elementSize * count;
  errno = 0;
  const usize written = writeOutput(stream, data, elementSize, count);
  const int errorNumber = errno;
  if(written != count)
  {
    const std::string actualBytes = elementSize != 0 && written > std::numeric_limits<usize>::max() / elementSize ? "unrepresentable" : std::to_string(written * elementSize);
    return MakeErrorResult(-5832, fmt::format("Cannot write Avizo output '{}': requested {} elements ({} bytes), wrote {} elements ({} bytes), with {} bytes per element. {}.",
                                              m_InputValues->OutputFile.string(), count, expectedBytes, written, actualBytes, elementSize, SystemErrorText(errorNumber)));
  }
  return {};
}

// -----------------------------------------------------------------------------
Result<> AvizoWriter::execute()
{
  // Make sure any directory path is also available as the user may have just typed
  // in a path without actually creating the full path
  Result<> result = CreateOutputDirectories(m_InputValues->OutputFile.parent_path());
  if(result.invalid())
  {
    return result;
  }

  const std::string outputPath = m_InputValues->OutputFile.string();
  errno = 0;
  FILE* outputFile = std::fopen(outputPath.c_str(), "wb");
  const int openErrorNumber = errno;
  if(outputFile == nullptr)
  {
    auto openError = MakeErrorResult(-5830, fmt::format("Cannot open Avizo output '{}'. Check the directory and write permissions. {}.", outputPath, SystemErrorText(openErrorNumber)));
    return MergeResults(std::move(result), std::move(openError));
  }

  const auto closeOnExit = MakeScopeGuard([this, &outputFile]() noexcept {
    if(outputFile != nullptr)
    {
      // A close attempt consumes the stream even if the operation reports failure.
      FILE* ownedFile = std::exchange(outputFile, nullptr);
      try
      {
        (void)closeOutput(ownedFile);
      } catch(...)
      {
      }
    }
  });

  const char* stage = "header";
  try
  {
    auto headerResult = generateHeader(outputFile);
    result = MergeResults(std::move(result), std::move(headerResult));
    if(result.valid())
    {
      stage = "data";
      auto dataResult = writeData(outputFile);
      result = MergeResults(std::move(result), std::move(dataResult));
    }
    if(result.valid())
    {
      stage = "flush";
      errno = 0;
      const int flushStatus = flushOutput(outputFile);
      const int flushErrorNumber = errno;
      if(flushStatus != 0)
      {
        auto flushError = MakeErrorResult(-5834, fmt::format("Cannot flush Avizo output '{}'. {}.", outputPath, SystemErrorText(flushErrorNumber)));
        result = MergeResults(std::move(result), std::move(flushError));
      }
    }
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& exception)
  {
    auto stageError = MakeErrorResult(-5836, fmt::format("Cannot write Avizo output '{}' during {}: {}.", outputPath, stage, exception.what()));
    result = MergeResults(std::move(result), std::move(stageError));
  } catch(...)
  {
    auto stageError = MakeErrorResult(-5837, fmt::format("Cannot write Avizo output '{}' during {}: an unknown error occurred.", outputPath, stage));
    result = MergeResults(std::move(result), std::move(stageError));
  }

  // The owner releases FILE before close because fclose can fail after consuming it.
  FILE* closingFile = std::exchange(outputFile, nullptr);
  try
  {
    errno = 0;
    const int closeStatus = closeOutput(closingFile);
    const int closeErrorNumber = errno;
    if(closeStatus != 0)
    {
      auto closeError = MakeErrorResult(-5835, fmt::format("Cannot close Avizo output '{}'. {}.", outputPath, SystemErrorText(closeErrorNumber)));
      result = MergeResults(std::move(result), std::move(closeError));
    }
  } catch(const std::bad_alloc&)
  {
    throw;
  } catch(const std::exception& exception)
  {
    auto closeError = MakeErrorResult(-5835, fmt::format("Cannot close Avizo output '{}': {}.", outputPath, exception.what()));
    result = MergeResults(std::move(result), std::move(closeError));
  } catch(...)
  {
    auto closeError = MakeErrorResult(-5835, fmt::format("Cannot close Avizo output '{}': an unknown error occurred.", outputPath));
    result = MergeResults(std::move(result), std::move(closeError));
  }

  return result;
}
