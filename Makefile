gst_src: gst_src.cc gstmark.c
	g++ gst_src.cc gstmark.c -o gst_src `pkg-config --cflags --libs gstreamer-1.0`
