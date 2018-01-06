# DFC_ESP8266FtpServer

An FTP server for the WEMOS D1.

Development status
------
Most is implemented. It's should be usable. But needs more testing to call it stable.

Features
------
 - Simple intergration.
 - It can handle multiple control connections. Every control connection can have its own data connection.
 - FTP Directories are supported.  
 - Compatible with "Esp8266 Sketch Data Upload" tool.

Origin
------
I started out with the FTP implementation of nailbuster. Although the implementation is fine, it didn't do what i wanted, i started out adjusting. But i have some "not invented by me" issues....So i started all over. 

Link to Nailbuster Ftp server:
https://github.com/nailbuster/esp8266FTPServer

Limitations
------
- It strongly depends on the WiFi.
- Slow : It's a tiny low end system, with a slow SPIFFS filesystem. Speed was not one of the design targets.
- Directory support is limited : SPIFFS itself does NOT support directories. But accepts '/' characters in a filename. This is used to emulate directories. The "Esp8266 Sketch Data Upload" tool does the same. Knowing this, following is a result:
  - Empty directories can not exist on a SPIFFS.
  - If the last file in a directory is deleted, the directory disappears.

  To overcome this limitation the following done:
  - System uses the '/' in a filename for directory seperation. This is done automatically. On the FTP site i acts as normal directories most of the time.
  - CWD lets you change to directories that do not exist but you can use it.
  - MKD doesn't do anything on the SPIFFS, but remembers this last MKD to emulate this directory in a list command.
- SPIFFS support only filenames of 32 characters. That's including the null termination and directory path.  
