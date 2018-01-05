# DFC_ESP8266FtpServer TO DO list


To Do v1
------
- [ ] timeout on ftp and data connections
- [ ] Check all replies, are make them more uniform, report filename etc

- [X] Check if all error path are covered.
- [X] Check for max path length (32 bytes)
- [X] implement ABORT
- [X] Client.TransferFile needs to be closed/checked before opening
- [X] Client.TransferFile needs to be checked if open before reading/writing
- [X] Implement SPIFFS.info for max path length

To Do v2
------
- [ ] Implement MLST and MLSD
- [ ] Update FEAT with SIZE, MLST, MLSD
- [ ] Implement rename
- [ ] implement NOOP

Wish List
------

Far Fetched and Dream list
------
- Abstracting Filesystem
- Support for SD cards
- Abstracting TCP
- ACTIVE ftp (is this still used ??)
- REST (restart). Pure luxery.

Legacy and techinical dept
------

Known Bugs
------

Known Issues
------
