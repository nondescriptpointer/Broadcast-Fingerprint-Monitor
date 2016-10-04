#pragma once

#include <gst/gst.h>

class Streamer {
	private:

	public:
		Streamer(void);
		~Streamer(void);
		static gboolean bus_callback(GstBus *bus, GstMessage *message, gpointer data);
};