# CameraspII
# Web cam on Rasspberry Pi

## Brief outline of Architecture
The application  consists of 
* the webserver which launches 
* the frame grabber application and
* a pinger that runs on the desktop to locate the IP address of the device

 The need for two processes is that once the camera is opened with a specific height and width, it is not possible to change it unless the application is launched again. The web server is essentially a REST API interface. The aim is to develop a single page web application that talks to the web server and sets or collects values as json files. The current <code>index.html</code> has significant scope for improvement. The web server also hosts an echo server to allow the IP address of the devive to be located.

 The frame grabber has two threads. The first one is solely for IPC with controller (web server) and the second thread is used for periodic image capture. The captured images are analysed for motion detection  and a mail is sent if motion is detected.

 The project uses C++ (GCC 8). 

## Documentation
The Doxygen generated output can be found at my [home page](http://thesundayprogrammer.com/camerasp/md_README.html).

## Acknowledgements
TheSundayProgrammer acknowledges with thanks the use of the following projects in [CameraspII](http://github.com/theSundayProgrammer/CameraspII)
 * the userland library code for capturing images from [James Hughes](https://github.com/JamesH65/userland)
 * The asynchronous model is driven using Chris Kohloff's [Asio](https://github.com/chriskohlhoff/asio)
 * The web server was adapted from [Ole Christian Eidheim](https://github.com/eidheim/Simple-Web-Server) which uses Asio
 * [Boost](www.boost.org): IPC, Base64, Filesystem etc.
 * The [json parser](https://github.com/open-source-parsers/jsoncpp)
 * [GSL](https://github.com/Microsoft/GSL) from Microsoft
 * [Cédric Verstraeten] (https://github.com/cedricve/motion-detection/blob/master/motion_src/src/motion_detection.cpp) who uses [OpenCV](https://opencv.org/) for motion detection

 Above all thanks to Linus Travold for Linux/Git and the GNU Foundation for all the tools used in the development. Thanks also
 to Microsoft and Github for hosting the repo. 

