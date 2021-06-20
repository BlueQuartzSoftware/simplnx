#pragma once

#include <string>
#include <vector>

#include "complex/Filtering/Argument.hpp"
#include "complex/Filtering/Parameter.hpp"

#include "complex/complex_export.hpp"

namespace complex
{
class Argument;
class DataStructure;
class Parameter;

using Arguments = std::vector<Argument>;
using Parameters = std::vector<Parameter>;

class COMPLEX_EXPORT AbstractFilter
{
public:
  using IdType = size_t;

  AbstractFilter(const std::string& name, IdType id);
  virtual ~AbstractFilter();

  std::string getName() const;
  IdType getId() const;

  virtual Parameters parameters() const = 0;

  /**
   * @brief
   * @param ds
   * @param args
   * @return bool
   */
  bool preflight(const DataStructure& ds, const Arguments& args) const;

  /**
   * @brief
   * @param ds
   * @param args
   */
  void execute(DataStructure& ds, const Arguments& args) const;

protected:
  virtual bool preflightImpl(const DataStructure& ds, const Arguments& args) const = 0;
  virtual void executeImpl(DataStructure& ds, const Arguments& args) const = 0;

private:
  std::string m_Name;
  IdType m_Id;
};
} // namespace complex
