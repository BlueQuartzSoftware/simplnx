#include "FileReplaceGuard.hpp"

#include "simplnx/Common/PathText.hpp"

#include <fmt/format.h>

#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <fstream>
#include <ios>
#include <iterator>
#include <memory>
#include <mutex>
#include <new>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

using namespace nx::core;

namespace fs = std::filesystem;

namespace
{
/**
 * @brief Holds the registered handlers of the process.
 *
 * Each token owns a shared_ptr to the registry, so the registry outlives every token, also during static destruction.
 */
struct FileReplaceRegistry
{
  std::mutex mutex;
  std::vector<std::shared_ptr<const FileReplaceHandler>> handlers;
};

/**
 * @brief Returns the registry of the process.
 * @return Shared owner of the registry.
 */
std::shared_ptr<FileReplaceRegistry> GetFileReplaceRegistry()
{
  static const std::shared_ptr<FileReplaceRegistry> s_Registry = std::make_shared<FileReplaceRegistry>();
  return s_Registry;
}

/**
 * @class FileReplaceRegistration
 * @brief Token object of one registered handler. Its destructor removes the handler from the registry.
 */
class FileReplaceRegistration
{
public:
  /**
   * @brief Creates the token of a registered handler.
   * @param registry Registry that holds the handler. The token keeps it alive.
   * @param entry Registered handler, identified by its address.
   */
  FileReplaceRegistration(std::shared_ptr<FileReplaceRegistry> registry, const FileReplaceHandler* entry) noexcept
  : m_Registry(std::move(registry))
  , m_Entry(entry)
  {
  }

  /**
   * @brief Removes the handler from the registry.
   *
   * The handler leaves the registry under the mutex but is destroyed after the mutex is released. A handler can own other
   * registration tokens through its captures, and their destructors lock the same mutex.
   */
  ~FileReplaceRegistration() noexcept
  {
    std::shared_ptr<const FileReplaceHandler> removedHandler;
    {
      const std::lock_guard<std::mutex> lock(m_Registry->mutex);
      std::vector<std::shared_ptr<const FileReplaceHandler>>& handlers = m_Registry->handlers;
      const FileReplaceHandler* entry = m_Entry;
      const auto entryIter = std::find_if(handlers.begin(), handlers.end(), [entry](const std::shared_ptr<const FileReplaceHandler>& candidate) { return candidate.get() == entry; });
      if(entryIter != handlers.end())
      {
        removedHandler = std::move(*entryIter);
        handlers.erase(entryIter);
      }
    }
  }

  FileReplaceRegistration(const FileReplaceRegistration&) = delete;
  FileReplaceRegistration(FileReplaceRegistration&&) = delete;
  FileReplaceRegistration& operator=(const FileReplaceRegistration&) = delete;
  FileReplaceRegistration& operator=(FileReplaceRegistration&&) = delete;

private:
  std::shared_ptr<FileReplaceRegistry> m_Registry;
  const FileReplaceHandler* m_Entry = nullptr;
};

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
/**
 * @brief True when the next registration simulates a failed allocation of its token.
 */
std::atomic_bool g_FailNextRegistrationAllocation = false;

/**
 * @brief Stream failure that the text file writers simulate.
 */
std::atomic<TextStreamFailureForTesting> g_TextStreamFailure = TextStreamFailureForTesting::None;

/**
 * @brief Splits the value of SIMPLNX_TEST_FILE_REPLACE_HANDLER into the action and the target path.
 * @tparam CharT Character type of the environment value: wchar_t on Windows, char elsewhere.
 * @param setting Value "<action>|<target path>".
 * @return The action as ASCII text and the lexically normalized target path, or no value when the value has no '|'.
 */
template <class CharT>
std::optional<std::pair<std::string, fs::path>> ParseTestHandlerSetting(std::basic_string_view<CharT> setting)
{
  const usize separator = setting.find(static_cast<CharT>('|'));
  if(separator == std::basic_string_view<CharT>::npos)
  {
    return std::nullopt;
  }
  std::string action;
  for(const CharT character : setting.substr(0, separator))
  {
    action.push_back(static_cast<char>(character));
  }
  return std::pair{std::move(action), fs::path(std::basic_string<CharT>(setting.substr(separator + 1))).lexically_normal()};
}

/**
 * @brief Reads a path-valued environment variable.
 * @param narrowName Variable name for std::getenv().
 * @param wideName The same name for _wgetenv() on Windows.
 * @return The value, or no value when the variable is not set.
 *
 * Windows reads the variable as UTF-16, so a path with any Unicode character works there. POSIX keeps the bytes.
 */
std::optional<std::basic_string<fs::path::value_type>> ReadEnvironmentVariable([[maybe_unused]] const char* narrowName, [[maybe_unused]] const wchar_t* wideName)
{
#ifdef _WIN32
  const wchar_t* value = _wgetenv(wideName);
#else
  const char* value = std::getenv(narrowName);
#endif
  if(value == nullptr)
  {
    return std::nullopt;
  }
  return std::basic_string<fs::path::value_type>(value);
}

/**
 * @brief Computes the 64-bit FNV-1a hash of a file.
 * @param filePath File to read.
 * @return The byte count and the hash, or no value when the file cannot be read.
 */
std::optional<std::pair<uint64, uint64>> HashFileBytes(const fs::path& filePath)
{
  std::ifstream stream(filePath, std::ios_base::binary);
  if(!stream.is_open())
  {
    return std::nullopt;
  }
  constexpr uint64 k_FnvOffsetBasis = 14695981039346656037ULL;
  constexpr uint64 k_FnvPrime = 1099511628211ULL;
  uint64 hash = k_FnvOffsetBasis;
  uint64 size = 0;
  char character = 0;
  while(stream.get(character))
  {
    hash ^= static_cast<uint64>(static_cast<uint8>(character));
    hash *= k_FnvPrime;
    size++;
  }
  return std::pair{size, hash};
}

/**
 * @brief Appends one observation line for a matching hook call to the file that SIMPLNX_TEST_FILE_REPLACE_LOG names.
 * @param logPath Observation file.
 * @param target Target of the hook call.
 * @param action Action the handler takes.
 * @param result Result that the handler returns.
 *
 * RegisterFileReplaceHandlerFromEnvironmentForTesting() documents the line format.
 * The size and the hash describe the target at callback time, so a test can prove the hook ran before the write.
 */
void AppendHookObservation(const fs::path& logPath, const fs::path& target, std::string_view action, const Result<>& result)
{
  std::string warningCodes;
  for(const auto& warning : result.warnings())
  {
    warningCodes += (warningCodes.empty() ? "" : ",") + std::to_string(warning.code);
  }
  std::string errorCodes;
  if(result.invalid())
  {
    for(const auto& error : result.errors())
    {
      errorCodes += (errorCodes.empty() ? "" : ",") + std::to_string(error.code);
    }
  }
  const std::optional<std::pair<uint64, uint64>> fileHash = HashFileBytes(target);
  const std::string sizeText = fileHash.has_value() ? std::to_string(fileHash->first) : std::string("absent");
  const std::string hashText = fileHash.has_value() ? fmt::format("{:016x}", fileHash->second) : std::string("absent");
  std::ofstream logStream(logPath, std::ios_base::out | std::ios_base::app | std::ios_base::binary);
  logStream << fmt::format("action={} valid={} warnings={} errors={} size={} fnv1a64={} target={}\n", action, result.valid() ? 1 : 0, warningCodes, errorCodes, sizeText, hashText,
                           PathToUtf8Text(target));
}

/**
 * @brief Registers the test handler that SIMPLNX_TEST_FILE_REPLACE_HANDLER selects.
 * @return Registration token, or null when the variable is not set or has no '|'.
 *
 * When SIMPLNX_TEST_FILE_REPLACE_LOG names a file, each call for the matching target appends one observation line to it.
 */
std::shared_ptr<void> RegisterEnvironmentHandler()
{
  const std::optional<std::basic_string<fs::path::value_type>> setting = ReadEnvironmentVariable(k_TestFileReplaceHandlerVariable, L"SIMPLNX_TEST_FILE_REPLACE_HANDLER");
  if(!setting.has_value())
  {
    return nullptr;
  }
  std::optional<std::pair<std::string, fs::path>> parsedSetting = ParseTestHandlerSetting(std::basic_string_view<fs::path::value_type>(*setting));
  if(!parsedSetting.has_value())
  {
    return nullptr;
  }
  const std::optional<std::basic_string<fs::path::value_type>> logSetting = ReadEnvironmentVariable(k_TestFileReplaceLogVariable, L"SIMPLNX_TEST_FILE_REPLACE_LOG");
  std::optional<fs::path> observationLogPath = logSetting.has_value() ? std::optional<fs::path>(fs::path(*logSetting)) : std::nullopt;
  return RegisterFileReplaceHandler(
      [action = std::move(parsedSetting->first), targetPath = std::move(parsedSetting->second), logPath = std::move(observationLogPath)](const fs::path& target, FileWriteHookMode) -> Result<> {
        if(target.lexically_normal() != targetPath)
        {
          return {};
        }
        Result<> result;
        if(action == "warn")
        {
          result = MakeWarningVoidResult(99701, fmt::format("Test file-replace handler warning for '{}'", PathToUtf8Text(target)));
        }
        else if(action == "refuse")
        {
          result = MakeErrorResult(-99702, fmt::format("Test file-replace handler refusal for '{}'", PathToUtf8Text(target)));
        }
        else if(action == "refuse-empty")
        {
          result = Result<>{{nonstd::make_unexpected(ErrorCollection{})}};
        }
        const bool isKnownAction = action == "warn" || action == "refuse" || action == "refuse-empty";
        if(logPath.has_value())
        {
          AppendHookObservation(*logPath, target, isKnownAction ? std::string_view(action) : std::string_view("none"), result);
        }
        return result;
      });
}
#endif

/**
 * @brief Mode that BeforeFileReplace() passes to the handlers on this thread.
 */
thread_local FileWriteHookMode t_FileReplaceMode = FileWriteHookMode::ReleaseReaders;

/**
 * @brief True while a handler runs on this thread.
 */
thread_local bool t_IsInFileReplaceHandler = false;

/**
 * @class HandlerCallScope
 * @brief Marks the calling thread as inside a handler for the lifetime of the object.
 *
 * The destructor clears the mark, also when the handler throws, so a later BeforeFileReplace() call on the thread works.
 * BeforeFileReplace() creates the object only when the mark is clear, so clearing restores the previous state.
 */
class HandlerCallScope
{
public:
  HandlerCallScope() noexcept
  {
    t_IsInFileReplaceHandler = true;
  }

  ~HandlerCallScope() noexcept
  {
    t_IsInFileReplaceHandler = false;
  }

  HandlerCallScope(const HandlerCallScope&) = delete;
  HandlerCallScope(HandlerCallScope&&) = delete;
  HandlerCallScope& operator=(const HandlerCallScope&) = delete;
  HandlerCallScope& operator=(HandlerCallScope&&) = delete;
};
} // namespace

// -----------------------------------------------------------------------------
std::shared_ptr<void> nx::core::RegisterFileReplaceHandler(FileReplaceHandler handler)
{
  if(!handler)
  {
    return nullptr;
  }
  std::shared_ptr<FileReplaceRegistry> registry = GetFileReplaceRegistry();
  auto entry = std::make_shared<const FileReplaceHandler>(std::move(handler));
  // The token exists before the handler is published. If an allocation throws, no handler stays registered: before the
  // publication nothing is registered, and a failed push_back() leaves the registry unchanged.
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  if(g_FailNextRegistrationAllocation.exchange(false, std::memory_order_relaxed))
  {
    throw std::bad_alloc();
  }
#endif
  std::shared_ptr<void> registration = std::make_shared<FileReplaceRegistration>(registry, entry.get());
  {
    const std::lock_guard<std::mutex> lock(registry->mutex);
    registry->handlers.push_back(std::move(entry));
  }
  return registration;
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
// -----------------------------------------------------------------------------
void nx::core::FailNextFileReplaceRegistrationForTesting()
{
  g_FailNextRegistrationAllocation.store(true, std::memory_order_relaxed);
}
#endif

// -----------------------------------------------------------------------------
Result<> nx::core::BeforeFileReplace(const fs::path& target)
{
  if(t_IsInFileReplaceHandler)
  {
    return MakeErrorResult(k_FileReplaceNestedError,
                           fmt::format("Cannot prepare '{}' for replacement: the request comes from a component that prepares another file for replacement on the same thread. "
                                       "A component must not write a file while it prepares a replacement.",
                                       PathToUtf8Text(target)));
  }

  std::vector<std::shared_ptr<const FileReplaceHandler>> handlers;
  {
    const std::shared_ptr<FileReplaceRegistry> registry = GetFileReplaceRegistry();
    const std::lock_guard<std::mutex> lock(registry->mutex);
    handlers = registry->handlers;
  }

  const FileWriteHookMode mode = t_FileReplaceMode;
  std::vector<Warning> warnings;
  for(const std::shared_ptr<const FileReplaceHandler>& handler : handlers)
  {
    Result<> handlerResult;
    {
      const HandlerCallScope callScope;
      handlerResult = (*handler)(target, mode);
    }
    if(handlerResult.invalid())
    {
      // The failing result is returned itself. MergeResults() derives validity from the error count, so it would turn an
      // invalid result with no errors into a success.
      std::vector<Warning>& failingWarnings = handlerResult.warnings();
      failingWarnings.insert(failingWarnings.begin(), std::make_move_iterator(warnings.begin()), std::make_move_iterator(warnings.end()));
      return handlerResult;
    }
    std::vector<Warning>& handlerWarnings = handlerResult.warnings();
    warnings.insert(warnings.end(), std::make_move_iterator(handlerWarnings.begin()), std::make_move_iterator(handlerWarnings.end()));
  }

  Result<> result;
  result.warnings() = std::move(warnings);
  return result;
}

// -----------------------------------------------------------------------------
Result<> nx::core::OpenFileForReplace(const fs::path& filePath, std::fstream& stream)
{
  Result<> hookResult = BeforeFileReplace(filePath);
  if(hookResult.invalid())
  {
    return hookResult;
  }
  try
  {
    stream.open(filePath, std::ios_base::out | std::ios_base::trunc);
  } catch(const std::ios_base::failure&)
  {
    // A stream with an exception mask throws on an open failure. The failure is reported as -6067 below, with the hook
    // warnings, as for a stream without an exception mask.
  }
  if(!stream.is_open())
  {
    Result<> openResult =
        MakeErrorResult(k_ReplaceFileOpenError, fmt::format("Cannot open '{}' for writing. Check that the folder exists and that you have write permission.", PathToUtf8Text(filePath)));
    openResult.warnings() = std::move(hookResult.warnings());
    return openResult;
  }
  return hookResult;
}

// -----------------------------------------------------------------------------
Result<> nx::core::ReplaceFileWithText(const fs::path& filePath, std::string_view text)
{
  std::fstream stream;
  Result<> result = OpenFileForReplace(filePath, stream);
  if(result.invalid())
  {
    return result;
  }
  const auto failWith = [&result](int32 code, std::string message) {
    Result<> failure = MakeErrorResult(code, std::move(message));
    failure.warnings() = std::move(result.warnings());
    return failure;
  };

  stream.write(text.data(), static_cast<std::streamsize>(text.size()));
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  if(GetTextStreamFailureForTesting() == TextStreamFailureForTesting::Write)
  {
    stream.setstate(std::ios_base::badbit);
  }
#endif
  if(stream.fail())
  {
    return failWith(k_ReplaceFileWriteError, fmt::format("Cannot write '{}'. Check the free disk space.", PathToUtf8Text(filePath)));
  }

  // Buffered text reaches the file only at the flush, so the close is checked separately.
  stream.close();
#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
  if(GetTextStreamFailureForTesting() == TextStreamFailureForTesting::Close)
  {
    stream.setstate(std::ios_base::failbit);
  }
#endif
  if(stream.fail())
  {
    return failWith(k_ReplaceFileCloseError, fmt::format("Cannot flush and close '{}'. The file can be incomplete. Check the free disk space.", PathToUtf8Text(filePath)));
  }
  return result;
}

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
// -----------------------------------------------------------------------------
void nx::core::SetTextStreamFailureForTesting(TextStreamFailureForTesting failure)
{
  g_TextStreamFailure.store(failure, std::memory_order_relaxed);
}

// -----------------------------------------------------------------------------
std::shared_ptr<void> nx::core::RegisterFileReplaceHandlerFromEnvironmentForTesting()
{
  // nxrunner and the Python module that nxrunner embeds both call this function. The static registers the handler once
  // per process and returns the same token to every caller.
  static const std::shared_ptr<void> s_Registration = RegisterEnvironmentHandler();
  return s_Registration;
}

// -----------------------------------------------------------------------------
TextStreamFailureForTesting nx::core::GetTextStreamFailureForTesting()
{
  return g_TextStreamFailure.load(std::memory_order_relaxed);
}
#endif

// -----------------------------------------------------------------------------
ScopedFileReplacePolicy::ScopedFileReplacePolicy(FileWriteHookMode mode) noexcept
: m_PreviousMode(t_FileReplaceMode)
{
  t_FileReplaceMode = mode;
}

// -----------------------------------------------------------------------------
ScopedFileReplacePolicy::~ScopedFileReplacePolicy() noexcept
{
  t_FileReplaceMode = m_PreviousMode;
}
