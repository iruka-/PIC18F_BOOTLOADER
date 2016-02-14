rem
rem Makefile が使えない場合のバッチ.
rem 


set C30=d:/browser/c30

pic30-gcc -mcpu=24FJ64GA002 -Os main.c -L%C30%/lib  -o main.cof -Wl,--script="%C30%\support\PIC24F\gld\p24FJ64GA002.gld",--defsym=__MPLAB_BUILD=1,--defsym=__MPLAB_DEBUG=1,--heap=128

pic30-objdump -S main.cof >main.lst

pic30-bin2hex main.cof

