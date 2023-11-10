# NX Runner (NX CLI)

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

Execute the pipeline at the target filepath. Optionally, create a log file at the specified path.

### Preflight

```bash
--preflight <pipeline filepath> [--logfile | -l]
-p <pipeline filepath> [--logfile | -l]
```

Preflight the pipeline at the target filepath. Optionally, create a log file at the specified path.

### Convert

```bash
--convert <pipeline filepath> [--logfile | -l]
-c <pipeline filepath> [--logfile | -l]

--convert-output <pipeline filepath> [--logfile | -l]
-co <pipeline filepath> [--logfile | -l]
```

Convert the SIMPL pipeline at the target filepath. Optionally, create a log file at the specified path.
Both options print the converted pipeline to the console. The second option (convert-output / co) also saves the converted pipeline to file based on the name of the converted pipeline.

For example, ```--convert-output bash D:/Directory/SIMPL.json``` will attempt to convert the SIMPL pipeline at `D:/Directory/SIMPL.json` and save the converted pipeline to `D:/Directory/SIMPL.d3pipeline`
