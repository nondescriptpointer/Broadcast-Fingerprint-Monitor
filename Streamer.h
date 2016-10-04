#pragma once

#include <gst/gst.h>

class Streamer {
	private:

	public:
		Streamer(void);
		~Streamer(void);
		static gboolean bus_callback(GstBus *bus, GstMessage *message, gpointer data);
		static void pad_callback(GstElement *element, GstPad *pad, gpointer data);
};