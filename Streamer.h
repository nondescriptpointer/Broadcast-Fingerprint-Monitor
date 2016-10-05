#pragma once

#include <Fingerprint.h>
#include <gst/gst.h>
#include <vector>
#include <ctime>
#include <set>

class Streamer {
	private:
		const int buffersize = 120000;
		const int updaterate = 11025;
		const float minimum_rate = 0.5;
	public:
		std::vector<float> samples;
		std::vector<FPCode*> refcodes;
		std::set<uint> refcode_keys;
		GMainLoop *mainloop;
		bool run_mode;
		int samples_received;
		const char* command;

		Streamer(const char* url, GMainLoop *loop, bool mode, const char* cmd); // mode = true is used to index the song, mode is false is used to listen based on data
		~Streamer(void);
		static gboolean bus_callback(GstBus *bus, GstMessage *message, gpointer data);
		static void pad_callback(GstElement *element, GstPad *pad, gpointer data);
		static GstFlowReturn buffer_callback(GstElement *element, gpointer data);
};