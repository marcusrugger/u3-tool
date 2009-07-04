bin_PROGRAMS = u3_tool u3_tool_usb

u3_tool_SOURCES = display_progress.c display_progress.h main.c md5.c md5.h \
	secure_input.c secure_input.h u3_commands.c u3_commands.h u3_error.c \
	u3_error.h u3.h u3_scsi.h u3_scsi_sg.c sg_err.h

u3_tool_usb_SOURCES = display_progress.c display_progress.h main.c md5.c md5.h \
	secure_input.c secure_input.h u3_commands.c u3_commands.h u3_error.c \
	u3_error.h u3.h u3_scsi.h u3_scsi_usb.c
u3_tool_usb_CFLAGS = $(LIBUSB_CFLAGS)
u3_tool_usb_LDADD = $(LIBUSB_LIBS)
