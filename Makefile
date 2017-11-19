build:echo echos

	gcc -o echo echo.c
	gcc -o echos echos.c
    
clean:

	-rm -f echo echos
