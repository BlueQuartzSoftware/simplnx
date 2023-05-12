#pragma once

#include "ComplexCore/ComplexCore_export.hpp"

#include "complex/DataStructure/DataPath.hpp"
#include "complex/DataStructure/DataStructure.hpp"
#include "complex/Filter/IFilter.hpp"
#include "complex/Parameters/FileSystemPathParameter.hpp"
#include "complex/Parameters/MultiArraySelectionParameter.hpp"

namespace complex
{
class ImageGeom;

struct COMPLEXCORE_EXPORT VtkRectilinearGridWriterInputValues
{
  FileSystemPathParameter::ValueType OutputFile;
  bool WriteBinaryFile;
  DataPath ImageGeometryPath;
  MultiArraySelectionParameter::ValueType SelectedDataArrayPaths;
};

/**
 * @class VtkRectilinearGridWriter
 * @brief This filter ...
 */

class COMPLEXCORE_EXPORT VtkRectilinearGridWriter
{
public:
  VtkRectilinearGridWriter(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, VtkRectilinearGridWriterInputValues* inputValues);
  ~VtkRectilinearGridWriter() noexcept;

  VtkRectilinearGridWriter(const VtkRectilinearGridWriter&) = delete;
  VtkRectilinearGridWriter(VtkRectilinearGridWriter&&) noexcept = delete;
  VtkRectilinearGridWriter& operator=(const VtkRectilinearGridWriter&) = delete;
  VtkRectilinearGridWriter& operator=(VtkRectilinearGridWriter&&) noexcept = delete;

  Result<> operator()();

  const std::atomic_bool& getCancel();

private:
  DataStructure& m_DataStructure;
  const VtkRectilinearGridWriterInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};

} // namespace complex
