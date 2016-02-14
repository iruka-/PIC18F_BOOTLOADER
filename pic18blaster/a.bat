rem 
rem カレントDIRにある main-0000.hexをSTM8S-DのSTM32CPUに書き込んで自動的に終了する.
rem
rem 

openocd.exe -f blaster.cfg -f stm32.cfg -f batch.cfg

rem openocd.exe -f blaster.cfg -f lpc2378.cfg -f batch.cfg

rem pause