#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/IFilter.hpp"
#include "simplnx/Parameters/ArraySelectionParameter.hpp"
#include "simplnx/Parameters/FileSystemPathParameter.hpp"
#include "simplnx/Parameters/NumberParameter.hpp"
#include "simplnx/Parameters/StringParameter.hpp"

namespace fs = std::filesystem;

namespace nx::core
{

struct SIMPLNXCORE_EXPORT CombineNodeBasedGeometriesInputValues
{
  std::vector<DataPath> InputGeometryPaths;
  DataPath OutputGeometryPath;
};

/**
 * @class
 */
class SIMPLNXCORE_EXPORT CombineNodeBasedGeometries
{
public:
  CombineNodeBasedGeometries(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, CombineNodeBasedGeometriesInputValues* inputValues);
  ~CombineNodeBasedGeometries() noexcept;

  CombineNodeBasedGeometries(const CombineNodeBasedGeometries&) = delete;
  CombineNodeBasedGeometries(CombineNodeBasedGeometries&&) noexcept = delete;
  CombineNodeBasedGeometries& operator=(const CombineNodeBasedGeometries&) = delete;
  CombineNodeBasedGeometries& operator=(CombineNodeBasedGeometries&&) noexcept = delete;

  enum class ErrorCodes : int64
  {
    FewerThanTwoPathsChosen = -455,
    ObjectNotAGeometry = -456,
    ObjectNotANodeGeometry = -457,
    NodeGeometryHasNoVertices = -458,
    DifferingGeometryTypes = -459,
    InconsistentGeometryElements = -460,
    UnsupportedGeometryType = -461
  };

  Result<> operator()();

  const std::atomic_bool& getCancel();

  void sendMessage(const std::string& message);

private:
  DataStructure& m_DataStructure;
  const CombineNodeBasedGeometriesInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace nx::core
