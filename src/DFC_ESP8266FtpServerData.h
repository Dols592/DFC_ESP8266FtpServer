//
#ifndef DFC_ESP8266_FTP_SERVER_DATA_H
#define DFC_ESP8266_FTP_SERVER_DATA_H

//Config defines
#define FTP_DATA_BUF_SIZE       1000

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
  String SeqCommand; //Command we expect next for commands that need to be in sequence
  String SeqArgument; //argument of previous command
  nTransferMode TransferMode;
  int32_t PasvListenPort;
  bool WaitingForDataConnection;
  WiFiServer* PasvListenServer;
  WiFiClient DataConnection;
  nTransferCommand TransferCommand;
  String TempDirectory;
  File TransferFile;
  int32_t LastReceivedCommand;
  int32_t LastReceivedData;
  uint8_t* DataBuffer;

  void Reset()
  {
    InUse = false;
    ClientConnection.flush();
    ClientConnection.stop();
    FtpState = NFS_IDLE;
    CurrentPath = "/";
    ControlState = NCS_START;
    Command = "";
    Arguments = "";
    SeqCommand = "";
    SeqArgument = "";
    TransferMode = NTM_UNKNOWN;
    PasvListenPort = 0;
    WaitingForDataConnection = false;
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
    LastReceivedCommand = 0;
    LastReceivedData = 0;
  }
  SClientInfo()
  {
    DataBuffer = new uint8_t[FTP_DATA_BUF_SIZE];
    PasvListenServer = NULL;
    Reset();
  }
};

#endif //DFC_ESP8266_FTP_SERVER_DATA_H

