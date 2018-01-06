# DFC_ESP8266FtpServer TO DO list


To Do v1
------
- [X] Check all replies, are make them more uniform, report filename etc
- [X] Add sensible DBG output on events
- [X] Check if all error path are covered.
- [X] Check for max path length (32 bytes)
- [X] implement ABORT
- [X] Client.TransferFile needs to be closed/checked before opening
- [X] Client.TransferFile needs to be checked if open before reading/writing
- [X] Implement SPIFFS.info for max path length
- [X] Timeout on ftp and data connections
- [X] Timeout on waiting for data connection
- [X] implement NOOP
- [X] Implement rename of files and directories

To Do v2
------
- [ ] Implement MLST and MLSD
- [ ] Update FEAT with SIZE, MLST, MLSD

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
- [X] Data connections function only the first time.
- [X] Timeout during on client connection during transfer
- [ ] Renaming file from "/test" to "/test1" give interesting results.

Known Issues
------
- Creating (MKD) directories is emulated. But it's not check on every command.
  Resulting that some commands don't work on the emulated directory or results are not
  exactly as expected.