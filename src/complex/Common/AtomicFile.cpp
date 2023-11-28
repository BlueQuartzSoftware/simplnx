#include "AtomicFile.hpp"

#include "complex/Common/Types.hpp"

#include <fmt/format.h>

#include <chrono>
#include <random>
#include <utility>

using namespace complex;

namespace
{
const std::string k_AlphaNum = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
}

AtomicFile::AtomicFile(const std::string& filename, bool autoCommit)
: m_FilePath(fs::path(filename))
, m_AutoCommit(autoCommit)
{
  const usize length = 16;

  std::string randomExtension(length, '-');

  auto seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  std::mt19937_64 generator;
  generator.seed(seed);
  auto distribution = std::uniform_int_distribution<uint8>(0, 62);

  for(usize i = 0; i < length; ++i)
  {
    randomExtension[i] = k_AlphaNum[distribution(generator)];
  }

  m_TempFilePath = fmt::format("{}/{}{}", m_FilePath.parent_path().string(), m_FilePath.stem().string() + randomExtension, m_FilePath.extension().string());
}

AtomicFile::AtomicFile(fs::path&& filepath, bool autoCommit)
: m_FilePath(std::move(filepath))
, m_AutoCommit(autoCommit)
{
  const usize length = 16;

  std::string randomExtension(length, '-');

  auto seed = static_cast<std::mt19937_64::result_type>(std::chrono::steady_clock::now().time_since_epoch().count());
  std::mt19937_64 generator;
  generator.seed(seed);
  auto distribution = std::uniform_int_distribution<uint8>(0, 62);

  for(usize i = 0; i < length; ++i)
  {
    randomExtension[i] = k_AlphaNum[distribution(generator)];
  }

  m_TempFilePath = fmt::format("{}/{}{}", m_FilePath.parent_path().string(), m_FilePath.stem().string() + randomExtension, m_FilePath.extension().string());
}

AtomicFile::~AtomicFile()
{
  if(m_AutoCommit)
  {
    commit();
  }
  if(fs::exists(m_TempFilePath))
  {
    removeTempFile();
  }
}

fs::path AtomicFile::tempFilePath() const
{
  return m_TempFilePath;
}

void AtomicFile::commit()
{
  fs::rename(m_TempFilePath, m_FilePath);
}

void AtomicFile::setAutoCommit(bool value)
{
  m_AutoCommit = value;
}

bool AtomicFile::getAutoCommit() const
{
  return m_AutoCommit;
}

void AtomicFile::removeTempFile()
{
  fs::remove(m_TempFilePath);
}
