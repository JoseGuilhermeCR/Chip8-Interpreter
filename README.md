# Chip8-Interpreter
Interpretador CHIP-8 escrito em C.

## Sobre

Um interpretador para o Chip 8 original que conta com 35 instruções. Projeto feito por diversão e para aprendizado.

Nem todas os programas que encontrei rodam nesse interpretador, porém não sei dizer os programas que apresentavam falha eram para a versão original do Chip 8. O input ainda não está do jeito que eu gostaria, principalmente a instrução "ld_vx_k/Fx0A". Além disso, o interpretador ainda não toca som quando necessário. De resto, programas não interativos aparentam rodar sem maiores problemas.

Tentei escrevê-lo de uma maneira que seja possível usar o interpretador sem que seja necessário usar a interface feita por mim. Para isso, só seriam necessários os arquivos chip8.c e chip8.h. A maneira como o interpretador se comporta, no entanto, está quase toda em chip8_interpreter.h e chip8_interpreter.c.

## Compilar

Esse projeto usa o SDL2. Para que o executável funcione no Windows, é necessário ter SDL2.dll na mesma pasta.
Para compilar, no Windows usando GCC(MinGW):
```
gcc chip8_interpreter.c chip8.c -Wall -pedantic-errors -I<include_sdl2> -LC:<lib_sdl2> -w -lmingw32 -lSDL2main -lSDL2 -o chip8_interpreter.exe
```
Onde <include_sdl2> e <lib_sdl2> são os diretórios do SDL2 de 32 bits (por razões de compatibilidade) localizados em i686-w64-mingw32 após baixar e descompactar a biblioteca.

No Linux, após instalar o SDL2, de preferência pelo repositório de sua distribuição, basta fazer o link com a biblioteca na hora de compilar.
Exemplo do comando em Ubuntu:
```
gcc chip8_interpreter.c chip8.c -Wall -pedantic-errors -lSDL2 -o chip8_interpreter
```

## Para fazer esse interpretador usei como referências:

* [Alguns programas e documentação por mattmikolay.](https://github.com/mattmikolay/chip-8)

* [Uma coleção grande de programas por dmatlack.](https://github.com/dmatlack/chip8/tree/master/roms)

* [Documentação geral, mais especificamente como as instruções funcionam.](http://devernay.free.fr/hacks/chip8/C8TECH10.HTM#Fx0A)

* [Um interpretador Chip 8 feito em javascript. Para uma ideia geral de como organizar o código por taniarascia.](https://github.com/taniarascia/chip8)

* [Mais documentação também por mattmikolay.](http://mattmik.com/files/chip8/mastering/chip8.html)
