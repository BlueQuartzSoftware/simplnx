#pragma once

#include "complex/Filtering/AbstractFilter.hpp"

//#include "test/testplugin_export.hpp"

class TestFilter : public complex::AbstractFilter
{
public:
  static const IdType ID;

  TestFilter();
  virtual ~TestFilter();

  /**
   * @brief
   * @return Parameters
   */
  complex::Parameters parameters() const override;

protected:
  bool preflightImpl(const complex::DataStructure& data, const complex::Arguments& args) const override;
  void executeImpl(complex::DataStructure& data, const complex::Arguments& args) const override;
};
