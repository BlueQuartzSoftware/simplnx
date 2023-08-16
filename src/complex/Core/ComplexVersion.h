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

class LibraryVersion
{
public:
  LibraryVersion();
  LibraryVersion(int majorNum, int minorNum, int patchNum);

  LibraryVersion(const LibraryVersion&);
  LibraryVersion(const LibraryVersion&&) = delete;
  LibraryVersion& operator=(const LibraryVersion&);
  LibraryVersion& operator=(LibraryVersion&&) = delete;

  bool operator==(const LibraryVersion&);
  bool operator>(const LibraryVersion&);
  bool operator<(const LibraryVersion&);

  virtual ~LibraryVersion();

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
