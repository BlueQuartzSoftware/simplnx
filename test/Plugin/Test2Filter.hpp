#pragma once

#include "complex/Filtering/AbstractFilter.hpp"

//#include "test/test2plugin_export.hpp"

class Test2Filter : public complex::AbstractFilter
{
public:
  static const IdType ID;

  Test2Filter();
  virtual ~Test2Filter();

  /**
   * @brief
   * @return Parameters
   */
  complex::Parameters parameters() const override;

protected:
  bool preflightImpl(const complex::DataStructure& data, const complex::Arguments& args) const override;
  void executeImpl(complex::DataStructure& data, const complex::Arguments& args) const override;
};
