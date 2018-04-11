# CameraspII
# Web cam on Rasspberry Pi

## Brief outline of Architecture
The application  consists of 
* the webserver which launches 
* the frame grabber application.

 The need for two processes is that once the camera is opened with a specific height and width, it is not possible to change it unless the application is launched again. The web server is essentially a REST API interface. The aim is to develop a single page web application that talks to the web server and sets or collects values as json files. The current <code>index.html</code> has significant scope for improvement.

 The frame grabber has two threads. The first one is solely for IPC with controller (web server) and the second thread is used for periodic image capture. The captured images are analysed for motion detection (need to cite original source/inspiration for this) and a mail is sent if motion is detected.

## Acknowledgements
TheSundayProgrammer acknowledges with thanks the use of the following projects in [CameraspII](http://github.com/theSundayProgrammer/CameraspII)
 * the userland library code for capturing images from [James Hughes](https://github.com/JamesH65/userland)
 * The web server was adapted from [Ole Christian Eidheim](https://github.com/eidheim/Simple-Web-Server)
 * The asynchronous model is driven using Chris Kohloff's [asio](https://github.com/chriskohlhoff/asio)
 * [Boost](www.boost.org)  "ipc" as well as other Boost projects
 * The [json parser](https://github.com/open-source-parsers/jsoncpp)
 * [GSL](https://github.com/Microsoft/GSL) from Microsoft
 
