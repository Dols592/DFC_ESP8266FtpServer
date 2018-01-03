/*
*/

//library includes
#include <vector>
#include "DFC_ESP8266FtpServer.h"
#include <ESP8266WiFi.h>
#include <FS.h>

//Config defines
#define FTP_DEBUG
#define FTP_CONTROL_PORT        21
#define FTP_DATA_PORT_START     20100
#define FTP_DATA_PORT_END       20200

//debug
#undef DBG
#undef DBGLN
#undef DBGF

#ifdef FTP_DEBUG
  #define DBG(...) Serial.print( __VA_ARGS__ )
  #define DBGLN(...) Serial.println( __VA_ARGS__ )
  #define DBGF(...) Serial.printf( __VA_ARGS__ )
#else
  #define DBG(...)
  #define DBGLN(...)
  #define DBGF(...)
#endif

DFC_ESP7266FtpServer::DFC_ESP7266FtpServer()
  : mFtpServer( FTP_CONTROL_PORT )
  , mLastDataPort(FTP_DATA_PORT_START)
{
}

void DFC_ESP7266FtpServer::Start()
{
#if(0)
  DBGLN("=============================================");
  DBGLN("Testing");
  String ParentDir = "";
  String FilePath = "////";
  GetParentDir(FilePath, ParentDir);
  DBGF("FilePath \"%s\"\r\n", FilePath.c_str());
  DBGF("ParentDir \"%s\"\r\n", ParentDir.c_str());
  DBGLN("=============================================");
#endif
  mFtpServer.begin();
}

void DFC_ESP7266FtpServer::Loop()
{
  if (mFtpServer.hasClient())
  {
    int32_t Pos = -1;
    if (!GetEmptyClientInfo(Pos))
    {
      WiFiClient OverflowClient = mFtpServer.available();
      if (!OverflowClient.connected()) //Client disconnected before we could handle
      {
        DBGLN("A new ftp connection disconnected before we could handle.");
        return;
      }
      OverflowClient.println( "10068 Too many users !");
      OverflowClient.flush();
      OverflowClient.stop();
      return;
    }

    //send welcome message
    mClientInfo[Pos].ClientConnection = mFtpServer.available();
    if (!mClientInfo[Pos].ClientConnection.connected()) //Client disconnected before we could handle
    {
      DBGLN("A new data connection disconnected before we could handle.");
      return;
    }
    mClientInfo[Pos].FtpState = NFS_WAITFORUSERNAME;
    mClientInfo[Pos].ClientConnection.println( "220 --==[ Welcome ]==--");
  }

  for (int32_t i=0; i<FTP_MAX_CLIENTS; i++)
  {
    if (mClientInfo[i].InUse)
    {
      CheckClient(mClientInfo[i]);
    }
  }
}

bool DFC_ESP7266FtpServer::GetEmptyClientInfo(int32_t& Pos)
{
  for (int32_t i=0; i<FTP_MAX_CLIENTS; i++)
  {
    if (!mClientInfo[i].InUse)
    {
      Pos = i;
      mClientInfo[i].InUse = true;
      return true;
    }
  }

  return false;
}

void DFC_ESP7266FtpServer::CheckClient(SClientInfo& Client)
{
  //is still connected?
  if (!Client.ClientConnection.connected())
    DisconnectClient(Client);

  //Check for new Data Connection
  if (Client.PasvListenServer && Client.PasvListenServer->hasClient())
  {
    Client.DataConnection = Client.PasvListenServer->available();
    if (!Client.DataConnection.connected())
      return;

    IPAddress Lip = Client.DataConnection.localIP();
    IPAddress Rip = Client.DataConnection.remoteIP();
    uint16_t Lport = Client.DataConnection.localPort();
    uint16_t Rport = Client.DataConnection.remotePort();
    DBG("Local:");
    DBG(Lip);
    DBG(":");
    DBG(Lport);
    DBG("  ");
    DBG(Rip);
    DBG(":");
    DBG(Rport);
    DBGLN("");

    CheckData(Client);
  }

  //Check for new control data
  GetControlData(Client);
}

void DFC_ESP7266FtpServer::CheckData(SClientInfo& Client)
{
  if (Client.PasvListenServer == NULL || !Client.DataConnection.connected())
    return;

  switch (Client.TransferCommand)
  {
    case NTC_LIST:
      Process_Data_LIST(Client);
      break;

    default:
      return;
      break;
  }

  DBGLN("Data send. Closing data connection.");
  Client.DataConnection.flush();
  Client.DataConnection.stop();
  Client.TransferCommand = NTC_NONE;
}

void DFC_ESP7266FtpServer::DisconnectClient(SClientInfo& Client)
{
  if (Client.ClientConnection.connected())
    Client.ClientConnection.println("221 Goodbye");

  Client.Reset();
}

void DFC_ESP7266FtpServer::GetControlData(SClientInfo& Client)
{
  while(1)
  {
    int32_t NextChr = Client.ClientConnection.read();
    if (NextChr < 0 || NextChr > 0xFF)
      break;

    //<CR> or <LF>
    if (NextChr == '\r' || NextChr == '\n')
    {
      if (Client.ControlState == NCS_START) //initial spaces
        continue;

      ProcessCommand(Client);
      Client.Arguments = "";
      Client.ControlState = NCS_START;
      continue;
    }

    // unwanted characters (control ascii and high-ascii)
    if (NextChr < 0x20 || NextChr > 0x7F)
      continue;

    //space seperator
    if (NextChr == ' ')
    {
      if (Client.ControlState == NCS_START) //initial spaces
        continue;

      if (Client.ControlState == NFS_COMMAND) //initial spaces
      {
        Client.ControlState = NFS_ARGUMENTS;
        Client.Arguments = "";
        continue;  
      }
    }
    
    if (Client.ControlState == NCS_START)
    {
      Client.ControlState = NFS_COMMAND;
      Client.Command = "";
    }

    if (Client.ControlState == NFS_COMMAND)
      Client.Command += (char) NextChr;      

    if (Client.ControlState == NFS_ARGUMENTS)
      Client.Arguments += (char) NextChr;      
  }
}

String DFC_ESP7266FtpServer::GetFirstArgument(SClientInfo& Client)
{
  int32_t Start = 0;
  int32_t End = Client.Arguments.length();
  for (int32_t i = End-1; i>= 0; i--)
  {
    if (Client.Arguments.charAt(i) == ' ')
      End = i;
  }

  return Client.Arguments.substring(Start, End);
}

String DFC_ESP7266FtpServer::ConstructPath(SClientInfo& Client)
{
  String Path = GetFirstArgument(Client);
  Path.replace("\\", "/");

  if (Client.Arguments.length() == 0)
    Path = "/";
  else if (!Client.Arguments.startsWith("/")) //its a absolute path
    Path = Client.CurrentPath + Path;

  Path += "/";
  while (Path.indexOf("//") >= 0)
    Path.replace("//", "/");

  return Path;
}

// return false if filepath is not in current dir
// Stores the filename or subdirectory in FileName.
bool DFC_ESP7266FtpServer::GetFileName(String CurrentDir, String FilePath, String& FileName, bool& IsDir)
{
  int32_t FilePathSize = FilePath.length();
  int32_t CurrentDirSize = CurrentDir.length();
  
  if (FilePathSize <= CurrentDirSize)
    return false;

  if (FilePath.indexOf(CurrentDir) != 0)
    return false;

  int32_t NextSlash = FilePath.indexOf('/', CurrentDirSize);
  
  //Check if there's more after the slash.
  //but if it's ends on a slash it's still a file with it's name ending on a /
  //That's SPIFFS specific, since it does not has directories.
  if (NextSlash < 0 || FilePathSize == NextSlash+1)
  {
    FileName = FilePath.substring(CurrentDirSize);
    IsDir = false;
  }
  else
  {
    FileName = FilePath.substring(CurrentDirSize, NextSlash);
    IsDir = true;
  }
  
  return true;
}

bool DFC_ESP7266FtpServer::GetParentDir(String FilePath, String& ParentDir)
{
  String Path = FilePath;
  Path.replace("\\", "/");
  while (Path.indexOf("//") >= 0)
    Path.replace("//", "/");

  if (Path.endsWith("/"))
    Path.remove(Path.length()-1);

  int32_t Pos = -1;
  int32_t LastSlash = Path.indexOf('/');
  while (LastSlash >= 0)
  {
    DBGF("LastSlash \"%d\"\r\n", LastSlash);
    Pos = LastSlash;
    LastSlash = Path.indexOf('/', LastSlash+1);
  }

  if (Pos < 0)
  {
    ParentDir = FilePath;
    return false;
  }

  ParentDir = FilePath.substring(0, Pos+1);  
  return true;
}

int32_t DFC_ESP7266FtpServer::GetNextDataPort()
{
  int32_t NewPort = mLastDataPort;
  while (1)
  {
    NewPort++;
    if (NewPort >= FTP_DATA_PORT_END)
      NewPort = FTP_DATA_PORT_START;

    bool PortInUse = false;
    for (int32_t i=0; i<FTP_MAX_CLIENTS; i++)
    {
      if (mClientInfo[i].InUse && mClientInfo[i].PasvListenPort == NewPort)
        PortInUse = true;
    }

    if (!PortInUse)
      return NewPort;

    if (NewPort == mLastDataPort) //all possible ports are in use
     return 0;
  }
}

void DFC_ESP7266FtpServer::ProcessCommand(SClientInfo& Client)
{
  //preprocess
  Client.Command.trim();
  Client.Arguments.trim();
  const String& cmd(Client.Command); //for easy access

  DBG("Command: ");
  DBG(cmd.c_str());
  DBG(" ");
  DBGLN(Client.Arguments.c_str());

  if (cmd.equalsIgnoreCase("USER") && Process_USER(Client)) return;
  if (cmd.equalsIgnoreCase("PASS") && Process_PASS(Client)) return;
  if (Client.FtpState < NFS_WAITFORCOMMAND)
  {
    DBG("Command Refused. Not logged in.");
    Client.ClientConnection.println( "530 Login needed.");
    return;
  }

  if (cmd.equalsIgnoreCase("QUIT") && Process_QUIT(Client)) return;
  if (cmd.equalsIgnoreCase("SYST") && Process_SYST(Client)) return;
  if (cmd.equalsIgnoreCase("FEAT") && Process_FEAT(Client)) return;
  if (cmd.equalsIgnoreCase("HELP") && Process_HELP(Client)) return;
  if (cmd.equalsIgnoreCase("PWD") && Process_PWD(Client)) return;
  if (cmd.equalsIgnoreCase("CDUP") && Process_CDUP(Client)) return;
  if (cmd.equalsIgnoreCase("CWD") && Process_CWD(Client)) return;
  if (cmd.equalsIgnoreCase("MKD") && Process_MKD(Client)) return;
  if (cmd.equalsIgnoreCase("RMD") && Process_RMD(Client)) return;
  if (cmd.equalsIgnoreCase("TYPE") && Process_TYPE(Client)) return;
  if (cmd.equalsIgnoreCase("PASV") && Process_PASV(Client)) return;
  if (cmd.equalsIgnoreCase("PORT") && Process_PORT(Client)) return;
  if (cmd.equalsIgnoreCase("LIST") && Process_DataCommand_Preprocess(Client, NTC_LIST)) return;

  Client.ClientConnection.println( "Command not known.");
  Client.ClientConnection.printf( "500 Unknown command %s.\n\r", cmd.c_str());
}

bool DFC_ESP7266FtpServer::Process_USER(SClientInfo& Client)
{
  if (Client.FtpState > NFS_WAITFORPASSWORD)
  {
    Client.ClientConnection.println( "530 Changing user is not allowed.");
    return true;
  }
  
  //check if user exists
  bool UsernameOK(false);
  if (mServerUsername.length() == 0) //we accept every username
    UsernameOK = true;
  else if (mServerUsername.equalsIgnoreCase(Client.Arguments))
    UsernameOK = true;

  //username is wrong, we request password, but always will be rejected.
  if (!UsernameOK)
  {
    Client.ClientConnection.println( "331 OK. Password required");
    Client.FtpState = NFS_WAITFORPASSWORD_USER_REJECTED;
    return true;
  }

  if (mServerPassword.length() == 0) //we don't need a password
  {
    Client.ClientConnection.println( "230 OK.");
    Client.FtpState = NFS_WAITFORCOMMAND;
    return true;    
  }
  else
  {
    Client.ClientConnection.println( "331 OK. Password required");
    Client.FtpState = NFS_WAITFORPASSWORD;
    return true;
  }
}

bool DFC_ESP7266FtpServer::Process_PASS(SClientInfo& Client)
{
  if (Client.FtpState <= NFS_WAITFORUSERNAME)
  {
    Client.ClientConnection.println( "503 Login with USER first.");
    return true;
  }
  if (Client.FtpState > NFS_WAITFORPASSWORD)
  {
    Client.ClientConnection.println( "230 Already logged in.");
    return true;
  }

  if (Client.FtpState == NFS_WAITFORPASSWORD_USER_REJECTED || !mServerPassword.equals(Client.Arguments))
  {
    Client.ClientConnection.println( "530 Username/Password wrong.");
    DisconnectClient(Client);
    return true;
  }
  
  Client.ClientConnection.println( "230 Logged in.");
  Client.FtpState = NFS_WAITFORCOMMAND;
  return true;
}

bool DFC_ESP7266FtpServer::Process_QUIT(SClientInfo& Client)
{
  DisconnectClient(Client);
  return true;  
}

bool DFC_ESP7266FtpServer::Process_SYST(SClientInfo& Client)
{
  Client.ClientConnection.println( "215 UNIX esp8266.");
  return true;
}

bool DFC_ESP7266FtpServer::Process_FEAT(SClientInfo& Client)
{
  Client.ClientConnection.println( "211 No Features.");
  return true;
}


bool DFC_ESP7266FtpServer::Process_HELP(SClientInfo& Client)
{
  Client.ClientConnection.println( "214 Ask the whizzkid for help.");
  return true;
}

bool DFC_ESP7266FtpServer::Process_PWD(SClientInfo& Client)
{
  Client.ClientConnection.printf( "257 \"%s\" Current Directory.\r\n", Client.CurrentPath.c_str());
  return true;
}

bool DFC_ESP7266FtpServer::Process_CDUP(SClientInfo& Client)
{
  String NewPath;
  GetParentDir(Client.CurrentPath, NewPath);

  Client.CurrentPath = NewPath;
  Client.ClientConnection.printf( "250 Directory successfully changed.\r\n");  
  return true;
}

//Because directories are simulated, we accept CWD to 
//dir that does not "exists".
bool DFC_ESP7266FtpServer::Process_CWD(SClientInfo& Client)
{
  String NewPath = ConstructPath(Client);
  
  Client.CurrentPath = NewPath;
  Client.ClientConnection.printf( "250 Directory successfully changed.\r\n");  
  DBGLN("New Directory: " + NewPath);
  return true;
}

bool DFC_ESP7266FtpServer::Process_MKD(SClientInfo& Client)
{
  String NewPath = ConstructPath(Client);

  Client.ClientConnection.printf( "257 \"%s\" is created.\n\r", NewPath.c_str());

  return true;
}

//we never accept directory removal. If all
//files are removed, directorie is removed automatically
bool DFC_ESP7266FtpServer::Process_RMD(SClientInfo& Client)
{
  Client.ClientConnection.println( "550 Directories can not be removed. It is removed automatically when it's empty.");

  return true;
}

bool DFC_ESP7266FtpServer::Process_TYPE(SClientInfo& Client)
{
  const String& arg(Client.Arguments); //for easy access

  if (arg.equalsIgnoreCase("A"))
    Client.ClientConnection.println( "200 TYPE is now ASII.");
  else if (arg.equalsIgnoreCase("I"))
    Client.ClientConnection.println( "200 TYPE is now 8-bit binary.");
  else
    Client.ClientConnection.println( "504 Unknow TYPE.");

  return true;
}

bool DFC_ESP7266FtpServer::Process_PASV(SClientInfo& Client)
{  
  IPAddress LocalIP = Client.ClientConnection.localIP();
  if (Client.PasvListenServer == NULL)
  {
    Client.PasvListenPort = GetNextDataPort();
    Client.PasvListenServer = new WiFiServer(Client.PasvListenPort);
    Client.PasvListenServer->begin();
  }
  
  int32_t PasvPort = Client.PasvListenPort;
  Client.ClientConnection.println( "227 Entering Passive Mode ("+ String(LocalIP[0]) + "," + String(LocalIP[1])+","+ String(LocalIP[2])+","+ String(LocalIP[3])+","+String( PasvPort >> 8 ) +","+String ( PasvPort & 255 )+").");
  Client.TransferMode = NTC_PASSIVE;
  return true;
}

bool DFC_ESP7266FtpServer::Process_PORT(SClientInfo& Client)
{
  //Client.TransferMode = NTC_ACTIVE;
  return false;
}

bool DFC_ESP7266FtpServer::Process_DataCommand_Preprocess(SClientInfo& Client, nTransferCommand TransferCommand)
{
  if (Client.TransferMode == NTM_UNKNOWN)
  {
    Client.ClientConnection.println( "425 Use PORT or PASV first.");
    return true;
  }
  if (Client.TransferCommand != NTC_NONE)
  {
    Client.ClientConnection.println( "450 Requested file action not taken, already an command is in process.");
    return true;
  }

  if (!Client.DataConnection.connected())
  {
    Client.ClientConnection.println( "150 Accepted data connection.");
  }
  else
  {
    Client.ClientConnection.println( "125 Data connection already open; transfer starting.");
  }

  Client.TransferCommand = TransferCommand;
  CheckData(Client);
  return true;
}

bool DFC_ESP7266FtpServer::Process_Data_LIST(SClientInfo& Client)
{
  DBGLN("Sending filelist.");

  Dir dir = SPIFFS.openDir(Client.CurrentPath);
  String FileName;
  String FilePath;
  bool IsDir;
  std::vector<String> DirList;
  while (dir.next()) 
  {    
    String FilePath = dir.fileName();
    if (GetFileName(Client.CurrentPath, FilePath, FileName, IsDir))
    {
      if (IsDir)
      {
        //check if listing is already been processed
        bool Found(false);
        for (std::vector<String>::iterator it=DirList.begin(); it!=DirList.end(); it++)
        {
          if (FileName.equals(*it))
            Found = true;
        }
        if (!Found)
        {
          Client.DataConnection.print("drw-rw-rw-    1 0        0               0 Jan 01  1970 ");
          Client.DataConnection.println(FileName);
          DirList.push_back(FileName);
        }
      }
      else
      {        
        size_t fileSize = dir.fileSize();
        Client.DataConnection.print("-rw-rw-rw-    1 0        0              22 Jan 01  1970 ");
        Client.DataConnection.println(FileName);
      }     
    }
  }

////                                0        1         2         3         4         5          
////                                12345678901234567890123456789012345678901234567890123456789
//  Client.DataConnection.println( "-rw-rw-rw-    1 0        0              22 Jan 01  1970 1MB.zip");
//  Client.DataConnection.println( "drw-rw-rw-    1 0        0               0 Jan 01  1970 SubDir");


  Client.ClientConnection.println( "226 File listing send.");
  return true;
}

