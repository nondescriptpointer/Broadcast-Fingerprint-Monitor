#include <iostream>
#include "Streamer.h"
#include <gst/gst.h>
#include <stdio.h>
#include <string>
#include <sstream> 

// main loop
static GMainLoop *mainloop;

int main(int argc, char *argv[]){
	// initialize gstreamer
	gst_init(&argc,&argv);
	// get version
	guint major, minor, micro, nano;
	gst_version(&major,&minor,&micro,&nano);
	const gchar *nano_string;
	if(nano == 1) nano_string = "(CVS)";
	else if(nano == 2) nano_string = "(Prerelease)";
	else nano_string = "";
	printf("GStreamer version %d.%d.%d %s\n", major, minor, micro, nano_string);

    // main loop to keep our app active
    mainloop = g_main_loop_new(NULL,FALSE);

    // check argc
    if(argc < 4){
    	std::cout << "Usage: monitor [build/scan] [local or remote url] [channel id] [command to run when found]" << std::endl;
    	return 1;
    }

    // mode
    bool mode = false;;
    std::string comp = "build";
    if(argv[1] == comp){
    	std::cout << "Rebuilding the reference fingerprint" << std::endl;
    	mode = true;
    }

    // convert rest to 
    const char* command = "";
    if(argc > 4){
        std::stringstream ss;
        for(int i = 4; i < argc; i++){
            ss << argv[i] << " ";
        }
        command = ss.str().c_str();
    }

    // set up the streamer
    Streamer* streamer = new Streamer(argv[2],mainloop,mode,command,argv[3]);

    // run main loop
    g_main_loop_run(mainloop);  

    // cleanup
    delete streamer;
}
