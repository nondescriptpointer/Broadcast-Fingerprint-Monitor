Design:
- Gstreamer to handle incoming stream and convert it to PCM, float, mono 11025 Hz
- Use Echoprint library to generate our codestring of the target audio
- Update the codestring as a sliding window if possible, checking the match percentage during each slide
- The matching should be our own implementation, use echoprint-server as a reference
- If there is a match, execute script and ignore further matching for a few minutes of stream data is received
- Handle disconnects and timeouts gracefully by reconnecting automatically

Todo:
- Download shoutcast server
- Download C++ gstreamer examples (preferably streaming)

Matching algorithms:
- Seems to match number of floating values matching overall (order independent?)
- Walk through the matching algorithm to make sure what we are doing is correct
