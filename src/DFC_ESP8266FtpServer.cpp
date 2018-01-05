/*
*/

//library includes
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

void DFC_ESP7266FtpServer::Init()
{
#if(0)
  DBGLN("=============================================");
  DBGLN("Testing");
  String ParentDir = "";
  String FilePath = "////";
  Help_GetParentDir(FilePath, ParentDir);
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
    if (!Help_GetEmptyClientInfo(Pos))
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
      Loop_ClientConnection(mClientInfo[i]);
    }
  }
}

void DFC_ESP7266FtpServer::Loop_ClientConnection(SClientInfo& Client)
{
  //is still connected?
  if (!Client.ClientConnection.connected())
    Help_DisconnectClient(Client);

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
    //String(Lip);
    DBG("Local:");
    DBG(Lip);
    DBG(":");
    DBG(Lport);
    DBG("  ");
    DBG(Rip);
    DBG(":");
    DBG(Rport);
    DBGLN("");
  }

  //Check for new control data
  Loop_GetControlData(Client);
  Loop_DataConnection(Client);
}

void DFC_ESP7266FtpServer::Loop_GetControlData(SClientInfo& Client)
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

      Loop_ProcessCommand(Client);
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

void DFC_ESP7266FtpServer::Loop_ProcessCommand(SClientInfo& Client)
{
  //preprocess
  Client.Command.trim();
  Client.Arguments.trim();
  const String& cmd(Client.Command); //for easy access

  DBG("Command: ");
  DBG(cmd.c_str());
  DBG(" ");
  DBGLN(Client.Arguments.c_str());

  if (cmd.equalsIgnoreCase("USER")) Process_USER(Client);
  else if (cmd.equalsIgnoreCase("PASS")) Process_PASS(Client);
  else if (Client.FtpState < NFS_WAITFORCOMMAND)
  {
    DBG("Command Refused. Not logged in.");
    Client.ClientConnection.println( "530 Login needed.");
    return;
  }    
  
  else if (cmd.equalsIgnoreCase("QUIT")) Process_QUIT(Client);
  else if (cmd.equalsIgnoreCase("SYST")) Process_SYST(Client);
  else if (cmd.equalsIgnoreCase("FEAT")) Process_FEAT(Client);
  else if (cmd.equalsIgnoreCase("HELP")) Process_HELP(Client);
  else if (cmd.equalsIgnoreCase("PWD")) Process_PWD(Client);
  else if (cmd.equalsIgnoreCase("CDUP")) Process_CDUP(Client);
  else if (cmd.equalsIgnoreCase("CWD")) Process_CWD(Client);
  else if (cmd.equalsIgnoreCase("MKD")) Process_MKD(Client);
  else if (cmd.equalsIgnoreCase("RMD")) Process_RMD(Client);
  else if (cmd.equalsIgnoreCase("TYPE")) Process_TYPE(Client);
  else if (cmd.equalsIgnoreCase("PASV")) Process_PASV(Client);
  else if (cmd.equalsIgnoreCase("PORT")) Process_PORT(Client);
  else if (cmd.equalsIgnoreCase("LIST")) Process_LIST(Client);
  else if (cmd.equalsIgnoreCase("SIZE")) Process_SIZE(Client);
  else if (cmd.equalsIgnoreCase("DELE")) Process_DELE(Client);
  else if (cmd.equalsIgnoreCase("STOR")) Process_STOR(Client);
  else if (cmd.equalsIgnoreCase("RETR")) Process_RETR(Client);
  else
  {
    Client.ClientConnection.println( "Command not known.");
    Client.ClientConnection.printf( "500 Unknown command %s.\n\r", cmd.c_str());
  }
}

void DFC_ESP7266FtpServer::Process_USER(SClientInfo& Client)
{
  if (Client.FtpState > NFS_WAITFORPASSWORD)
  {
    Client.ClientConnection.println( "530 Changing user is not allowed.");
    return;
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
    return;
  }

  if (mServerPassword.length() == 0) //we don't need a password
  {
    Client.ClientConnection.println( "230 OK.");
    Client.FtpState = NFS_WAITFORCOMMAND;
  }
  else
  {
    Client.ClientConnection.println( "331 OK. Password required");
    Client.FtpState = NFS_WAITFORPASSWORD;
  }
}

void DFC_ESP7266FtpServer::Process_PASS(SClientInfo& Client)
{
  if (Client.FtpState <= NFS_WAITFORUSERNAME)
  {
    Client.ClientConnection.println( "503 Login with USER first.");
    return;
  }
  if (Client.FtpState > NFS_WAITFORPASSWORD)
  {
    Client.ClientConnection.println( "230 Already logged in.");
    return;
  }

  if (Client.FtpState == NFS_WAITFORPASSWORD_USER_REJECTED || !mServerPassword.equals(Client.Arguments))
  {
    Client.ClientConnection.println( "530 Username/Password wrong.");
    Help_DisconnectClient(Client);
    return;
  }

  Client.ClientConnection.println( "230 Logged in.");
  Client.FtpState = NFS_WAITFORCOMMAND;
}

void DFC_ESP7266FtpServer::Process_QUIT(SClientInfo& Client)
{
  Help_DisconnectClient(Client);
}

void DFC_ESP7266FtpServer::Process_SYST(SClientInfo& Client)
{
  Client.ClientConnection.println( "215 UNIX esp8266.");
}

void DFC_ESP7266FtpServer::Process_FEAT(SClientInfo& Client)
{
  Client.ClientConnection.println( "211 No Features.");
}


void DFC_ESP7266FtpServer::Process_HELP(SClientInfo& Client)
{
  Client.ClientConnection.println( "214 Ask the whizzkid for help.");
}

void DFC_ESP7266FtpServer::Process_PWD(SClientInfo& Client)
{
  Client.ClientConnection.printf( "257 \"%s\" Current Directory.\r\n", Client.CurrentPath.c_str());
}

void DFC_ESP7266FtpServer::Process_CDUP(SClientInfo& Client)
{
  String NewPath;
  Help_GetParentDir(Client.CurrentPath, NewPath);

  Client.CurrentPath = NewPath;
  Client.ClientConnection.printf( "250 Directory successfully changed.\r\n");
}

//Because directories are simulated, we accept CWD to
//dir that does not "exists".
void DFC_ESP7266FtpServer::Process_CWD(SClientInfo& Client)
{
  String NewPath = Help_GetPath(Client, true);

  Client.CurrentPath = NewPath;
  Client.ClientConnection.printf( "250 Directory successfully changed.\r\n");
  DBGLN("New Directory: " + NewPath);
}

void DFC_ESP7266FtpServer::Process_MKD(SClientInfo& Client)
{
  Client.TempDirectory = Help_GetPath(Client, true);
  Client.ClientConnection.printf( "257 \"%s\" is created.\n\r", Client.TempDirectory.c_str());
}

//we never accept directory removal. If all
//files are removed, directorie is removed automatically
void DFC_ESP7266FtpServer::Process_RMD(SClientInfo& Client)
{
  String Directory = Help_GetPath(Client, true);
  if (Help_DirExist(Directory))
  {
    Client.ClientConnection.printf( "550 Directory \"%s\" not removed. It's not empty.\n\r", Directory.c_str());
    return;
  }

  if (Client.TempDirectory.indexOf(Directory) == 0)
    Client.TempDirectory = "";

  Client.ClientConnection.printf( "250 Directory \"%s\" is removed.\n\r", Directory.c_str());
}

void DFC_ESP7266FtpServer::Process_TYPE(SClientInfo& Client)
{
  const String& arg(Client.Arguments); //for easy access

  if (arg.equalsIgnoreCase("A"))
    Client.ClientConnection.println( "200 TYPE is now ASII.");
  else if (arg.equalsIgnoreCase("I"))
    Client.ClientConnection.println( "200 TYPE is now 8-bit binary.");
  else
    Client.ClientConnection.println( "504 Unknow TYPE.");
}

void DFC_ESP7266FtpServer::Process_PASV(SClientInfo& Client)
{
  IPAddress LocalIP = Client.ClientConnection.localIP();
  if (Client.PasvListenServer == NULL)
  {
    Client.PasvListenPort = Help_GetNextDataPort();
    Client.PasvListenServer = new WiFiServer(Client.PasvListenPort);
    Client.PasvListenServer->begin();
  }

  int32_t PasvPort = Client.PasvListenPort;
  Client.ClientConnection.println( "227 Entering Passive Mode ("+ String(LocalIP[0]) + "," + String(LocalIP[1])+","+ String(LocalIP[2])+","+ String(LocalIP[3])+","+String( PasvPort >> 8 ) +","+String ( PasvPort & 255 )+").");
  Client.TransferMode = NTC_PASSIVE;
}

void DFC_ESP7266FtpServer::Process_PORT(SClientInfo& Client)
{
  //Client.TransferMode = NTC_ACTIVE;
}

void DFC_ESP7266FtpServer::Process_LIST(SClientInfo& Client)
{
  if (!Process_DataCommand_Preprocess(Client))
    return;
  
  //Other check's and preprocessing
  
  Process_DataCommand_Responds_OK(Client, NTC_LIST);
}

void DFC_ESP7266FtpServer::Process_SIZE(SClientInfo& Client)
{
  //check filename
  String FilePath = Help_GetPath(Client, false);
  if (!SPIFFS.exists(FilePath))
  {
    Client.ClientConnection.printf( "550 File %s does not exist.\r\n", FilePath.c_str());
    return;
  }
  
  File FileHandle = SPIFFS.open(FilePath, "r");
  if (!FileHandle)
  {
    Client.ClientConnection.printf( "550 File %s could not be opened.\r\n", FilePath.c_str());
    return;
  }
  
  Client.ClientConnection.println( "213 " + String(FileHandle.size()));
  FileHandle.close();
}

void DFC_ESP7266FtpServer::Process_DELE(SClientInfo& Client)
{
  //check filename
  String FilePath = Help_GetPath(Client, false);
  if (!SPIFFS.exists(FilePath))
  {
    Client.ClientConnection.printf( "550 File %s does not exist.\r\n", FilePath.c_str());
    return;
  }

  SPIFFS.remove(FilePath);
  Client.ClientConnection.printf( "250 File %s successful deleted.\r\n", FilePath.c_str());
}

void DFC_ESP7266FtpServer::Process_STOR(SClientInfo& Client)
{
  if (!Process_DataCommand_Preprocess(Client))
    return;

  //check filename
  String FilePath = Help_GetPath(Client, false);
  if (FilePath.length() > 31)
  {
    Client.ClientConnection.printf( "550 File %s exeeds filename length of 31 characters.\r\n", FilePath.c_str());
    return;
  }

  //open file
  Client.TransferFile = SPIFFS.open(FilePath, "w");

  Process_DataCommand_Responds_OK(Client, NTC_STOR);
}

void DFC_ESP7266FtpServer::Process_RETR(SClientInfo& Client)
{
  if (!Process_DataCommand_Preprocess(Client))
    return;

  //check filename
  String FilePath = Help_GetPath(Client, false);
  if (!SPIFFS.exists(FilePath))
  {
    Client.ClientConnection.printf( "550 File %s does not exist.\r\n", FilePath.c_str());
    return;
  }

  //open file
  Client.TransferFile = SPIFFS.open(FilePath, "r");

  Process_DataCommand_Responds_OK(Client, NTC_RETR);
}

void DFC_ESP7266FtpServer::Loop_DataConnection(SClientInfo& Client)
{
  if (Client.PasvListenServer == NULL || !Client.DataConnection.connected())
  {
    if (Client.TransferCommand != NTC_NONE)       //Client disconnected during transfer
    {
       Process_DataCommand_DISCONNECTED(Client);  //Handle disconnect event
       Process_DataCommand_END(Client);           //Close data connectin, gracefully.
    }
    return;
  }
  
  if (Client.TransferCommand == NTC_LIST) Process_Data_LIST(Client);
  else if (Client.TransferCommand == NTC_STOR) Process_Data_STOR(Client);
  else if (Client.TransferCommand == NTC_RETR) Process_Data_RETR(Client);
}

bool DFC_ESP7266FtpServer::Process_DataCommand_Preprocess(SClientInfo& Client)
{
  if (Client.TransferMode == NTM_UNKNOWN)
  {
    Client.ClientConnection.println( "425 Use PORT or PASV first.");
    return false;
  }
  if (Client.TransferCommand != NTC_NONE)
  {
    Client.ClientConnection.println( "450 Requested file action not taken, already an command is in process.");
    return false;
  }
  return true;
}

void DFC_ESP7266FtpServer::Process_DataCommand_Responds_OK(SClientInfo& Client, nTransferCommand TransferCommand)
{
  if (!Client.DataConnection.connected())
    Client.ClientConnection.println( "150 Accepted data connection.");
  else
    Client.ClientConnection.println( "125 Data connection already open; transfer starting.");

  Client.TransferCommand = TransferCommand;
  Loop_DataConnection(Client);
}

void DFC_ESP7266FtpServer::Process_DataCommand_END(SClientInfo& Client)
{
  DBGLN("Data send/received. Closing data connection.");
  Client.DataConnection.flush();
  Client.DataConnection.stop();
  Client.TransferCommand = NTC_NONE;
}

void DFC_ESP7266FtpServer::Process_DataCommand_DISCONNECTED(SClientInfo& Client)
{
  switch (Client.TransferCommand)
  {
    //Data connection lost before we send all data. It's a abort
    case NTC_LIST:
    case NTC_RETR:
      Client.ClientConnection.println( "426 Data connection unexpectacly disconnected. transfer aborted.");
      break;

    //Data connection disconnected during receiving data.
    //Implicit this is a signal all data is send from client.
    //So it is a successfull transfer.
    case NTC_STOR:
      Client.ClientConnection.println( "226 Connection closed; transfer successful.");
      break;
  }
}

////                               0         1         2         3         4         5          
////                               012345678901234567890123456789012345678901234567890123456789
//  Client.DataConnection.println( "-rw-rw-rw-    1 0        0              22 Jan 01  1970 1MB.zip");
//  Client.DataConnection.println( "drw-rw-rw-    1 0        0               0 Jan 01  1970 SubDir");
void DFC_ESP7266FtpServer::Process_Data_LIST(SClientInfo& Client)
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
    if (Help_GetFileName(Client.CurrentPath, FilePath, FileName, IsDir))
    {
      if (IsDir)
      {
        if (Help_CheckIfDirIsUnique(DirList, FileName))
        {
          Client.DataConnection.print("drw-rw-rw-    1 0        0               0 Jan 01  1970 ");
          Client.DataConnection.println(FileName);
        }
      }
      else
      {
        //Client.DataConnection.print("-rw-rw-rw-    1 0        0              22 Jan 01  1970 ");
        String FileSizeString = String(dir.fileSize());
        String WhiteSpaceString = String("              ");
        WhiteSpaceString = WhiteSpaceString.substring(FileSizeString.length());
        
        Client.DataConnection.print("-rw-rw-rw-    1 0        0  ");
        Client.DataConnection.print(WhiteSpaceString);
        Client.DataConnection.print(FileSizeString);        
        Client.DataConnection.print(" Jan 01  1970 ");
        Client.DataConnection.println(FileName);
      }
    }
  }
  
  //Append the TempDirectory created at MKD if we need to add it.
  //Help_GetFileName wil report it's a file, but we are sure it's a directory.
  //we ignore IsDir.
  if (Help_GetFileName(Client.CurrentPath, Client.TempDirectory, FileName, IsDir))
  {
    if (FileName.endsWith("/"))
      FileName.remove(FileName.length()-1);
    if (Help_CheckIfDirIsUnique(DirList, FileName))
    {
      Client.DataConnection.print("drw-rw-rw-    1 0        0               0 Jan 01  1970 ");
      Client.DataConnection.println(FileName);
    }
  }

  Client.ClientConnection.println( "226 File listing send.");
  Process_DataCommand_END(Client);
}

void DFC_ESP7266FtpServer::Process_Data_STOR(SClientInfo& Client)
{
  uint8_t Data[256];  
  int32_t BytesRead = Client.DataConnection.read(Data, 256);
  if (BytesRead > 0)
  {
    Client.TransferFile.write(Data, BytesRead);
  }
}

void DFC_ESP7266FtpServer::Process_Data_RETR(SClientInfo& Client)
{
  uint8_t Data[256];
  int32_t BytesRead = Client.TransferFile.read(Data, 256);
  if (BytesRead > 0)
  {
    Client.DataConnection.write(Data, BytesRead);
  }
  else
  {
    Process_DataCommand_END(Client);
    Client.ClientConnection.println( "226 File transfer done.");
  }
}

bool DFC_ESP7266FtpServer::Help_GetEmptyClientInfo(int32_t& Pos)
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

int32_t DFC_ESP7266FtpServer::Help_GetNextDataPort()
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

void DFC_ESP7266FtpServer::Help_DisconnectClient(SClientInfo& Client)
{
  if (Client.ClientConnection.connected())
    Client.ClientConnection.println("221 Goodbye");

  Client.Reset();
}

String DFC_ESP7266FtpServer::Help_GetFirstArgument(SClientInfo& Client)
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

String DFC_ESP7266FtpServer::Help_GetPath(SClientInfo& Client, bool IsPath)
{
  String Path = Help_GetFirstArgument(Client);
  Path.replace("\\", "/");

  if (Path.length() == 0)
    Path = "/";
  else if (!Path.startsWith("/")) //its a absolute path
    Path = Client.CurrentPath + Path;

  if (IsPath)
    Path += "/";
 
  while (Path.indexOf("//") >= 0)
    Path.replace("//", "/");

  return Path;
}

/* Help_GetFileName

Input values
  CurrentDir : Full path of current directory to find files in.
  FilePath   : Full path of file to get name from.

output values
  FileName : Name of file or subdirectory. without path
  IsDir    : If FileName is a subdirectory (or an file)

Return value
  true  : Name found
  false : FilePath is file or subdirectory in CurrentDir
*/
bool DFC_ESP7266FtpServer::Help_GetFileName(String CurrentDir, String FilePath, String& FileName, bool& IsDir)
{
  int32_t FilePathSize = FilePath.length();
  int32_t CurrentDirSize = CurrentDir.length();

  if (FilePathSize <= CurrentDirSize)
    return false;

  if (FilePath.indexOf(CurrentDir) != 0)
    return false;

  int32_t NextSlash = FilePath.indexOf('/', CurrentDirSize);

  if (NextSlash < 0 || FilePathSize == NextSlash+1) //+1 is correct. We want the '/' if FILENAME ends with a '/'. 
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

bool DFC_ESP7266FtpServer::Help_GetParentDir(String FilePath, String& ParentDir)
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

//If directory exist, it's automatically not emtpy, because Empty directories do not exist.
//DirPath needs to be absolute
bool DFC_ESP7266FtpServer::Help_DirExist(String DirPath)
{
  Dir FindDir = SPIFFS.openDir("/");
  while (FindDir.next())
  {
    String FilePath = FindDir.fileName();
    if (FilePath.indexOf(DirPath) == 0 && FilePath.length() > DirPath.length())
      return true;
  }  
  return false;
}

//creates a list with unique names. If a name is already present in list,
//it returns false.
bool DFC_ESP7266FtpServer::Help_CheckIfDirIsUnique(std::vector<String>& DirList, const String& Name)
{
  bool Found(false);
  for (std::vector<String>::iterator it=DirList.begin(); it!=DirList.end(); it++)
  {
    if (Name.equals(*it))
      Found = true;
  }

  if (!Found)
  {
    DirList.push_back(Name);
  }
  
  return !Found;
}