#pragma once

#include "simplnx/simplnx_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <memory>
#include <string_view>

namespace nx::core
{
/**
 * @enum FileWriteHookMode
 * @brief Selects what a file-replace handler does with a component that reads the target file.
 *
 * A writer replaces the file at its target path. A component that reads that file keeps the path, so after the write it
 * reads other data or holds a handle that blocks the write. The writer selects how a handler prevents this with
 * ScopedFileReplacePolicy.
 */
enum class FileWriteHookMode : uint8
{
  ReleaseReaders, ///< Makes each component that reads the target stop reading it before the write. This mode is the default.
  RefuseIfRead    ///< Changes nothing and returns k_FileReplaceRefusedError if a component reads the target.
};

/**
 * @brief Error code of a handler that cannot make the replacement of the target safe.
 *
 * A handler returns this code when a component that reads the target cannot stop reading it. A handler also returns it
 * when it cannot find out whether a component reads the target.
 */
inline constexpr int32 k_FileReplaceUnsafeError = -6039;

/**
 * @brief Error code of a handler in FileWriteHookMode::RefuseIfRead when a component still reads the target.
 */
inline constexpr int32 k_FileReplaceRefusedError = -6053;

/**
 * @brief Error code of BeforeFileReplace() when a handler on the same thread calls it.
 */
inline constexpr int32 k_FileReplaceNestedError = -6061;

/**
 * @brief Prepares the components of one subsystem for the replacement of a file.
 *
 * The first argument is the target path as the writer passes it to BeforeFileReplace(). The second argument is the mode
 * of the innermost ScopedFileReplacePolicy on the calling thread. The handler returns a valid result, possibly with
 * warnings, when no component of its subsystem reads the target after the call. Otherwise it returns an invalid result
 * whose message contains the target.
 */
using FileReplaceHandler = std::function<Result<>(const std::filesystem::path& target, FileWriteHookMode mode)>;

/**
 * @brief Registers a handler that BeforeFileReplace() calls for every target in this process.
 * @param handler Handler to call. An empty handler registers nothing.
 * @return Registration token. The handler stays registered while a copy of the token exists. Null when handler is empty.
 * @throws std::bad_alloc When an allocation fails. The handler is then not registered.
 *
 * BeforeFileReplace() calls the handlers in registration order. The function is safe to call from any thread, also from
 * a handler while BeforeFileReplace() runs. A handler that registers during a BeforeFileReplace() call does not run in
 * that call. The last copy of the token unregisters the handler. A BeforeFileReplace() call that already copied the
 * handler list can still run the handler once after the release. Thus the handler must stay safe to call until every
 * call that started before the release has returned.
 */
SIMPLNX_EXPORT std::shared_ptr<void> RegisterFileReplaceHandler(FileReplaceHandler handler);

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
/**
 * @brief Makes the next RegisterFileReplaceHandler() call throw std::bad_alloc where it allocates the registration token.
 *
 * This is a `*ForTesting` seam for the exception safety of the registration. The flag clears itself on that call.
 */
SIMPLNX_EXPORT void FailNextFileReplaceRegistrationForTesting();
#endif

/**
 * @brief Lets the registered handlers prepare their components before a writer changes a file.
 * @param target Final path that the writer creates, truncates, opens for writing or renames over. The handlers receive
 * it as given; AtomicFile passes an absolute path.
 * @return Valid result with the warnings of all handlers. Otherwise the first invalid handler result, with the warnings
 * of the earlier handlers in front of its own. An invalid handler result with no errors stays invalid. Error
 * k_FileReplaceNestedError (-6061) when a handler on the calling thread makes the call. Valid with no warnings when no
 * handler is registered.
 *
 * A component that reads a file keeps the path of that file. If a writer replaces the file, the component reads other
 * data or holds a handle that blocks the write. For each target, the writer runs this function before it changes that
 * target. If the call is invalid, that target stays unchanged. Earlier outputs can already have changed. A writer that
 * gives a stronger guarantee, for example no change to any of its files, documents it. AtomicFile::Create(), the DREAM3D
 * writers and the path-based HDF5::DataStructureWriter writers make the call. Do not call it in preflight.
 *
 * The handlers receive the mode of the innermost ScopedFileReplacePolicy of the calling thread. The default mode is
 * FileWriteHookMode::ReleaseReaders. The function copies the handler list under a mutex and runs the handlers without it. The first invalid
 * result stops the calls, so a later handler does not run. An exception from a handler propagates unchanged.
 *
 * A handler must not call this function, also not through a writer: the nested call returns k_FileReplaceNestedError
 * (-6061) and calls no handler.
 *
 * Limitations:
 * - A component that starts to read target after this call returns is not covered. This includes a component that
 *   starts to read target while the writer still writes it.
 * - Writers that do not call this function are not covered, for example code that opens the file directly with a
 *   third-party library.
 * - Other processes are not covered.
 */
SIMPLNX_EXPORT Result<> BeforeFileReplace(const std::filesystem::path& target);

/**
 * @brief Error code of OpenFileForReplace() and ReplaceFileWithText() when the file cannot be opened.
 */
inline constexpr int32 k_ReplaceFileOpenError = -6067;

/**
 * @brief Error code of ReplaceFileWithText() when the text cannot be written.
 */
inline constexpr int32 k_ReplaceFileWriteError = -6068;

/**
 * @brief Error code of ReplaceFileWithText() when the flush at the close fails.
 */
inline constexpr int32 k_ReplaceFileCloseError = -6069;

/**
 * @brief Runs BeforeFileReplace() on a file, then opens the file for writing and truncates it.
 * @param filePath File to replace.
 * @param stream Stream that receives the open file. It must not be open. Its exception mask stays as it is; an open
 * failure returns k_ReplaceFileOpenError also when the mask makes the open throw.
 * @return Success with the hook's warnings. Otherwise the hook result unchanged, which leaves the file unchanged, or
 * k_ReplaceFileOpenError (-6067) naming filePath, with the hook's warnings.
 *
 * Use this function for a file that is written in place over a longer time, for example a log file.
 */
SIMPLNX_EXPORT Result<> OpenFileForReplace(const std::filesystem::path& filePath, std::fstream& stream);

/**
 * @brief Runs BeforeFileReplace() on a file, then replaces the file with text.
 * @param filePath File to replace.
 * @param text New content of the file.
 * @return Success with the hook's warnings. Otherwise the hook result unchanged, which leaves the file unchanged. Other
 * errors name filePath and carry the hook's warnings: k_ReplaceFileOpenError (-6067), k_ReplaceFileWriteError (-6068)
 * or k_ReplaceFileCloseError (-6069). After -6068 or -6069 the file can be incomplete.
 */
SIMPLNX_EXPORT Result<> ReplaceFileWithText(const std::filesystem::path& filePath, std::string_view text);

#if defined(SIMPLNX_BUILD_TESTS) && SIMPLNX_BUILD_TESTS
/**
 * @enum TextStreamFailureForTesting
 * @brief Selects a stream failure that the text file writers simulate.
 */
enum class TextStreamFailureForTesting : uint8
{
  None,  ///< Simulates no failure.
  Write, ///< Marks the stream as failed after the text is written.
  Close  ///< Marks the stream as failed after the close.
};

/**
 * @brief Makes every later text file write in this process simulate a stream failure.
 * @param failure Failure to simulate, or TextStreamFailureForTesting::None to stop.
 *
 * This is a `*ForTesting` seam for the write and close failure branches of ReplaceFileWithText() and of the XDMF writer. A
 * real file system does not produce these failures on demand. The storage is in FileReplaceGuard.cpp, so simplnx and every test executable
 * share one value. A test that sets a value restores None before it ends.
 */
SIMPLNX_EXPORT void SetTextStreamFailureForTesting(TextStreamFailureForTesting failure);

/**
 * @brief Name of the environment variable that RegisterFileReplaceHandlerFromEnvironmentForTesting() reads.
 */
inline constexpr const char* k_TestFileReplaceHandlerVariable = "SIMPLNX_TEST_FILE_REPLACE_HANDLER";

/**
 * @brief Name of the environment variable that names the observation file of the test handler.
 */
inline constexpr const char* k_TestFileReplaceLogVariable = "SIMPLNX_TEST_FILE_REPLACE_LOG";

/**
 * @brief Registers the file-replace handler that a process test selects through SIMPLNX_TEST_FILE_REPLACE_HANDLER.
 * @return Registration token, or null when the variable is not set or has no '|'. Every call returns the same value.
 *
 * The value is "<action>|<target path>". The handler compares lexically normalized target paths. For a matching target,
 * "warn" returns warning 99701. The action "refuse" returns error -99702. The action "refuse-empty" returns an invalid
 * result without errors. nxrunner and the Python module call this function at startup. This lets process tests check
 * results at those boundaries.
 *
 * The first call registers the handler. Later calls return the same token. Thus nxrunner and its embedded Python module
 * share one handler. The handler stays registered until the process ends.
 *
 * When SIMPLNX_TEST_FILE_REPLACE_LOG names a file, the handler writes observations to that file.
 * Each matching call appends one observation. The line format is:
 * @code
 * action=<action> valid=<0|1> warnings=<codes> errors=<codes> size=<bytes|absent> fnv1a64=<16 hex digits|absent> target=<UTF-8 path>
 * @endcode
 * The valid field is 1 for a valid result and 0 for an invalid result.
 * The warnings and errors fields contain comma-separated decimal codes, or remain empty.
 * The size and the 64-bit FNV-1a hash describe the target bytes at callback time.
 * This function exists only in builds with SIMPLNX_BUILD_TESTS.
 */
SIMPLNX_EXPORT std::shared_ptr<void> RegisterFileReplaceHandlerFromEnvironmentForTesting();

/**
 * @brief Returns the stream failure that SetTextStreamFailureForTesting() selected.
 * @return The selected failure, TextStreamFailureForTesting::None by default.
 */
SIMPLNX_EXPORT TextStreamFailureForTesting GetTextStreamFailureForTesting();
#endif

/**
 * @class ScopedFileReplacePolicy
 * @brief Sets the FileWriteHookMode that BeforeFileReplace() passes to the handlers on the current thread.
 *
 * The policy applies only to the thread that creates it. The innermost policy wins. The destructor restores the mode
 * that was active when the object was created. Destroy the objects in the reverse order of creation and on the thread
 * that created them; a local variable does both.
 */
class SIMPLNX_EXPORT ScopedFileReplacePolicy
{
public:
  /**
   * @brief Makes mode the current mode of the calling thread.
   * @param mode Mode that BeforeFileReplace() passes to the handlers until this object is destroyed.
   */
  explicit ScopedFileReplacePolicy(FileWriteHookMode mode) noexcept;

  /**
   * @brief Restores the mode that was current when this object was created.
   */
  ~ScopedFileReplacePolicy() noexcept;

  ScopedFileReplacePolicy(const ScopedFileReplacePolicy&) = delete;
  ScopedFileReplacePolicy(ScopedFileReplacePolicy&&) = delete;
  ScopedFileReplacePolicy& operator=(const ScopedFileReplacePolicy&) = delete;
  ScopedFileReplacePolicy& operator=(ScopedFileReplacePolicy&&) = delete;

private:
  FileWriteHookMode m_PreviousMode = FileWriteHookMode::ReleaseReaders;
};
} // namespace nx::core
