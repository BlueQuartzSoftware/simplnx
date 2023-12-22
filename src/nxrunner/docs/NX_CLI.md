# NX Runner (NX CLI)

NX Runner is an executable that allows preflighting and executing simplnx pipelines from the command line. The executable is created as part of the default simplnx CMake project and created alongside the simplnx library file.

In addition, NX Runner contains commands for porting SIMPL pipeline files to simplnx pipelines. One of these commands prints the converted pipeline to the terminal, and the other saves the converted pipeline to a new file using the simplnx pipeline extension.

## Commands

### Help

```bash
--help | -h
--help [command] [--logfile | -l]
-h [command] [--logfile | -l]
```

Lists available commands or displays details regarding a specific command.

### Execute

```bash
--execute <pipeline filepath> [--logfile | -l]
-e <pipeline filepath> [--logfile | -l]
```

Executes the pipeline at the target filepath while printing the output to the terminal. Optionally, a log file is created at the specified filepath where the output is saved.

For example, ```--execute bash D:/Directory/pipeline.d3pipeline -l D:/Logs/pipeline.log``` will attempt to execute the pipeline at `D:/Directory/pipeline.d3pipeline` and saves the output to `D:/Logs/pipeline.log`.

### Preflight

```bash
--preflight <pipeline filepath> [--logfile | -l]
-p <pipeline filepath> [--logfile | -l]
```

Preflights the pipeline at the target filepath while printing the output to the terminal. Optionally, a log file is created at the specified filepath where the output is saved.

For example, ```--preflight bash D:/Directory/pipeline.d3pipeline -l D:/Logs/pipeline.log``` will attempt to preflight the pipeline at `D:/Directory/pipeline.d3pipeline` and saves the output to `D:/Logs/pipeline.log`.

### Convert

```bash
--convert <pipeline filepath> [--logfile | -l]
-c <pipeline filepath> [--logfile | -l]

--convert-output <pipeline filepath> [--logfile | -l]
-co <pipeline filepath> [--logfile | -l]
```

Converts a SIMPL pipeline at the target filepath to a simplnx pipeline. If any errors are encountered during the process, they are printed to the console. Otherwise, the converted pipeline is printed to the console. Optionally, a log file is created at the specified filepath where the output is saved.

The second option (convert-output / co) also saves the converted pipeline to file based on the name of the converted pipeline using the simplnx pipeline extension (`.d3pipeline`).

For example, ```--convert-output bash D:/Directory/SIMPL.json``` will attempt to convert the SIMPL pipeline at `D:/Directory/SIMPL.json` and save the converted pipeline to `D:/Directory/SIMPL.d3pipeline`
