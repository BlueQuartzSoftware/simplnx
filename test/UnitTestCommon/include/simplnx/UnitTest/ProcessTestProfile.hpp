#pragma once

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <random>
#include <stdexcept>
#include <string>

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#define SIMPLNX_TEST_RESTORE_NOMINMAX
#endif
#include <Windows.h>
#ifdef SIMPLNX_TEST_RESTORE_NOMINMAX
#undef SIMPLNX_TEST_RESTORE_NOMINMAX
#undef NOMINMAX
#endif
#endif

namespace nx::core::UnitTest
{
/**
 * @brief Isolates one test process without taking ownership of borrowed fixtures.
 * Routing remains installed after finish and destruction. Call before framework or Application construction.
 * The ownership checks protect cooperative test processes, not hostile concurrent filesystem changes.
 */
class ProcessTestProfile
{
public:
  ProcessTestProfile() = default;
  ProcessTestProfile(const ProcessTestProfile&) = delete;
  ProcessTestProfile(ProcessTestProfile&&) = delete;
  ProcessTestProfile& operator=(const ProcessTestProfile&) = delete;
  ProcessTestProfile& operator=(ProcessTestProfile&&) = delete;

  ~ProcessTestProfile() noexcept
  {
    if(!m_Finished && !m_Root.empty())
    {
      try
      {
        std::cerr << "Unfinished test profile retained: " << m_Root << '\n';
      } catch(...)
      {
      }
    }
  }

  /**
   * @brief Installs a fresh profile or validates an explicit marker and token request.
   * @param error Receives a startup error. No ambient fallback is permitted.
   * @return True when routing and optional external reporting succeed.
   */
  bool install(std::string& error)
  {
    try
    {
      if(m_Attempted)
      {
        throw std::runtime_error("Test profile installation was already attempted");
      }
      m_Attempted = true;
      const char* requested = std::getenv("SIMPLNX_TEST_PROFILE_DIR");
      const char* token = std::getenv("SIMPLNX_TEST_PROFILE_TOKEN");
      if(requested != nullptr || token != nullptr)
      {
        if(requested == nullptr || token == nullptr || *requested == '\0' || *token == '\0')
        {
          throw std::runtime_error("Test profile directory and nonempty token must be supplied together");
        }
        std::filesystem::path requestedPath(requested);
        while(requestedPath != requestedPath.root_path() && (requestedPath.filename().empty() || requestedPath.filename() == "."))
        {
          requestedPath = requestedPath.parent_path();
        }
        if(!requestedPath.is_absolute() || requestedPath == requestedPath.root_path())
        {
          throw std::runtime_error("Borrowed test profile must be an absolute non-root directory");
        }
        // Ancestor aliases can name a valid fixture. The requested directory itself must remain a plain entry.
        requirePlainEntry(requestedPath);
        m_Root = std::filesystem::canonical(requestedPath);
        if(m_Root == m_Root.root_path())
        {
          throw std::runtime_error("Borrowed test profile resolves to a filesystem root");
        }
        requirePlainPath(m_Root);
        m_Token = token;
        validateMarker();
      }
      else
      {
        m_Parent = std::filesystem::canonical(std::filesystem::temp_directory_path());
        requirePlainPath(m_Parent);
        std::random_device random;
        for(int attempt = 0; attempt < 32; ++attempt)
        {
          m_Token = std::to_string(random()) + "-" + std::to_string(random()) + "-" + std::to_string(random());
          const auto candidate = m_Parent / ("simplnx-test-profile-" + m_Token);
          if(std::filesystem::create_directory(candidate))
          {
            m_Root = candidate;
            m_OwnsRoot = true;
            break;
          }
        }
        if(m_Root.empty())
        {
          throw std::runtime_error("Cannot reserve an exclusive test profile directory");
        }
        writeFile(m_Root / ".simplnx-test-profile", m_Token);
        std::filesystem::create_directories(corePreferencesFile().parent_path());
        writeFile(corePreferencesFile(), "{}");
      }

      // Core Preferences reads the narrow CRT environment on Windows.
      const auto rootString = m_Root.string();
      if(std::filesystem::path(rootString) != m_Root)
      {
        throw std::runtime_error("Test profile path cannot round-trip through the core environment");
      }
      setEnvironment(homeVariable(), rootString.c_str());
      const char* installed = std::getenv(homeVariable());
      if(installed == nullptr || std::filesystem::path(installed) != m_Root)
      {
        throw std::runtime_error("Core environment did not retain the test profile path");
      }
#ifdef _WIN32
      std::wstring nativeHome(32768, L'\0');
      const DWORD size = GetEnvironmentVariableW(L"USERPROFILE", nativeHome.data(), static_cast<DWORD>(nativeHome.size()));
      if(size == 0 || size >= nativeHome.size())
      {
        throw std::runtime_error("Inherited Windows environment has no usable test profile");
      }
      nativeHome.resize(size);
      if(std::filesystem::path(nativeHome) != m_Root)
      {
        throw std::runtime_error("CRT and inherited Windows profile paths differ");
      }
#endif
      setEnvironment("SIMPLNX_TEST_PROFILE_DIR", nullptr);
      setEnvironment("SIMPLNX_TEST_PROFILE_TOKEN", nullptr);
      setEnvironment("SIMPLNX_TEST_ACTIVE_PROFILE", rootString.c_str());
      if(const char* report = std::getenv("SIMPLNX_TEST_PROFILE_REPORT"))
      {
        const std::filesystem::path reportPath(report);
        setEnvironment("SIMPLNX_TEST_PROFILE_REPORT", nullptr);
        if(!reportPath.is_absolute())
        {
          throw std::runtime_error("Test profile report must use an absolute external path");
        }
        const auto reportParent = std::filesystem::canonical(reportPath.parent_path());
        requirePlainPath(reportParent);
        const auto resolvedReport = reportParent / reportPath.filename();
        for(auto parent = reportParent; !parent.empty(); parent = parent.parent_path())
        {
          if(parent == m_Root)
          {
            throw std::runtime_error("Test profile report must be outside the profile root");
          }
          if(parent == parent.root_path())
          {
            break;
          }
        }
        if(std::filesystem::exists(resolvedReport) || std::filesystem::is_symlink(std::filesystem::symlink_status(resolvedReport)))
        {
          throw std::runtime_error("Test profile report already exists");
        }
        writeFile(resolvedReport, rootString);
      }
      m_Installed = true;
      error.clear();
      return true;
    } catch(const std::exception& exception)
    {
      error = exception.what();
      return false;
    }
  }

  const std::filesystem::path& root() const noexcept
  {
    return m_Root;
  }
  bool ownsRoot() const noexcept
  {
    return m_OwnsRoot;
  }

  std::filesystem::path corePreferencesFile() const
  {
#ifdef _WIN32
    return m_Root / "AppData/Local/DREAM3DNX/preferences.json";
#elif defined(__APPLE__)
    return m_Root / "Library/Preferences/DREAM3DNX/preferences.json";
#else
    return m_Root / ".config/DREAM3DNX/preferences.json";
#endif
  }

  /**
   * @brief Removes only a successfully completed, exclusively owned profile.
   * @param successful True after the framework and all known owners finish successfully.
   * @param error Receives an error and the retained or residual path.
   * @return The cached completion result. Borrowed roots are never removed or repaired.
   */
  bool finish(bool successful, std::string& error) noexcept
  {
    try
    {
      if(!m_Finished)
      {
        m_Finished = true;
        if(!successful || !m_Installed)
        {
          throw std::runtime_error("Test did not complete successfully");
        }
        if(m_OwnsRoot)
        {
          requirePlainPath(m_Root);
          if(m_Root.parent_path() != m_Parent || m_Root.filename() != "simplnx-test-profile-" + m_Token || std::filesystem::canonical(m_Root) != m_Root)
          {
            throw std::runtime_error("Owned test profile path changed");
          }
          validateMarker();
          // Nested profile markers and links can belong to independent or active owners.
          for(const auto& entry : std::filesystem::recursive_directory_iterator(m_Root))
          {
            requirePlainEntry(entry.path());
            if(entry.path().filename() == ".simplnx-test-profile" && entry.path().parent_path() != m_Root)
            {
              throw std::runtime_error("Independent test profile remains below the owned root");
            }
          }
          std::error_code removeError;
          std::filesystem::remove_all(m_Root, removeError);
          if(removeError)
          {
            throw std::runtime_error("Cannot remove owned test profile: " + removeError.message());
          }
        }
        m_FinishSucceeded = true;
      }
      error = m_FinishError;
      return m_FinishSucceeded;
    } catch(const std::exception& exception)
    {
      try
      {
        m_FinishError = std::string(exception.what()) + "; test profile retained or partially removed: " + m_Root.string();
        error = m_FinishError;
      } catch(...)
      {
      }
      return false;
    } catch(...)
    {
      return false;
    }
  }

  /**
   * @brief Finishes the profile and combines its outcome with a framework exit code.
   * @param result Framework result, including successful listing counts above 255.
   * @param successful True when the framework and known owners completed successfully.
   * @param error Receives the completion error.
   * @return Original result on success; a failure in 1 through 255 otherwise.
   */
  int finishExitCode(int result, bool successful, std::string& error) noexcept
  {
    if(finish(successful, error))
    {
      return result;
    }
    // POSIX keeps only eight exit-status bits. Large listing counts must not hide cleanup failures.
    return result > 0 && result <= 255 ? result : 1;
  }

  static const char* homeVariable() noexcept
  {
#ifdef _WIN32
    return "USERPROFILE";
#else
    return "HOME";
#endif
  }

private:
  static void setEnvironment(const char* name, const char* value)
  {
#ifdef _WIN32
    const int result = _putenv_s(name, value == nullptr ? "" : value);
#else
    const int result = value == nullptr ? unsetenv(name) : setenv(name, value, 1);
#endif
    if(result != 0)
    {
      throw std::runtime_error(std::string("Cannot set test environment variable: ") + name);
    }
  }

  static void writeFile(const std::filesystem::path& path, const std::string& text)
  {
    std::ofstream stream(path, std::ios::binary);
    stream << text;
    stream.close();
    if(!stream)
    {
      throw std::runtime_error("Cannot write test profile file: " + path.string());
    }
  }

  static void requirePlainEntry(const std::filesystem::path& path)
  {
    const auto status = std::filesystem::symlink_status(path);
    if(!std::filesystem::is_directory(status) && !std::filesystem::is_regular_file(status))
    {
      throw std::runtime_error("Test profile contains a missing path, link or special file: " + path.string());
    }
#ifdef _WIN32
    const DWORD attributes = GetFileAttributesW(path.c_str());
    if(attributes == INVALID_FILE_ATTRIBUTES || (attributes & FILE_ATTRIBUTE_REPARSE_POINT) != 0)
    {
      throw std::runtime_error("Test profile contains a reparse point or unreadable path: " + path.string());
    }
#endif
  }

  static void requirePlainPath(const std::filesystem::path& path)
  {
    for(auto parent = path; !parent.empty(); parent = parent.parent_path())
    {
      requirePlainEntry(parent);
      if(parent == parent.root_path())
      {
        break;
      }
    }
  }

  void validateMarker() const
  {
    if(!std::filesystem::is_directory(m_Root))
    {
      throw std::runtime_error("Test profile is not a directory");
    }
    const auto marker = m_Root / ".simplnx-test-profile";
    requirePlainEntry(marker);
    if(!std::filesystem::is_regular_file(marker) || std::filesystem::file_size(marker) != m_Token.size())
    {
      throw std::runtime_error("Test profile marker is not a matching regular file");
    }
    std::ifstream stream(marker, std::ios::binary);
    const std::string actual((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
    if(!stream || actual != m_Token)
    {
      throw std::runtime_error("Test profile marker token does not match");
    }
  }

  std::filesystem::path m_Root;
  std::filesystem::path m_Parent;
  std::string m_Token;
  std::string m_FinishError;
  bool m_Attempted = false;
  bool m_Installed = false;
  bool m_OwnsRoot = false;
  bool m_Finished = false;
  bool m_FinishSucceeded = false;
};
} // namespace nx::core::UnitTest
