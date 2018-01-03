# DFC_ESP8266FtpServer

An FTP server for the WEMOS D1.

Development status
------
In it's initial development. Highly instable.

Connections
------
It can handle multiple control connections. At this moment only one data connection per control connection. Also at the moment only passive mode is implemented.

Directory support
------
SPIFFS does not support directories. But accepts "/" in file names. This way directories are simulated. The esp8266 sketch data upload does this simular. 

#### MKD
FTP accepts this command, but simulates the directorie for a short while. When files are stored in the directory it becomes permanent.

#### RMD
As a result directories can not be removed, and are "removed" automatically when it doesn't not contain files anymore.

Origin
------
I started out with the FTP implementation of nailbuster. I use this a an ispiration. But i have some "not invented by me" issues....
https://github.com/nailbuster/esp8266FTPServer
