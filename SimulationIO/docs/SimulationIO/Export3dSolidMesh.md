# Export3dSolidMesh

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | MeshingPackage | Meshing package | ChoicesParameter::ValueType | ChoicesParameter |
| YES | outputPath | Path | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | PackageLocation | Package Location | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | NetgenSTLFileName | STL File Prefix | StringParameter::ValueType | StringParameter |
| YES | GmshSTLFileName | STL File Prefix | StringParameter::ValueType | StringParameter |
| YES | SurfaceMeshFaceLabelsArrayPath | Face Labels | DataPath | ArraySelectionParameter |
| YES | FeatureEulerAnglesArrayPath | Euler Angles | DataPath | ArraySelectionParameter |
| YES | FeaturePhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
| YES | FeatureCentroidArrayPath | Feature Centroids | DataPath | ArraySelectionParameter |
| YES | MeshFileFormat | Mesh File Format | ChoicesParameter::ValueType | ChoicesParameter |
| YES | RefineMesh | Refine Mesh (q) | bool | BoolParameter |
| YES | MaxRadiusEdgeRatio | Maximum Radius-Edge Ratio | float32 | Float32Parameter |
| YES | MinDihedralAngle | Minimum Dihedral Angle | float32 | Float32Parameter |
| YES | OptimizationLevel | Optimization Level (O) | int32 | Int32Parameter |
| YES | MeshSize | Mesh Size | ChoicesParameter::ValueType | ChoicesParameter |
| YES | LimitTetrahedraVolume | Limit Tetrahedra Volume (a) | bool | BoolParameter |
| YES | MaxTetrahedraVolume | Maximum Tetrahedron Volume | float32 | Float32Parameter |
| YES | IncludeHolesUsingPhaseID | Include Holes Using PhaseID | bool | BoolParameter |
| YES | PhaseID | PhaseID | int32 | Int32Parameter |
| YES | TetDataContainerName | Data Container Name | StringParameter::ValueType | StringParameter |
| YES | VertexAttributeMatrixName | Vertex Attribute Matrix Name | StringParameter::ValueType | StringParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix Name | StringParameter::ValueType | StringParameter |
