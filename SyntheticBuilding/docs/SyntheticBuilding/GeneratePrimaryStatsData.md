# GeneratePrimaryStatsData

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | PhaseName | Phase Name | StringParameter::ValueType | StringParameter |
| YES | CrystalSymmetry | Crystal Symmetry | ChoicesParameter::ValueType | ChoicesParameter |
| YES | MicroPresetModel | Microstructure Preset Model | ChoicesParameter::ValueType | ChoicesParameter |
| YES | PhaseFraction | Phase Fraction | float64 | Float64Parameter |
| YES | Mu | Mu | float64 | Float64Parameter |
| YES | Sigma | Sigma | float64 | Float64Parameter |
| YES | MinCutOff | Min.Cut Off | float64 | Float64Parameter |
| YES | MaxCutOff | Max Cut Off | float64 | Float64Parameter |
| YES | BinStepSize | Bin Step Size | float64 | Float64Parameter |
| NO | OdfData | ODF | <<<NOT_IMPLEMENTED>>> | DynamicTableFilterParameter |
| NO | MdfData | MDF | <<<NOT_IMPLEMENTED>>> | DynamicTableFilterParameter |
| NO | AxisOdfData | Axis ODF | <<<NOT_IMPLEMENTED>>> | DynamicTableFilterParameter |
| YES | CreateEnsembleAttributeMatrix | Create Data Container & Ensemble AttributeMatrix | bool | BoolParameter |
| YES | DataContainerName | Data Container | DataPath | DataGroupCreationParameter |
| YES | CellEnsembleAttributeMatrixName | Cell Ensemble Attribute Matrix | DataPath | ArrayCreationParameter |
| YES | AppendToExistingAttributeMatrix | Append To Existing AttributeMatrix | bool | BoolParameter |
| YES | SelectedEnsembleAttributeMatrix | Selected Ensemble AttributeMatrix | DataPath | DataGroupSelectionParameter |
