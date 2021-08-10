#pragma once

#include "complex/Filter/IFilter.hpp"

#include "test/test2plugin_export.hpp"

class TEST2PLUGIN_EXPORT Test2Filter : public complex::IFilter
{
public:
  static const std::string ID;

  Test2Filter();
  virtual ~Test2Filter();

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
  /**
   * @brief Filter-specifics for performing dataCheck.
   * @param data
   * @param args
   * @param messageHandler
   * @return DataCheckResult
   */
  DataCheckResult dataCheckImpl(const complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) const override;

  /**
   * @brief Filter-specifics for performing execute.
   * @param data
   * @param args
   * @param messageHandler
   * @return ExecuteResult
   */
  ExecuteResult executeImpl(complex::DataStructure& data, const complex::Arguments& args, const MessageHandler& messageHandler) override;
};
