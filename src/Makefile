OBJ = main.o secure_input.o display_progress.o md5.o u3_commands.o u3_error.o
OBJ_SG = u3_scsi_sg.o
OBJ_USB = u3_scsi_usb.o
OBJ_DEBUG = u3_scsi_debug.o

CFLAGS += -g -Wall

.PHONY: all clean

all: u3_tool u3_tool_sg

u3_tool: $(OBJ) $(OBJ_USB)
	$(CC) -o u3_tool $(OBJ) $(OBJ_USB) $(LDFLAGS) -lusb 

u3_tool_sg: $(OBJ) $(OBJ_SG)
	$(CC) -o u3_tool_sg $(OBJ) $(OBJ_SG) $(LDFLAGS)

u3_tool_debug: $(OBJ) $(OBJ_DEBUG)
	$(CC) -o u3_tool_debug $(OBJ) $(OBJ_DEBUG) $(LDFLAGS)

main.o: main.c u3.h

secure_input.o: secure_input.c secure_input.h

display_progress.o: display_progress.c display_progress.h

md5.o: md5.c md5.h

u3_commands.o: u3_commands.c u3_commands.h u3.h

u3_error.o: u3_error.c u3_error.h u3.h

u3_scsi_sg.o: u3_scsi_sg.c u3_scsi.h sg_err.h u3.h

u3_scsi_usb.o: u3_scsi_usb.c u3_scsi.h u3.h

u3_scsi_debug.o: u3_scsi_debug.c u3_scsi.h u3.h

clean:
	rm -f u3_tool u3_tool_sg u3_tool_debug $(OBJ) $(OBJ_SG) $(OBJ_USB) $(OBJ_DEBUG)
