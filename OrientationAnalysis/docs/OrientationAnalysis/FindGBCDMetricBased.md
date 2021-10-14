# FindGBCDMetricBased #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | PhaseOfInterest | Phase of Interest | int32 | Int32Parameter |
| YES | MisorientationRotation | Fixed Misorientation | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ChosenLimitDists | Limiting Distances | ChoicesParameter::ValueType | ChoicesParameter |
| YES | NumSamplPts | Number of Sampling Points (on a Hemisphere) | int32 | Int32Parameter |
| YES | ExcludeTripleLines | Exclude Triangles Directly Neighboring Triple Lines | bool | BoolParameter |
| YES | DistOutputFile | Output Distribution File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | ErrOutputFile | Output Distribution Errors File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | SaveRelativeErr | Save Relative Errors Instead of Their Absolute Values | bool | BoolParameter |
| YES | NodeTypesArrayPath | Node Types | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshFaceLabelsArrayPath | Face Labels | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshFaceNormalsArrayPath | Face Normals | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshFaceAreasArrayPath | Face Areas | DataPath | ArraySelectionParameter |
| YES | SurfaceMeshFeatureFaceLabelsArrayPath | Feature Face Labels | DataPath | ArraySelectionParameter |
| YES | FeatureEulerAnglesArrayPath | Average Euler Angles | DataPath | ArraySelectionParameter |
| YES | FeaturePhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
| YES | CrystalStructuresArrayPath | Crystal Structures | DataPath | ArraySelectionParameter |
