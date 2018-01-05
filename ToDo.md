# DFC_ESP8266FtpServer TO DO list


To Do v1
------
- [ ] Check all replies, are make them more uniform, report filename etc

- [D] Check if all error path are covered.
- [D] Check for max path length (32 bytes)
- [D] implement ABORT
- [D] Client.TransferFile needs to be closed/checked before opening
- [D] Client.TransferFile needs to be checked if open before reading/writing
- [D] Implement SPIFFS.info for max path length
- [D] Timeout on ftp and data connections
- [D] Timeout on waiting for data connection

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
- [F] Data connections function only the first time.
- [F] Timeout during on client connection during transfer

Known Issues
------
