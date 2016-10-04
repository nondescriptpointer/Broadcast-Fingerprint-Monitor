#include "Streamer.h"
#include <iostream>

// should I use the playbin and/or uridecodebin, appsink? should be in base plugins
// autoaudiosink
// it's possible to have a handler on a smaller level
// our handler is static right now which limits our ability to use the current state?

/*
  source   = gst_element_factory_make ("filesrc",       "file-source");
  demuxer  = gst_element_factory_make ("oggdemux",      "ogg-demuxer");
  decoder  = gst_element_factory_make ("vorbisdec",     "vorbis-decoder");
  conv     = gst_element_factory_make ("audioconvert",  "converter");
  sink     = gst_element_factory_make ("autoaudiosink", "audio-output");

  need to convert it to floating point PCM at 11025Hz and mono
*/

Streamer::Streamer(void){
	// setup pipeline
	GstElement *pipeline = gst_pipeline_new("radiopipeline");

	// playbin object
	GstElement *uridecodebin = gst_element_factory_make("uridecodebin","uridecodebin");
	g_object_set(G_OBJECT(uridecodebin), "uri", "http://mp3.streampower.be/radio1-high.mp3", NULL);

	// create an audio sink
	GstElement *convert = gst_element_factory_make("audioconvert","audioconvert");
	GstElement *resample = gst_element_factory_make("audioresample","audioresample");
	GstElement *sink = gst_element_factory_make("autoaudiosink", "audio-output");
	
	// add elements to pipeline
	gst_bin_add_many(GST_BIN(pipeline), uridecodebin, convert, resample, sink, NULL);

	// link the conversion pipeline and sink already
	if(!gst_element_link_many(convert, resample, sink, NULL)){
		g_warning("Failed to link elements!");
	}

	// listen for newly created pads on the uridecodebin
	g_signal_connect(uridecodebin, "pad-added", G_CALLBACK(pad_callback), convert);

	// create a bus to listen to issues
	GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	guint bus_watch_id = gst_bus_add_watch(bus,bus_callback,NULL);
	gst_object_unref(bus);

	// start playback
	gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);
};

// cleanup streamer
Streamer::~Streamer(void){
	// TODO: cleanup
	/*gst_element_set_state(pipeline,GST_STATE_NULL);
	gst_object_unref(pipeline);
	g_source_remove(bus_watch_id);*/
}

// message handling
gboolean Streamer::bus_callback(GstBus *bus, GstMessage *message, gpointer data){
	g_print("Got %s message\n",GST_MESSAGE_TYPE_NAME(message));

	switch (GST_MESSAGE_TYPE (message)){
	    case GST_MESSAGE_ERROR: {
			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
			g_print ("Error: %s\n", err->message);
			g_error_free (err);
			g_free (debug);
			//g_main_loop_quit (loop);
			break;
	    }
	    case GST_MESSAGE_EOS:
			/* end-of-stream */
			//g_main_loop_quit (loop);
			break;
	    case GST_MESSAGE_STATE_CHANGED: {
	    	GstState old_state, new_state;
	    	gst_message_parse_state_changed(message, &old_state, &new_state, NULL);
	    	g_print("Element %s changed state from %s to %s\n",GST_OBJECT_NAME(message->src),gst_element_state_get_name(old_state),gst_element_state_get_name(new_state));
	    	break;
	    }
	    default:
	      /* unhandled message */
	      break;
	}

	return TRUE;
}

// pad handling
void Streamer::pad_callback(GstElement *element, GstPad *pad, gpointer data){
	GstPad *sinkpad;
	GstElement *decoder = (GstElement *) data;

	sinkpad = gst_element_get_static_pad(decoder,"sink");
	gst_pad_link(pad, sinkpad);
	gst_object_unref(sinkpad);

	/*
	gchar *name;
	name = gst_pad_get_name(pad);
	g_print("A new pad %s was created\n",name);
	g_free(name);*/

	// set up a new pad link for the newly created pad
}