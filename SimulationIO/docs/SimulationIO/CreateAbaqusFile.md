# CreateAbaqusFile

| Ready | Parameter Key | Human Name | Parameter Type | Parameter Class |
|-------|---------------|------------|-----------------|----------------|
| YES | OutputPath | Output Path  | FileSystemPathParameter::ValueType | FileSystemPathParameter |
| YES | OutputFilePrefix | Output File Prefix | StringParameter::ValueType | StringParameter |
| YES | JobName | Job Name | StringParameter::ValueType | StringParameter |
| YES | NumDepvar | Number of Solution Dependent State Variables | int32 | Int32Parameter |
| YES | NumUserOutVar | Number of User Output Variables | int32 | Int32Parameter |
| NO | MatConst | Material Constants | <<<NOT_IMPLEMENTED>>> | DynamicTableFilterParameter |
| YES | AbqFeatureIdsArrayPath | Feature Ids | DataPath | ArraySelectionParameter |
| YES | CellEulerAnglesArrayPath | Euler Angles | DataPath | ArraySelectionParameter |
| YES | CellPhasesArrayPath | Phases | DataPath | ArraySelectionParameter |
