# Copyright 2020-2022 Laurent Cabaret
# Copyright 2020-2022 Vincent Jacques

program-1-no-chrones: program-1.cpp Makefile
	g++ -std=gnu++11 program-1.cpp -I`chrones instrument c++ header-location` -DCHRONES_DISABLED -o program-1-no-chrones

program-1-yes-chrones: program-1.cpp Makefile
	g++ -std=gnu++11 program-1.cpp -I`chrones instrument c++ header-location` -o program-1-yes-chrones
