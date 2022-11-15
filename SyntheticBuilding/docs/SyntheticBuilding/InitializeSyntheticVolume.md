# InitializeSyntheticVolume

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | EstimateNumberOfFeatures | Estimate Number of Features | bool | BoolParameter |
| YES | GeometryDataContainer | Existing Geometry | DataPath | DataGroupSelectionParameter |
| YES | InputStatsArrayPath | Statistics | DataPath | ArraySelectionParameter |
| YES | InputPhaseTypesArrayPath | Phase Types | DataPath | ArraySelectionParameter |
| YES | DataContainerName | Synthetic Volume Data Container | DataPath | DataGroupCreationParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | GeometrySelection | Source of Geometry | ChoicesParameter::ValueType | ChoicesParameter |
| YES | Dimensions | Dimensions (Voxels) | VectorInt32Parameter::ValueType | VectorInt32Parameter |
| YES | Spacing | Spacing | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | Origin | Origin | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | LengthUnit | Length Units (For Description Only) | ChoicesParameter::ValueType | ChoicesParameter |
