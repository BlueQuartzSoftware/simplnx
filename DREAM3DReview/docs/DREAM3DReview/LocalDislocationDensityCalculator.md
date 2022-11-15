# LocalDislocationDensityCalculator

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | CellSize | Cell Size (Microns) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | EdgeDataContainerName | Edge DataContainer | DataPath | DataGroupSelectionParameter |
| YES | BurgersVectorsArrayPath | Burgers Vectors Array | DataPath | ArraySelectionParameter |
| YES | SlipPlaneNormalsArrayPath | Slip Plane Normals Array | DataPath | ArraySelectionParameter |
| YES | OutputDataContainerName | Volume Data Container | DataPath | DataGroupCreationParameter |
| YES | OutputAttributeMatrixName | Cell AttributeMatrix | DataPath | ArrayCreationParameter |
| YES | OutputArrayName | Dislocation Line Density Array Name | DataPath | ArrayCreationParameter |
| YES | DominantSystemArrayName | Dominant System Array Name | DataPath | ArrayCreationParameter |
