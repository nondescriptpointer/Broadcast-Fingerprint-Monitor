#include <iostream>
#include "Streamer.h"
#include <gst/gst.h>
#include <stdio.h>

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


    std::cout << "Test" << std::endl;
    new Streamer;
}
