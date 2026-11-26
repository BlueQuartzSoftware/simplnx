#pragma once

#include "simplnx/DataStructure/DataPath.hpp"
#include "simplnx/Filter/IFilter.hpp"

#include <vector>

namespace nx::core
{
class AttributeMatrix;

namespace CellDataAliasTest
{
inline const DataPath k_StrictGeometry({"Strict Geometry"});
inline const DataPath k_StrictCells = k_StrictGeometry.createChildPath("Cell Data");
inline const DataPath k_StrictIds = k_StrictCells.createChildPath("Feature Ids");
inline const DataPath k_OtherValues = k_StrictGeometry.createChildPath("Other Data").createChildPath("Other Values");

inline const DataPath k_AliasGeometry({"Alias Test Geometry"});
inline const DataPath k_CanonicalCells = k_AliasGeometry.createChildPath("Cells");
inline const DataPath k_AliasCells({"Alternate Parent", "Cells"});

AttributeMatrix& BuildStrictInput(DataStructure& dataStructure);
void CheckStrictPreflight(const IFilter::PreflightResult& result, int32 expectedCode);
void BuildAliasInput(DataStructure& dataStructure);
void CheckAliasPreflight(const IFilter::PreflightResult& result);
void CheckAliasOutput(const DataStructure& dataStructure, const std::vector<int32>& expectedIds);
void CheckFeatureDataWarning(const IFilter::PreflightResult& result);
} // namespace CellDataAliasTest
} // namespace nx::core
