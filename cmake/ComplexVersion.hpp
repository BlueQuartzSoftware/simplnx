#pragma once

#include <string>

namespace complex::Version
{

std::string ApplicationName();
std::string Complete();
std::string Major();
std::string Minor();
std::string Patch();
std::string Suffix();
std::string Package();
std::string PackageComplete();
std::string BuildDate();
std::string GitHashShort();
std::string GitHash();

class AppVersion
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
} // namespace DREAM3DNX::Version
