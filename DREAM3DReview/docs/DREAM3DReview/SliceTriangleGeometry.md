# SliceTriangleGeometry

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | SliceDirection | Slice Direction (ijk) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | SliceRange | Slice Range | ChoicesParameter::ValueType | ChoicesParameter |
| YES | Zstart | Slicing Start | float32 | Float32Parameter |
| YES | Zend | Slicing End | float32 | Float32Parameter |
| YES | SliceResolution | Slice Spacing | float32 | Float32Parameter |
| YES | HaveRegionIds | Have Region Ids | bool | BoolParameter |
| YES | CADDataContainerName | CAD Geometry | DataPath | DataGroupSelectionParameter |
| YES | RegionIdArrayPath | Region Ids | DataPath | ArraySelectionParameter |
| YES | SliceDataContainerName | Slice Geometry | StringParameter::ValueType | StringParameter |
| YES | EdgeAttributeMatrixName | Edge Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | SliceIdArrayName | Slice Ids | DataPath | ArrayCreationParameter |
| YES | SliceAttributeMatrixName | Slice Attribute Matrix | DataPath | ArrayCreationParameter |
