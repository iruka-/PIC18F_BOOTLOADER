rem
rem 800番地オリジンのPICmonをpicbootで書き込んで実行する.
rem (実行前に RC2 の BOOT Jumperを外しておいてください)
rem 

picboot %1 %2 %3 -r loader-18F14K50.hex
