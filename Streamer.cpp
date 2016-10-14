#include "Streamer.h"
#include <iostream>
#include <cstring>
#include <Codegen.h>
#include <string>
#include <fstream>
#include <sstream>
#include <vorbis/vorbisenc.h>
#include <boost/filesystem.hpp>

using namespace boost::filesystem;

/*
	// todo: find distance difference between the 2 files if needed
*/

Streamer::Streamer(const char* url, GMainLoop *loop, bool mode, const char *cmd, const char* chan){
	// ref to mainloop
	mainloop = loop;
	run_mode = mode;
	command = cmd;
	uri = url;
    channel_id = chan;

	// read the file with the data
	if(!run_mode){
		// check size first
	    std::ifstream fcheck("ref_fingerprint.dat", std::ifstream::ate | std::ifstream::binary);
	    int filesize = fcheck.tellg();
	    fcheck.close();
	    
	    // read the file
		std::ifstream is("ref_fingerprint.dat",std::ios::binary | std::ios::in);
		uint *data = new uint[filesize/sizeof(uint)];
		is.read(reinterpret_cast<char*>(data), std::streamsize(filesize));
		is.close();

		// load codes into our reference vector
		for(uint i = 0; i < filesize/sizeof(uint)/2; i++){
			refcodes.push_back(new FPCode(data[i*2],data[i*2+1]));
			refcode_keys.insert(data[i*2+1]);
		}
	}

	// setup pipeline
	GstElement *pipeline = gst_pipeline_new("radiopipeline");

	// playbin object
	GstElement *uridecodebin = gst_element_factory_make("uridecodebin","uridecodebin");
	// "http://mp3.streampower.be/radio1-high.mp3" or "file:///home/ego/Downloads/safteyjingle.mp3"
	g_object_set(G_OBJECT(uridecodebin), "uri", url, NULL);

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
	g_signal_connect(sink, "new-sample", G_CALLBACK(buffer_callback), this);
	
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
	gst_bus_add_watch(bus,bus_callback,this);
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
	    	Streamer *streamer = static_cast<Streamer*>(data);

			GError *err;
			gchar *debug;

			gst_message_parse_error (message, &err, &debug);
			g_print ("Error: %s\n", err->message);
			g_error_free (err);
			g_free (debug);

			// restart, we'll restart the process
			g_main_loop_quit(streamer->mainloop);
			break;
	    }
	    case GST_MESSAGE_EOS: {
	    	Streamer *streamer = static_cast<Streamer*>(data);

	    	// if we are scanning file, write it to file
	    	if(streamer->run_mode){
	    		// copy the samples into an array
	    		int numsamples = streamer->samples.size();
	    		float *data = new float[numsamples];
	    		std::copy(streamer->samples.begin(),streamer->samples.end(), data);

	    		// do a codegen on the samples
				Codegen * pCodegen = new Codegen(data, numsamples, 0);
				std::vector<FPCode> codes = pCodegen->getCodes(); 

				// create data struct of uints twice the size of the file to save
				uint *savedata = new uint[codes.size()*2];
				for(uint i = 0; i < codes.size(); i++){
					savedata[i*2] = codes[i].frame;
					savedata[i*2+1] = codes[i].code;
				}
				// save this data to datafile
				std::ofstream output("ref_fingerprint.dat", std::ios::binary | std::ios::trunc | std::ios::out);
				output.write(reinterpret_cast<const char*>(savedata), std::streamsize(codes.size()*2*sizeof(uint)));
				output.close();

				std::cout << "Succesfully wrote ref_fingerpint.dat" << std::endl;

				// display to check
				/*std::cout << codes.size() << std::endl;
				for (uint i = 0; i < codes.size(); i++){
					std::cout << codes[i].frame << " " << codes[i].code << std::endl;
				}*/

				// cleanup codegen
				delete pCodegen;

				// quit the loop
				g_main_loop_quit (streamer->mainloop);
			}else{
				// restart, we'll restart the process to try and reconnect
				g_main_loop_quit(streamer->mainloop);
			}
			break;
		}
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
	GstSample *sample;
	GstMapInfo map;
	GstBuffer *buffer;

	g_signal_emit_by_name(element, "pull-sample", &sample, NULL);
	if(sample){
		// get reference to self
		Streamer *streamer = static_cast<Streamer*>(data);

		buffer = gst_sample_get_buffer(sample);
		gst_buffer_map (buffer, &map, GST_MAP_READ);

    	// cast our buffer to floats
    	void *ptr = map.data;
    	float *floats = static_cast<float*>(ptr);

    	// update samples received
    	streamer->samples_received += map.size/4;

    	// add elements to our buffer array
    	for(uint i = 0; i < map.size/4; i++){
    		streamer->samples.push_back(floats[i]);
    	}

    	// if in normal/radio mode
    	if(!streamer->run_mode){
    		// make the buffer smaller if it's too big
    		if(streamer->samples.size() > (uint)streamer->buffersize){
    			streamer->samples.erase(streamer->samples.begin(),streamer->samples.begin()+(streamer->samples.size()-streamer->buffersize));
    		}

    		// run the check
    		if(streamer->samples.size() == (uint)streamer->buffersize && streamer->samples_received >= streamer->updaterate){
    			streamer->samples_received = 0;

	    		// copy the samples into an array
	    		int numsamples = streamer->samples.size();
	    		float *data = new float[numsamples];
	    		std::copy(streamer->samples.begin(),streamer->samples.end(), data);

	    		// do a codegen on the samples
				Codegen * pCodegen = new Codegen(data, numsamples, 0);
				std::vector<FPCode> codes = pCodegen->getCodes(); 

    			// compare with reference map
				std::set<uint> keys;
				uint unique_matches = 0;
				for(std::vector<FPCode>::iterator it = codes.begin(); it != codes.end(); ++it){
					if(keys.find(it->code) == keys.end() && streamer->refcode_keys.find(it->code) != streamer->refcode_keys.end()){
						keys.insert(it->code);
						unique_matches += 1;
					}
				}

				if(unique_matches > 50){
					std::cout << "Got a match! " << unique_matches << std::endl;
					// offset the received samples so we stop checking for a while
					streamer->samples_received = -streamer->updaterate * 15;

                    // ogg encoding
                    int eos = 0;

                    // output file
                    std::string outputdir("audiologs/");
                    std::string subfolder(streamer->channel_id);
                    path targetpath(outputdir+subfolder);
                    // create directory if it does not exist
                    if(!is_directory(targetpath)){
                        create_directories(targetpath);
                    }
                    // count number of files in directory
                    int count = 0;
                    for(directory_iterator it(targetpath); it != directory_iterator(); ++it){
                        count += 1;
                    }
                    // set up new directory
                    std::stringstream ss;
                    ss << count << ".ogg";
                    targetpath /= path(ss.str());
                    
                    //targetpath /= path("")
                    FILE* output = fopen(targetpath.string().c_str(),"wb");

                    // write the current buffer to a file
                    ogg_stream_state os; // takes physical pages, weld into logical stream of packets
                    ogg_page og; // ogg bitstream page, vorbis packets are inside
                    ogg_packet op; // raw packet of data for decode
                    vorbis_info vi; // struct that stores static vorbis bitstream settings
                    vorbis_comment vc; // struct that stores user comments
                    vorbis_dsp_state vd; // central working state for the packet->PCM decoder
                    vorbis_block vb; // local working space for packet->PCM decode

                    /* encode setup */
                    vorbis_info_init(&vi);
                    int ret = vorbis_encode_init_vbr(&vi,1,streamer->updaterate,0.1);
                    if(ret){
                        std::cerr << "Vorbis encoder creation failed." << std::endl;
                        exit(1);
                    }
                    // add comment
                    vorbis_comment_init(&vc);
                    vorbis_comment_add_tag(&vc,"ENCODER","Streamer");
                    // set up analysis state and auxiliary encoding storage
                    vorbis_analysis_init(&vd,&vi);
                    vorbis_block_init(&vd,&vb);
                    // set up packet->stream encoder, pick random serial number so we are more likely to build chained streams just by concatenation
                    srand(time(NULL));
                    ogg_stream_init(&os,rand());
                    // headers
                    {
                        ogg_packet header;
                        ogg_packet header_comm;
                        ogg_packet header_code;
                        vorbis_analysis_headerout(&vd,&vc,&header,&header_comm,&header_code);
                        ogg_stream_packetin(&os,&header);
                        ogg_stream_packetin(&os,&header_comm);
                        ogg_stream_packetin(&os,&header_code);
                        // ensures actual audio data will start on a new page as per the spec
                        while(!eos){
                            int result = ogg_stream_flush(&os,&og);
                            if(result == 0) break;
                            fwrite(og.header,1,og.header_len,output);
                            fwrite(og.body,1,og.body_len,output);
                        }
                    }
                    {
                        long i;
                        float **buffer = vorbis_analysis_buffer(&vd,numsamples);
                        for(i = 0; i < numsamples; i++){
                            buffer[0][i] = data[i];
                        }
                        vorbis_analysis_wrote(&vd,i);
                        // vorbis does some data preanalysis, then divvies up blocks for more involved processing
                        while(vorbis_analysis_blockout(&vd,&vb)==1){
                            vorbis_analysis(&vb,NULL);
                            vorbis_bitrate_addblock(&vb);
                            while(vorbis_bitrate_flushpacket(&vd,&op)){
                                ogg_stream_packetin(&os,&op);
                                while(!eos){
                                    int result = ogg_stream_pageout(&os,&og);
                                    if(result == 0) break;
                                    fwrite(og.header,1,og.header_len,output);
                                    fwrite(og.body,1,og.body_len,output);
                                    if(ogg_page_eos(&og)) eos = 1;
                                }
                            }
                        }
                    }
                    ogg_stream_clear(&os);
                    vorbis_block_clear(&vb);
                    vorbis_dsp_clear(&vd);
                    vorbis_comment_clear(&vc);
                    vorbis_info_clear(&vi);
                    fclose(output);

					// run command
					FILE *in;
					char buff[512];
					if(!(in = popen(streamer->command, "r"))){
						std::cerr << "Running command failed" << std::endl;
						delete pCodegen;
						delete data;
					    gst_buffer_unmap(buffer,&map);
					    gst_sample_unref(sample);
						return GST_FLOW_OK;
					}
					while(fgets(buff, sizeof(buff), in)!=NULL){
						std::cout << buff;
					}
					pclose(in);
				}
				delete pCodegen;
				delete data;
    		}
    	}
	    gst_buffer_unmap(buffer,&map);
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
