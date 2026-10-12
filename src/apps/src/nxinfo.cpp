/**
 * @file nxinfo.cpp
 * @brief Command-line tool that reports information about simplnx-related files.
 */

#include "NxInfo.hpp"

#include <cstddef>
#include <cstdlib>
#include <exception>
#include <iostream>
#include <span>
#include <string>
#include <vector>

/**
 * @brief Runs the nxinfo command-line application.
 * @param argc Number of command-line arguments.
 * @param argv Command-line argument array.
 * @return Zero on success or a documented nxinfo process status.
 */
int main(int argc, char* argv[]) noexcept
{
  try
  {
    const std::span<char*> rawArguments(argv, static_cast<std::size_t>(argc));
    std::vector<std::string> arguments;
    arguments.reserve(!rawArguments.empty() ? rawArguments.size() - 1 : 0);
    for(const char* argumentPtr : rawArguments.subspan(rawArguments.empty() ? 0 : 1))
    {
      arguments.emplace_back(argumentPtr);
    }
    return nx::core::nxinfo::Run(arguments, {.Output = std::cout, .Error = std::cerr});
  } catch(const std::exception& error)
  {
    std::cerr << "Fatal error: nxinfo failed unexpectedly: " << error.what() << '\n';
  } catch(...)
  {
    std::cerr << "Fatal error: nxinfo failed because of an unknown exception.\n";
  }
  return EXIT_FAILURE;
}
