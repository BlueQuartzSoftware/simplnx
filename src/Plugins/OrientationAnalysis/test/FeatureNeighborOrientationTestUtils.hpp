#pragma once

#include "simplnx/DataStructure/DataArray.hpp"
#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/DataStructure/NeighborList.hpp"

#include <array>

namespace nx::core::UnitTest::FeatureNeighbors
{
struct InputPaths
{
  InputPaths(const DataPath& featureMatrix, const DataPath& ensembleMatrix);

  DataPath featurePhases;
  DataPath avgQuats;
  DataPath neighborList;
  DataPath crystalStructures;
};

/** @brief Initializes unassigned features with identity quaternions and empty neighbor lists. */
void InitializeFeatures(Int32Array& phases, Float32Array& quats, NeighborList<int32>& neighbors);

// A four-component float32 quaternion occupies 16 bytes, giving 65,536 tuples per 1 MiB HDF5 chunk.
inline constexpr usize k_TailTuple = 65536;
inline constexpr usize k_LastFullChunkTuple = k_TailTuple - 1;

/** @brief Gives each boundary feature a single neighbor with identity orientation and verifies the HDF5 input stores. */
void CreateChunkTailInputs(DataStructure& dataStructure, const InputPaths& paths, uint32 crystalStructure, const std::array<float32, 4>& lastFullChunkQuat, const std::array<float32, 4>& tailQuat);
} // namespace nx::core::UnitTest::FeatureNeighbors
