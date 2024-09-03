Body Basher flashing requirements:
- Have the STM32 Programmer Command Line Interface installed (STM32_Programmer_CLI.exe)
- Edit the flash.bat accordingly to run the STM32_Programmer_CLI.exe (not necessary to use the batch file, possible to run the commands manually)
Procedure:
- Reset or power cycle the Body Basher while the BOOT pins are shorted
- Run the flash.bat
- Reset or power cycle the Body Basher again
Notes:
- It is possible to use the STM32 Programmer GUI, however it is less stable than the CLI
- Due to problems with the STM drivers, the flashing does not succeed every time, rerun the batch file in case of a fail
- Tip: run the flash.bat from the terminal so the output can be examined in case of a failure

