#include "simplnx/UnitTest/ProcessTestProfile.hpp"

#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace
{
namespace fs = std::filesystem;
using nx::core::UnitTest::ProcessTestProfile;

std::optional<std::string> environmentValue(const char* name)
{
  const char* value = std::getenv(name);
  return value == nullptr ? std::nullopt : std::optional<std::string>(value);
}

struct LateWitness
{
  LateWitness() = default;
  LateWitness(const LateWitness&) = default;
  LateWitness& operator=(const LateWitness&) = default;

  // Rvalues copy the path so the source keeps its shutdown check.
  LateWitness(LateWitness&& other)
  : LateWitness(static_cast<const LateWitness&>(other))
  {
  }

  LateWitness& operator=(LateWitness&& other)
  {
    return *this = static_cast<const LateWitness&>(other);
  }

  fs::path root;

  ~LateWitness() noexcept
  {
    if(root.empty())
    {
      return;
    }
    try
    {
      const char* home = std::getenv(ProcessTestProfile::homeVariable());
      if(fs::exists(root) || home == nullptr || fs::path(home) != root)
      {
        std::_Exit(86);
      }
      // This static observer uses retained routing without accessing Application.
      fs::create_directories(fs::path(home));
      std::ofstream stream(fs::path(home) / "late-witness.txt");
      stream << "isolated after removal";
      stream.close();
      if(!stream)
      {
        std::_Exit(87);
      }
    } catch(...)
    {
      std::_Exit(88);
    }
  }
};

LateWitness s_LateWitness;
} // namespace

int main(int argc, char* argv[])
{
  if(argc != 2)
  {
    return 2;
  }
  const std::string mode(argv[1]);
  const std::array<const char*, 7> names = {"HOME", "USERPROFILE", "LOCALAPPDATA", "APPDATA", "TEMP", "TMP", "TMPDIR"};
  std::array<std::optional<std::string>, 7> before;
  for(size_t index = 0; index < names.size(); ++index)
  {
    before[index] = environmentValue(names[index]);
  }
  ProcessTestProfile profile;
  std::string error;
  if(!profile.install(error))
  {
    std::cerr << error << '\n';
    return 3;
  }
  for(size_t index = 0; index < names.size(); ++index)
  {
    if(std::string(names[index]) != ProcessTestProfile::homeVariable() && environmentValue(names[index]) != before[index])
    {
      std::cerr << "Unrelated environment variable changed: " << names[index] << '\n';
      return 4;
    }
  }
  if(std::getenv("SIMPLNX_TEST_PROFILE_DIR") != nullptr || std::getenv("SIMPLNX_TEST_PROFILE_TOKEN") != nullptr || std::getenv("SIMPLNX_TEST_PROFILE_REPORT") != nullptr)
  {
    return 5;
  }
  const auto root = profile.root();
  if(mode == "unfinished")
  {
    return 9;
  }
  if(mode == "tamper" || mode == "status-256-failure")
  {
    std::ofstream(root / ".simplnx-test-profile") << "changed-token";
  }
  else if(mode == "nested-owner")
  {
    fs::create_directory(root / "independent");
    std::ofstream(root / "independent/.simplnx-test-profile") << "independent-owner";
  }
  else if(mode == "root-replaced")
  {
    fs::rename(root, root.string() + "-original");
    fs::create_directory(root);
    std::ofstream(root / "unrelated.txt") << "replacement must survive";
  }
  else if(mode == "late")
  {
    s_LateWitness.root = root;
  }
  else if(mode != "success" && mode != "failure" && mode != "status-256-success")
  {
    return 6;
  }
  if(mode == "status-256-failure" || mode == "status-256-success")
  {
    const int result = profile.finishExitCode(256, true, error);
    if(!error.empty())
    {
      std::cerr << error << '\n';
    }
    return result;
  }
  const bool finished = profile.finish(mode != "failure", error);
  const std::string firstError = error;
  if(profile.finish(true, error) != finished || error != firstError)
  {
    return 7;
  }
  if(!finished)
  {
    std::cerr << error << '\n';
    return 8;
  }
  if(profile.ownsRoot() && fs::exists(root))
  {
    return 10;
  }
  return 0;
}
