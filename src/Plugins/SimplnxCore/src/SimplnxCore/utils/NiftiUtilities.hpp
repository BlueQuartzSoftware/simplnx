#pragma once

#include "SimplnxCore/SimplnxCore_export.hpp"
#include "SimplnxCore/utils/nifti1.h"

#include "simplnx/Common/Result.hpp"
#include "simplnx/Common/Types.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>

namespace nx::core::nifti
{
inline constexpr usize k_HeaderSize = 348;
inline constexpr usize k_MinVoxOffset = 352;
inline constexpr std::array<char, 4> k_SingleFileMagic = {'n', '+', '1', '\0'};
inline constexpr std::array<char, 4> k_PairFileMagic = {'n', 'i', '1', '\0'};

struct SIMPLNXCORE_EXPORT NiftiTypeInfo
{
  DataType dataType{DataType::uint8};
  usize componentCount{1};
  int16 bitpix{0};
};

/**
 * @brief Maps a NIfTI datatype code (NIFTI_TYPE_*) to a simplnx DataType plus
 * component count. Returns std::nullopt for codes not supported by this filter
 * (complex*, float128, binary, etc).
 */
std::optional<NiftiTypeInfo> SIMPLNXCORE_EXPORT NiftiDatatypeToSimplnx(int16 niftiDatatype);

struct SIMPLNXCORE_EXPORT NiftiMetadata
{
  // Raw header (kept for diagnostics and reporting)
  std::string magic;
  std::array<int16, 8> dim{};
  std::array<float32, 8> pixdim{};

  // Canonicalized values used to build the ImageGeom
  std::array<usize, 3> dimensions{0, 0, 0};
  std::array<float32, 3> spacing{1.0f, 1.0f, 1.0f};
  std::array<float32, 3> origin{0.0f, 0.0f, 0.0f};

  // Datatype
  int16 niftiDatatype{0};
  int16 bitpix{0};
  DataType dataType{DataType::uint8};
  usize componentCount{1};

  // Data placement + byte order
  usize voxOffset{k_MinVoxOffset};
  bool byteSwapRequired{false};

  // Optional data scaling (y = slope * x + inter)
  float32 sclSlope{0.0f};
  float32 sclInter{0.0f};
  bool hasNontrivialScaling{false};

  // Orientation
  int16 qformCode{0};
  int16 sformCode{0};
  std::array<std::array<float32, 4>, 3> sformMatrix{};
  bool affineHasRotation{false};

  // Other
  int16 intentCode{0};
  std::string intentName;
  std::string descrip;
  std::string filePath;
};

/**
 * @brief Reads and validates the 348-byte NIfTI-1 header from a .nii or .nii.gz
 * file, returning a NiftiMetadata populated with canonicalized geometry,
 * datatype, scaling, and orientation information.
 *
 * Rejects: .hdr/.img pair format, non-3D files, unsupported datatype codes.
 */
Result<NiftiMetadata> SIMPLNXCORE_EXPORT ReadNiftiHeader(const std::filesystem::path& filePath, bool useAffineIfPresent);

} // namespace nx::core::nifti
