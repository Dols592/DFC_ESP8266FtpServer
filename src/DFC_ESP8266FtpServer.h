//
#ifndef DFC_ESP8266_FTP_SERVER_H
#define DFC_ESP8266_FTP_SERVER_H

//Config defines
#ifndef FTP_MAX_CLIENTS
  #define FTP_MAX_CLIENTS   2
#endif

//Includes
#include "DFC_ESP8266FtpServerData.h"
#include <vector>

class DFC_ESP7266FtpServer
{
public: //Constructor
  DFC_ESP7266FtpServer();
  
public: //Interface
  void      Init();
  void      Loop();

protected: //Help functions
  void      Loop_ClientConnection(SClientInfo& Client);
  void      Loop_GetControlData(SClientInfo& Client);
  void      Loop_ProcessCommand(SClientInfo& Client);
  void      Process_USER(SClientInfo& Client);
  void      Process_PASS(SClientInfo& Client);
  void      Process_QUIT(SClientInfo& Client);
  void      Process_SYST(SClientInfo& Client);
  void      Process_FEAT(SClientInfo& Client);
  void      Process_HELP(SClientInfo& Client);
  void      Process_PWD(SClientInfo& Client);
  void      Process_CDUP(SClientInfo& Client);
  void      Process_CWD(SClientInfo& Client);
  void      Process_MKD(SClientInfo& Client);
  void      Process_RMD(SClientInfo& Client);
  void      Process_TYPE(SClientInfo& Client);
  void      Process_PASV(SClientInfo& Client);
  void      Process_PORT(SClientInfo& Client);
  void      Process_LIST(SClientInfo& Client);
  void      Process_SIZE(SClientInfo& Client);
  void      Process_DELE(SClientInfo& Client);
  void      Process_STOR(SClientInfo& Client);
  void      Process_RETR(SClientInfo& Client);
  void      Process_ABOR(SClientInfo& Client);
  void      Loop_DataConnection(SClientInfo& Client);
  bool      Process_DataCommand_Preprocess(SClientInfo& Client);
  void      Process_DataCommand_Responds_OK(SClientInfo& Client, nTransferCommand TransferCommand);
  void      Process_DataCommand_END(SClientInfo& Client);
  void      Process_DataCommand_DISCONNECTED(SClientInfo& Client);
  void      Process_Data_LIST(SClientInfo& Client);
  void      Process_Data_STOR(SClientInfo& Client);
  void      Process_Data_RETR(SClientInfo& Client);
  bool      Help_GetEmptyClientInfo(int32_t& Pos);
  int32_t   Help_GetNextDataPort();
  void      Help_DisconnectClient(SClientInfo& Client);
  String    Help_GetFirstArgument(SClientInfo& Client);
  String    Help_GetPath(SClientInfo& Client, bool IsPath);
  bool      Help_GetFileName(String CurrentDir, String FilePath, String& FileName, bool& IsDir);
  bool      Help_GetParentDir(String FilePath, String& ParentDir);
  bool      Help_DirExist(String FilePath);
  bool      Help_CheckIfDirIsUnique(std::vector<String>& DirList, const String& Name);

public: //Config
  String mServerUsername;
  String mServerPassword;

protected: //Variables
  WiFiServer mFtpServer;
  SClientInfo mClientInfo[FTP_MAX_CLIENTS];
  int32_t mLastDataPort;
};

#endif //DFC_ESP8266_FTP_SERVER_H