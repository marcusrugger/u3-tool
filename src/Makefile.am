sbin_PROGRAMS = u3-tool

shared_source = display_progress.c display_progress.h main.c md5.c md5.h \
	secure_input.c secure_input.h u3_commands.c u3_commands.h u3_error.c \
	u3_error.h u3.h u3_scsi.h 

u3_tool_SOURCES = $(shared_source) u3_scsi_sg.c sg_err.h

if HAVE_LIBUSB
sbin_PROGRAMS += u3-tool-usb

u3_tool_usb_SOURCES = $(shared_source) u3_scsi_usb.c
u3_tool_usb_CFLAGS = $(LIBUSB_CFLAGS)
u3_tool_usb_LDADD = $(LIBUSB_LIBS)
endif

EXTRA_DIST = u3_tool.dev u3_scsi_spt.c
