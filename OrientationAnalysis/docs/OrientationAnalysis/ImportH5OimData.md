# ImportH5OimData #

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | InputFile | Input File | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| NO | SelectedScanNames | Scan Names | <<<NOT_IMPLEMENTED>>> | OEMEbsdScanSelectionFilterParameter |
| YES | ZSpacing | Z Spacing (Microns) | float64 | Float64Parameter |
| YES | Origin | Origin (XYZ) | VectorFloat32Parameter::ValueType | VectorFloat32Parameter |
| YES | ReadPatternData | Import Pattern Data | bool | BoolParameter |
| YES | DataContainerName | Data Container | DataPath | DataGroupCreationParameter |
| YES | CellAttributeMatrixName | Cell Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | CellEnsembleAttributeMatrixName | Cell Ensemble Attribute Matrix | DataPath | ArrayCreationParameter |
