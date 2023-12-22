#pragma once

#include "simplnx/simplnx_export.hpp"

#include <string>

namespace nx::core::Version
{

SIMPLNX_EXPORT std::string ApplicationName();
SIMPLNX_EXPORT std::string Complete();
SIMPLNX_EXPORT std::string Major();
SIMPLNX_EXPORT std::string Minor();
SIMPLNX_EXPORT std::string Patch();
SIMPLNX_EXPORT std::string Suffix();
SIMPLNX_EXPORT std::string Package();
SIMPLNX_EXPORT std::string PackageComplete();
SIMPLNX_EXPORT std::string BuildDate();
SIMPLNX_EXPORT std::string GitHashShort();
SIMPLNX_EXPORT std::string GitHash();

class SIMPLNX_EXPORT AppVersion
{
public:
  AppVersion();
  AppVersion(int majorNum, int minorNum, int patchNum);

  AppVersion(const AppVersion&);
  AppVersion(const AppVersion&&) = delete;
  AppVersion& operator=(const AppVersion&);
  AppVersion& operator=(AppVersion&&) = delete;

  bool operator==(const AppVersion&);
  bool operator>(const AppVersion&);
  bool operator<(const AppVersion&);

  virtual ~AppVersion();

  int getMajorNum();
  int getMinorNum();
  int getPatchNum();

  void setMajorNum(int major);
  void setMinorNum(int minor);
  void setPatchNum(int patch);

private:
  int m_MajorNum = -1;
  int m_MinorNum = -1;
  int m_PatchNum = -1;
};
} // namespace nx::core::Version
