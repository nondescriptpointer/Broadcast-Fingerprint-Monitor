#include "Streamer.h"
#include <iostream>
#include <cstring>

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

  apsink in combination with the caps to specify the format

  if the sample rate is 11025 we should have roughly 119070 samples collected
*/

Streamer::Streamer(void){
	// setup pipeline
	GstElement *pipeline = gst_pipeline_new("radiopipeline");

	// playbin object
	GstElement *uridecodebin = gst_element_factory_make("uridecodebin","uridecodebin");
	//g_object_set(G_OBJECT(uridecodebin), "uri", "http://mp3.streampower.be/radio1-high.mp3", NULL);
	g_object_set(G_OBJECT(uridecodebin), "uri", "file:///home/ego/Downloads/safteyjingle.mp3", NULL);

	// create an audio sink
	GstElement *convert = gst_element_factory_make("audioconvert","audioconvert");
	GstElement *resample = gst_element_factory_make("audioresample","audioresample");
	GstElement *sink = gst_element_factory_make("appsink", "appsink");
	g_object_set(G_OBJECT(sink), "caps", gst_caps_new_simple("audio/x-raw",
		"format",G_TYPE_STRING,"F32LE",
		"rate",G_TYPE_INT,11025,
		"channels",G_TYPE_INT,1,
	NULL),NULL);
	g_object_set(G_OBJECT(sink), "emit-signals", TRUE, NULL);

	// listen for new buffer samples
	int samples = 0;
	std::cout << &samples << std::endl;
	g_signal_connect(sink, "new-sample", G_CALLBACK(buffer_callback), &samples);
	
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

// buffer callback
GstFlowReturn Streamer::buffer_callback(GstElement *element, gpointer data){
	g_print("Got a buffer sample\n");

	GstSample *sample;
	GstMapInfo map;
	GstBuffer *buffer;
	int *num_samples = static_cast<int*>(data);
	std::cout << *num_samples << std::endl;
	g_signal_emit_by_name(element, "pull-sample", &sample, NULL);
	if(sample){
		buffer = gst_sample_get_buffer(sample);
		gst_buffer_map (buffer, &map, GST_MAP_READ);
    	g_print("\n here size=%d\n",map.size);
    	// cast our buffer to floats
    	void *ptr = map.data;
    	float *floats = static_cast<float*>(ptr);
    	// increase number of samples
    	*num_samples += map.size;

    	// show floats
    	std::cout << floats[0] << std::endl;
    	std::cout << floats[1] << std::endl;
    	std::cout << floats[2] << std::endl;
    	std::cout << floats[3] << std::endl;
    	/*
    	
    	items = static_cast<float>(map.data);*/
    	/*std::cout << sizeof(float) << std::endl;
    	float g;
    	memcpy(&g, map.data, sizeof(float));
    	std::cout << g << std::endl;*/
    	//std::cout << float(map.data) << " " << float(map.data[1]) << std::endl;
    	// TODO: these are not floats which is an issue
	    gst_buffer_unmap (buffer,&map);
	    gst_sample_unref(sample);
	}

	return GST_FLOW_OK;
}

// pad handling
void Streamer::pad_callback(GstElement *element, GstPad *pad, gpointer data){
	GstPad *sinkpad;
	GstElement *decoder = (GstElement *) data;

	sinkpad = gst_element_get_static_pad(decoder,"sink");
	gst_pad_link(pad, sinkpad);
	gst_object_unref(sinkpad);
}

/* 
TODO:
- try and convert it to a float* array with the size of the original divided by 4
- try and feed it into echoprint codegen and retrieve the full signature
- compare with the signature created by fastingest or similar tools

*/