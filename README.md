# Broadcast fingerprint monitor

A small C++ program to monitor a radio stream if a specific track or ad is played. It's using gstreamer to pull in the PCM data and echoprint-codegen for fingerprinting.

To use, first build a fingerprint of what you want to scan for:
	monitor build file:///path/to/file.mp3
Then just run this to start scanning a radio channel
	monitor scan "http://url-to.com/livestream.mp4" commandtorunwhenfound

Notes:
- You'll probably want to modify the buffersize (buffer that is matched to the audio fingerpint), updaterate (how often the fingerprints are compared) and minimum_rate (how many codes need to match at a minimum to match)
- We are currently logging the PCM data to disk when a match has been found for diagnostics
- I am are only using the codes, not the frames from echoprint. Accuracy can likely be increased by doing this.
- Echoprint has been slightly modified to not compress the fingerprints since that is not needed for my use case.