#ifndef OLESS_H
#define OLESS_H

#include "oless/oless_common.hpp"
#include "oless/pole.h"

#include <list>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

namespace OleStructuredStorage
{

class Oless
{
private:
  char* m_file;
  std::vector<IExportable*> m_results;

  void printStreamInfo(POLE::Storage*, std::string, std::string);
  void recurse(POLE::Storage*, const std::string, void (Oless::*)(POLE::Storage*, std::string, std::string));

public:
  Oless() {};
  Oless(char*);

  std::vector<OleSummary*> List();
  void Dump(char*, std::string);
  bool IsOless();
};
} // namespace OleStructuredStorage
#endif
