#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"

#include "simplnx/Common/Result.hpp"
#include "simplnx/DataStructure/DataStructure.hpp"
#include "simplnx/Filter/Arguments.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <filesystem>

namespace fs = std::filesystem;

namespace nx::core
{

struct SIMPLNXCORE_EXPORT ReadStlFileInputValues
{
  fs::path stlFilePath;
  DataPath geometryPath;
  DataPath faceGroupPath;
  DataPath faceNormalsDataPath;
  bool scaleOutput = false;
  float32 scaleFactor = 1.0F;
};

/**
 * @class ReadStlFile
 * @brief Reads a binary STL mesh file into a TriangleGeom.
 *
 * The algorithm supports Magics color data and VxElements metadata. It merges duplicate vertices after it reads the mesh.
 */
class SIMPLNXCORE_EXPORT ReadStlFile
{
public:
  ReadStlFile(DataStructure& dataStructure, const IFilter::MessageHandler& mesgHandler, const std::atomic_bool& shouldCancel, ReadStlFileInputValues* inputValues);
  ~ReadStlFile() noexcept;

  ReadStlFile(const ReadStlFile&) = delete;
  ReadStlFile(ReadStlFile&&) noexcept = delete;
  ReadStlFile& operator=(const ReadStlFile&) = delete;
  ReadStlFile& operator=(ReadStlFile&&) noexcept = delete;

  /**
   * @brief Reads triangles and eliminates duplicate nodes.
   * @return File, parse, or node-elimination error, or success after cancellation.
   *
   * Cancellation and parse errors can retain partially written mesh arrays.
   * Per-value DataStore writes do not report I/O errors.
   */
  Result<> operator()();

private:
  DataStructure& m_DataStructure;
  const ReadStlFileInputValues* m_InputValues = nullptr;
  const std::atomic_bool& m_ShouldCancel;
  const IFilter::MessageHandler& m_MessageHandler;
};
} // namespace nx::core
