# Makefile for alsa test routine

SRC = client.cpp server.cpp main.cpp
CC = g++
BIN = testbandwidth
#INC_PATH =-I${PWD}/out/include
#LD_PATH  =-L${PWD}/out/lib


#SRC0 = sound_test1.c
#BIN0 = sound_test1
#SRC1 = newtest_spdif.c
#BIN1 = newtest_spdif
#SRC2 = newtest_iis.c
#SRC2 += audio_render_alsa.c
#BIN2 = newtest_iis
#CC = arm-none-linux-gnueabi-gcc
#CC = /opt/arm-2009q1/bin/arm-none-linux-gnueabi-gcc
#CC = /opt/tuxbuilder-1.0/mipsel-unknown-linux-gnu/cross/bin/mipsel-unknown-linux-gnu-gcc
#LIB = ${PWD}/out/lib/libsalsa.a

INC_PATH =-I${PWD}/out/include
LD_PATH  =-L${PWD}/out/lib
INSTALL_PATH  =${PWD}/out/bin

all:
	$(CC) $(SRC) -o $(BIN)	
clean:
	rm -f $(BIN)