#pragma once

#include "complex/Filter/IFilter.hpp"

#include "test/testplugin_export.hpp"

class TESTPLUGIN_EXPORT TestFilter : public complex::IFilter
{
public:
  static const std::string ID;

  TestFilter();
  virtual ~TestFilter();

  /**
   * @brief Returns the name of the filter.
   * @return std::string
   */
  [[nodiscard]] std::string name() const override;

  /**
   * @brief Returns the filters ID as a std::string.
   * @return std::string
   */
  [[nodiscard]] std::string uuid() const override;

  /**
   * @brief Returns the filter's human label.
   * @return std::string
   */
  [[nodiscard]] std::string humanName() const override;

  /**
   * @brief Returns a collection of parameters used.
   * @return Parameters
   */
  [[nodiscard]] complex::Parameters parameters() const override;

protected:
  DataCheckResult dataCheckImpl(const complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) const override;
  ExecuteResult executeImpl(complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) override;
};
