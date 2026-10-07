libltc (SMPTE linear timecode) - https://github.com/x42/libltc

The .c files and decoder.h / encoder.h are from the libltc 1.3.1 release and are compiled into the
Windows build. ltc.h is the header xSchedule has always used (a 2019 snapshot with the same API as 1.3.1).
Linux builds link the system libltc instead (-lltc). License: LGPL-3.0-or-later, see COPYING.
