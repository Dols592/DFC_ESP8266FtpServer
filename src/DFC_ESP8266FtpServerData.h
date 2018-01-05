//
#ifndef DFC_ESP8266_FTP_SERVER_DATA_H
#define DFC_ESP8266_FTP_SERVER_DATA_H

//Config defines

//Includes
#include <ESP8266WiFi.h>
#include <fs.h>

enum nFtpState
{
  NFS_IDLE = 0,
  NFS_WAITFORUSERNAME,
  NFS_WAITFORPASSWORD_USER_REJECTED, //we display error after password
  NFS_WAITFORPASSWORD,
  NFS_WAITFORCOMMAND
};

enum nControlState
{
  NCS_START = 0,
  NFS_COMMAND,
  NFS_ARGUMENTS,
  NFS_END
};

enum nTransferCommand
{
  NTC_NONE = 0,
  NTC_LIST,
  NTC_STOR,
  NTC_RETR
};

enum nTransferMode
{
  NTM_UNKNOWN = 0,
  NTC_ACTIVE,
  NTC_PASSIVE
};

struct SClientInfo
{
  bool InUse;

  WiFiClient ClientConnection;
  nFtpState FtpState;
  String CurrentPath;

  nControlState ControlState;
  String Command;
  String Arguments;

  nTransferMode TransferMode;
  int32_t PasvListenPort;
  WiFiServer* PasvListenServer;
  WiFiClient DataConnection;
  nTransferCommand TransferCommand;
  String TempDirectory;
  File TransferFile;
  
  void Reset()
  {
    InUse = false;
    ClientConnection.flush();
    ClientConnection.stop();
    FtpState = NFS_IDLE;
    CurrentPath = "/";
    ControlState = NCS_START;
    TransferMode = NTM_UNKNOWN;
    PasvListenPort = 0;
    if (PasvListenServer)
    {
      PasvListenServer->close();
      delete PasvListenServer;
      PasvListenServer = NULL;
    }
    DataConnection.flush();
    DataConnection.stop();
    TransferCommand = NTC_NONE;
    TempDirectory = "";
    TransferFile.close();
  }
  SClientInfo()
  {
    PasvListenServer = NULL;
    Reset();
  }
};

#endif //DFC_ESP8266_FTP_SERVER_DATA_H

