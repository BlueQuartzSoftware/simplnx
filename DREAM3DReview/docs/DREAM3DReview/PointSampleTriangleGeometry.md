# PointSampleTriangleGeometry

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | SamplesNumberType | Source for Number of Samples | ChoicesParameter::ValueType | ChoicesParameter |
| YES | NumberOfSamples | Number of Sample Points | int32 | Int32Parameter |
| YES | UseMask | Use Mask | bool | BoolParameter |
| YES | TriangleGeometry | Triangle Geometry to Sample | DataPath | DataGroupSelectionParameter |
| YES | ParentGeometry | Source Geometry for Number of Sample Points | DataPath | DataGroupSelectionParameter |
| YES | TriangleAreasArrayPath | Face Areas | DataPath | ArraySelectionParameter |
| YES | MaskArrayPath | Mask | DataPath | ArraySelectionParameter |
| YES | SelectedDataArrayPaths | Attribute Arrays to Transfer | MultiArraySelectionParameter::ValueType | MultiArraySelectionParameter |
| YES | VertexGeometry | Vertex Geometry | StringParameter::ValueType | StringParameter |
| YES | VertexAttributeMatrixName | Vertex Attribute Matrix | DataPath | ArrayCreationParameter |
