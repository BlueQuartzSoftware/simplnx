#pragma once

#include "complex/complex_export.hpp"

#include <string>

namespace complex::Version
{

COMPLEX_EXPORT std::string ApplicationName();
COMPLEX_EXPORT std::string Complete();
COMPLEX_EXPORT std::string Major();
COMPLEX_EXPORT std::string Minor();
COMPLEX_EXPORT std::string Patch();
COMPLEX_EXPORT std::string Suffix();
COMPLEX_EXPORT std::string Package();
COMPLEX_EXPORT std::string PackageComplete();
COMPLEX_EXPORT std::string BuildDate();
COMPLEX_EXPORT std::string GitHashShort();
COMPLEX_EXPORT std::string GitHash();

class COMPLEX_EXPORT AppVersion
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
} // namespace complex::Version
