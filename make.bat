gbdk\bin\lcc -Wa-l -Wf-bo2 -c -o  tiles00.o tiles/linustiles.c
gbdk\bin\lcc -Wa-l -Wf-bo3 -c -o  map01.o map/map1.c
gbdk\bin\lcc -Wa-l -Wf-bo3 -c -o  map02.o map/map2.c
gbdk\bin\lcc -Wa-l -Wf-bo3 -c -o  map03.o map/map3.c
gbdk\bin\lcc -Wa-l -Wf-bo3 -c -o  map04.o map/map4.c
gbdk\bin\lcc -Wa-l -Wf-bo3 -c -o  map05.o map/map5.c
gbdk\bin\lcc -Wa-l -Wf-bo4 -c -o  map06.o map/map6.c
gbdk\bin\lcc -Wa-l -Wf-bo4 -c -o  map07.o map/map7.c
gbdk\bin\lcc -Wa-l -Wf-bo4 -c -o  map08.o map/map8.c
gbdk\bin\lcc -Wa-l -Wf-bo4 -c -o  map09.o map/map9.c
gbdk\bin\lcc -Wa-l -Wf-bo2 -c -o  char00.o character/char.c
gbdk\bin\lcc -Wa-l -Wf-bo2 -c -o  menu00.o menu/menu.c
gbdk\bin\lcc -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -c -o song.o music/song.c
gbdk\bin\lcc -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -c -o gbt_player.o music/gbt_player.s
gbdk\bin\lcc -Wa-l -Wl-m -Wl-j -DUSE_SFR_FOR_REG -c -o gbt_player_bank1.o music/gbt_player_bank1.s
gbdk\bin\lcc -Wa-l -c -o  main.o main.c 
gbdk\bin\lcc -Wl-yt3 -Wl-yo8 -Wl-ya4 -o sternstein.gb main.o map01.o map02.o map03.o map04.o map05.o map06.o map07.o map08.o map09.o tiles00.o char00.o menu00.o song.o gbt_player.o gbt_player_bank1.o
pause

del *.o
del *.lst

bgb\bgb.exe sternstein.gb