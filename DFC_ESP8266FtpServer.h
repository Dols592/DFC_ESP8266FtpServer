//
#ifndef DFC_ESP8266_FTP_SERVER_H
#define DFC_ESP8266_FTP_SERVER_H

//Config defines
#ifndef FTP_MAX_CLIENTS
  #define FTP_MAX_CLIENTS   2
#endif

//Includes
#include <vector>
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
    }
    PasvListenServer = NULL;
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

class DFC_ESP7266FtpServer
{
public: //Constructor
  DFC_ESP7266FtpServer();

public: //Interface
  void    Start();
  void    Loop();

protected: //Help functions
  bool      GetEmptyClientInfo(int32_t& Pos);
  void      CheckClient(SClientInfo& Client);
  void      CheckData(SClientInfo& Client);
  void      DisconnectClient(SClientInfo& Client);
  void      GetControlData(SClientInfo& Client);
  String    GetFirstArgument(SClientInfo& Client);
  String    ConstructPath(SClientInfo& Client, bool IsPath);
  bool      GetFileName(String CurrentDir, String FilePath, String& FileName, bool& IsDir);
  bool      GetParentDir(String FilePath, String& ParentDir);
  bool      ExistDir(String FilePath);

  int32_t   GetNextDataPort();
  bool      CheckIfPresentList(std::vector<String>& DirList, const String& Name);

  void      ProcessCommand(SClientInfo& Client);
  bool      Process_USER(SClientInfo& Client);
  bool      Process_PASS(SClientInfo& Client);
  bool      Process_QUIT(SClientInfo& Client);
  bool      Process_SYST(SClientInfo& Client);
  bool      Process_FEAT(SClientInfo& Client);
  bool      Process_HELP(SClientInfo& Client);
  bool      Process_PWD(SClientInfo& Client);
  bool      Process_CDUP(SClientInfo& Client);
  bool      Process_CWD(SClientInfo& Client);
  bool      Process_MKD(SClientInfo& Client);
  bool      Process_RMD(SClientInfo& Client);
  bool      Process_TYPE(SClientInfo& Client);
  bool      Process_PASV(SClientInfo& Client);
  bool      Process_PORT(SClientInfo& Client);
  bool      Process_LIST(SClientInfo& Client);
  bool      Process_SIZE(SClientInfo& Client);
  bool      Process_DELE(SClientInfo& Client);
  bool      Process_STOR(SClientInfo& Client);
  bool      Process_RETR(SClientInfo& Client);
  bool      Process_DataCommand_Preprocess(SClientInfo& Client);
  bool      Process_DataCommand_Responds_OK(SClientInfo& Client, nTransferCommand TransferCommand);
  bool      Process_DataCommand_END(SClientInfo& Client);
  bool      Process_DataCommand_DISCONNECTED(SClientInfo& Client);
  bool      Process_Data_LIST(SClientInfo& Client);
  bool      Process_Data_STOR(SClientInfo& Client);
  bool      Process_Data_RETR(SClientInfo& Client);

public: //Config
  String  mServerUsername;
  String  mServerPassword;

protected: //Variables
  WiFiServer mFtpServer;
  SClientInfo mClientInfo[2];
  int32_t mLastDataPort;
};

#endif //DFC_ESP8266_FTP_SERVER_H