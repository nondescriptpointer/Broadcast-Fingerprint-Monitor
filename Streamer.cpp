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
*/

Streamer::Streamer(void){
	// setup pipeline
	GstElement *pipeline = gst_pipeline_new("my-pipeline");

	// create a bus to listen to issues
	GstBus *bus = gst_pipeline_get_bus(GST_PIPELINE(pipeline));
	guint bus_watch_id = gst_bus_add_watch(bus,bus_callback,NULL);
	gst_object_unref(bus);

	// create elements
	GstElement *source, *filter, *sink;
	source = gst_element_factory_make("fakesrc","source");
	filter = gst_element_factory_make("identity","filter");
	sink = gst_element_factory_make("fakesink","sink");

	// add elements to the pipeline
	gst_bin_add_many(GST_BIN(pipeline), source, filter, sink, NULL);

	// link the elements
	if(!gst_element_link_many(source,filter,sink,NULL)){
		g_warning("Failed to link elements!");
	}

	// start playback
	gst_element_set_state(GST_ELEMENT(pipeline), GST_STATE_PLAYING);


	// check if element exists
	/*if(!element){
		g_print("Failed to create element of type 'fakesrc'\n");
	}*/

	// get the name of the element
	/*gchar *name;
	g_object_get(G_OBJECT(element),"name",&name,NULL);
	g_print("Name of the element is %s\n",name);
	g_free(name);*/


	//gst_object_unref(GST_OBJECT(element));
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