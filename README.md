# ReliableUDP
A reliable version of UDP, utilizing packet loss and re transmissions
# How to use
## Receiver
1) compile receiver on gnu.<br />
*gcc receiver.c -O receive*
2) Run receiver by typing<br />
	*./receiver \<port number>* <br /><br/>
  
```
example:  ./receiver 3000
```
## Sender
1) compile sender on gnu.<br />
	*gcc sender.c -O sender*
2) Run sender by typing<br />
	*./sender \<IP address> \<port number> \<video file name or path>* <br /><br/>
```
example:  ./sender 127.0.0.1 3000 video.mp4
```
### Important Notes
1) Run receiver before running sender
2) Sender and Receiver port number must be same
3) Make sure the video file to be sent is in the same folder as the code, otherwise you need to give full path to the video file as third argument for sender
4) Receiver will save the video by the name received.mp4
6) clear your screen before running the code, otherwise the printed stats may be printed in between old printed information

