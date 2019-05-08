SRC := encoder_h264.c
OBJ := $(SRC:.o=.c)
INCLUDE :=  -I /home/share/lib_so/ffmpeg4.1/include
LIB := -L /home/share/lib_so/ffmpeg4.1/lib -lavcodec -lavutil -lavformat -lavfilter
CFLAGS:=

demo:$(OBJ)
	gcc -o $@  $< $(INCLUDE) $(LIB) $(CFLAGS)

.PHONY:clean
clean:
	-rm -rf demo *.o *.h264

