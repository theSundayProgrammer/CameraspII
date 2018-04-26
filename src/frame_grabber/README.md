# Some notes on the evolution of the project.

## HTTP
The project started of with using [Ken Baker](https://github.com/kenba/via-http)'s via-http library.
But the web server was hanging after a few hours of running. Unable to locate the issue and not finding 
the time to come up with a minimum reproducible project, I tried Ole Eidheim's web server which has so far been pretty stable.


## Raspicam
The project originally used the C++ library from [Cedric](https://github.com/cedricve/raspicam).
Again after some time, longer than a few hours, the call back function was not called. However
[James Hughes](https://github.com/JamesH65/userland) was more stable. That is what I am using now

## SMTP
Initiatially I used <code>curl</code> library for SMTP. There were two issues:
 * It was syncronous
 * It was sending the same message n times to the same receipient at the n-th iteration.

So I decided to write the Smtp protocol using asio. [Jay's](https://raspberrypiprogramming.blogspot.com.au/2014/09/send-email-to-gmail-in-c-with-boost-and.html)
blog was very helpful although he considers only synchronous email.
