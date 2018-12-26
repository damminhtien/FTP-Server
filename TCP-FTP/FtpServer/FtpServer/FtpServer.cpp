#include "FtpServer.h"
#include "string"
#include "mystring.h"
#include "direct.h"

Client client[MAX_CLIENTS];
bool status = true;
int connections = 0;
int dbug;
CRITICAL_SECTION CriticalSection;


FtpServer::FtpServer(){
	int debug;

	// Load basic starting routine  (WSAstartup & local hostname)
	Ftp::Init();

	/* Zero out structure */
	memset(&ServerAddr, 0, sizeof(ServerAddr));      
	
	ServerAddr.sin_family		=	AF_INET;					 /* Internet address family */
	ServerAddr.sin_addr.s_addr	=	htonl(INADDR_ANY);			/* Any incoming interface */
	ServerAddr.sin_port			=	htons(LISTEN_ON_PORT);         /* Local port */
	
	// Create the socket
	sock = Ftp::createSocket();

	// Bind the server socket
	debug = bind(sock,(struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) ;

	cout << "\nBinding socket:" << debug << endl;
	if(debug != 0)
		Ftp::err_sys("BIND FAILED",WSAGetLastError());
	else
		cout<<"Socket Bound to port : "<< LISTEN_ON_PORT << endl;
	
	// Successfull bind, now listen for Server requests.
	debug = listen(sock,5);
	if (debug < 0)
		Ftp::err_sys("Listen error :(");
	else
		cout<< "Server Listen mode : " << debug << endl;

	cout << "Waiting for clients!\n";
}


FtpServer::~FtpServer(){
	WSACleanup();
}


void FtpServer::accept_new_clients(){
	for(int i=0; i < MAX_CLIENTS; i++)	{
		// if open slot
		if( !(client[i].conn)) {
			// if accept client works
			if(accept_client(&client[i])) {
				client[i].runner = true;
				updateStatus(CLIENT_CONN);
				CreateThread(0, 0, ReceiveCmds, (LPVOID) &client[i], 0, 0);
			}
		}
	}
}


bool FtpServer::accept_client(Client *cli) {
	cli->i = sizeof(sockaddr); // save address

	cli->cs = accept(sock, (struct sockaddr *)&cli->addr, &cli->i);

	if (cli->cs != 0 && cli->cs != SOCKET_ERROR) {
		cli->conn = true;
		FD_ZERO(&cli->set);
		FD_SET(cli->cs, &cli->set);
		return true;
	}
	return false;
}


bool FtpServer::check_server_status(){
	return status;
}


void FtpServer::updateStatus(int msg) {
	if (msg == CLIENT_CONN) {
		connections++;
		cout << "\nWow, a client has connected. There are now " << connections << " clients connected.";
	}
	else if (msg == CLIENT_DISC) {
		connections--;
		cout << "\nWee, a client has disconnected. There are now " << connections << " clients connected.";
	}
	else {
		//never leave out anything
		cout << "\n>>>>>>We got an unknown message :" << msg;
	}
}


bool FtpServer::disconnectClient(Client *cli) {
	if (cli->cs)
		closesocket(cli->cs);
	cli->conn = false;
	cli->i = -1;
	updateStatus(CLIENT_DISC);
	return true;
}


void FtpServer::notiCmd(bool flag, char* cmd) {
	if (cmd)
		cout << "\nDone command '" << cmd << "'";
	else 
		cout << "\nError when process '" << cmd << "'";
}


DWORD WINAPI FtpServer::ReceiveCmds(LPVOID lpParam) {
	Client cli = *(Client*)lpParam;
	checkLogin(&cli);
	while (cli.runner) {
		cli.runner = FtpServer::handleFrame(&cli);
	}
	return 1;
}


bool FtpServer::handleFrame(Client* cli) {
	char data[64], cmd[16], arg[48];
	dbug = recv(cli->cs, data, sizeof(data), 0);
	if (dbug <= 0)
	{
		updateStatus(CLIENT_DISC);
		closesocket(cli->cs);
		return 0;
	}
	data[dbug] = 0;
	cout << "\nReceived: " << data;
	sscanf(data, "%s %s", cmd, arg);
	cout << "\nCommand: " << cmd << "\tArguments: " << arg;

	if (strcmp(cmd, "!") == 0) notiCmd(checkOS(cli), cmd);
	if (strcmp(cmd, "?") == 0) notiCmd(listCmd(cli), cmd);
	if (strcmp(cmd, "adduser") == 0) notiCmd(addUser(cli, arg), cmd);
	if (strcmp(cmd, "deluser") == 0) notiCmd(delUser(cli, arg), cmd);
	if (strcmp(cmd, "updateuser") == 0) notiCmd(updateUser(cli, arg), cmd);
	if (strcmp(cmd, "put") == 0) notiCmd(doUpload(cli, arg), cmd);
	if (strcmp(cmd, "get") == 0) notiCmd(doDownload(cli, arg), cmd);
	if (strcmp(cmd, "rename") == 0) notiCmd(renamefile(cli, arg), cmd);
	if (strcmp(cmd, "delete") == 0) notiCmd(deletefile(cli, arg), cmd);
	if (strcmp(cmd, "mk") == 0) notiCmd(makefile(cli, arg), cmd);
	if (strcmp(cmd, "mkdir") == 0) notiCmd(makefolder(cli, arg), cmd);
	if (strcmp(cmd, "pwd") == 0) notiCmd(showCurDir(cli), cmd);
	if (strcmp(cmd, "renamedir") == 0) notiCmd(renamefolder(cli, arg), cmd);
	return true;
}


void FtpServer::checkLogin(Client* cli) {
	int flag = 1;
	while (flag) {
		send(cli->cs, "\nWelcome to connect FTPServer!\nPlease type your username and password to continue!\nUsername: ", sizeof(char) * 94, 0);
		char username[64];
		dbug = recv(cli->cs, username, sizeof(username), 0);
		if (dbug <= 0)
		{
			updateStatus(CLIENT_DISC);
			closesocket(cli->cs);
			cli->runner = false;
			break;
		}
		username[dbug] = 0;
		send(cli->cs, "Password: ", sizeof(char) * 12, 0);
		char password[64];
		dbug = recv(cli->cs, password, sizeof(password), 0);
		if (dbug <= 0)
		{
			updateStatus(CLIENT_DISC);
			closesocket(cli->cs);
			cli->runner = false;
			break;
		}
		password[dbug] = 0;
		InitializeCriticalSection(&CriticalSection);
		EnterCriticalSection(&CriticalSection);
		char filename[] = "c:\\Users\\USER\\Desktop\\FTP-Server\\TCP-FTP\\FtpServer\\FtpServer\\db.txt"; 
		//open and get the file handle
		FILE* fh;
		fopen_s(&fh, filename, "r");
		//check if file exists
		if (fh == NULL) {
			cout << "File db is missing %s" << filename;
			break;
		}
		//read line by line
		const size_t line_size = 255;
		char* line = (char*)malloc(line_size);
		while (fgets(line, line_size, fh) != NULL) {
			int len = strlen(line);
			//cout << line;
			for (int i = 0; i < len; i++) {
				if (line[i] == 32) {
					char c_username[31];
					memcpy(c_username, &line[0], i);
					c_username[i] = '\0';
					char c_password[31];
					memcpy(c_password, &line[i + 1], len - i - 2);
					c_password[len - i - 2] = '\0';
					if (strcmp(username, c_username) == 0 && strcmp(password, c_password) == 0) {
						flag = 0;
						send(cli->cs, "ok", sizeof(char) * 3, 0);
						break;
					}
				}
			}
		}
		fclose(fh);
		free(line);
		LeaveCriticalSection(&CriticalSection);
	}
}


bool FtpServer::addUser(Client* cli, char* arg) {
	string username, password;
	string tmp = arg;
	username = tmp.substr(0, tmp.find(":"));
	password = tmp.substr(tmp.find(":") + 1, tmp.length());
	ofstream os("c:\\Users\\USER\\Desktop\\FTP-Server\\TCP-FTP\\FtpServer\\FtpServer\\db.txt", ios_base::app | ios_base::out);
	os << username << " " << password << "\n";
	send(cli->cs, "\nAdd new user successfully", sizeof(char) * 27, 0);
	return 1;
}


bool FtpServer::delUser(Client* cli, char* arg) {
	string username, password;
	string tmp = arg;
	username = tmp.substr(0, tmp.find(":"));
	password = tmp.substr(tmp.find(":") + 1, tmp.length());
	username.append(" ");
	username.append(password);
	
	std::string line;
	std::ifstream fin;
	string path = "c:\\Users\\USER\\Desktop\\FTP-Server\\TCP-FTP\\FtpServer\\FtpServer\\db.txt";
	fin.open(path);
	std::ofstream temp; // contents of path must be copied to a temp file then renamed back to the path file
	temp.open("temp.txt");
	while (getline(fin, line)) {
		if (line != username) // write all lines to temp other than the line marked fro erasing
			temp << line << std::endl;
	}
	temp.close();
	fin.close();
	const char * p = path.c_str(); // required conversion for remove and rename functions
	remove(p);
	rename("temp.txt", p);

	send(cli->cs, "\nDelete user successfully", sizeof(char) * 27, 0);
	return 1;
}


bool FtpServer::updateUser(Client* cli, char* arg) {
	string newacc, oldacc, newusername, newpass, oldusername, oldpass;
	string tmp = arg;
	oldacc = tmp.substr(0, tmp.find("-"));
	newacc = tmp.substr(tmp.find("-") + 1, tmp.length());
	
	newusername = newacc.substr(0, newacc.find(":"));
	newpass = newacc.substr(newacc.find(":") + 1, newacc.length());
	oldusername = oldacc.substr(0, oldacc.find(":"));
	oldpass = oldacc.substr(oldacc.find(":") + 1, oldacc.length());
	oldusername.append(" ");
	oldusername.append(oldpass);

	std::string line;
	std::ifstream fin;
	string path = "c:\\Users\\USER\\Desktop\\FTP-Server\\TCP-FTP\\FtpServer\\FtpServer\\db.txt";
	fin.open(path);
	std::ofstream temp; 
	temp.open("temp.txt");
	while (getline(fin, line)) {
		if (line != oldusername) 
			temp << line << std::endl;
	}
	temp.close();
	fin.close();
	const char * p = path.c_str(); 
	remove(p);
	rename("temp.txt", p);

	ofstream os("c:\\Users\\USER\\Desktop\\FTP-Server\\TCP-FTP\\FtpServer\\FtpServer\\db.txt", ios_base::app | ios_base::out);
	os << newusername << " " << newpass << "\n";

	send(cli->cs, "\nUpdate user information successfully", sizeof(char) * 38, 0);
	return 1;
}


bool FtpServer::doUpload(Client* cli, char* filename) {
	send(cli->cs, "\nData stream to upload established", sizeof(char)*35, 0);
	char data[64];
	dbug = recv(cli->cs, data, sizeof(data), 0);
	if (dbug <= 0)
	{
		updateStatus(CLIENT_DISC);
		closesocket(cli->cs);
		cli->runner = false;
		return false;
	}
	data[dbug] = 0;
	if (strcmp(data, "ok") == 0) send(cli->cs, filename, strlen(filename), 0);
	else return false;
	recvFileAndWrite(filename, cli->cs);
	return true;
}


bool FtpServer::doDownload(Client* cli, char* filename) {
	send(cli->cs, "\nData stream to download established", sizeof(char) * 37, 0);
	char data[64];
	dbug = recv(cli->cs, data, sizeof(data), 0);
	data[dbug] = 0;
	if (dbug <= 0)
	{
		updateStatus(CLIENT_DISC);
		closesocket(cli->cs);
		cli->runner = false;
		return false;
	}
	if (strcmp(data, "ok") == 0) send(cli->cs, filename, strlen(filename), 0);
	else return false;
	readFileAndSend(filename, cli->cs);
	return 0;
}


bool FtpServer::checkOS(Client *cli) {
	OSVERSIONINFO os;
	ZeroMemory(&os, sizeof(OSVERSIONINFO));
	os.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
	GetVersionEx(&os);
	std::string ret = "Serverside OS: Windows ";
	if (os.dwMajorVersion == 10)
		ret += "10";
	else if (os.dwMajorVersion == 6) {
		if (os.dwMinorVersion == 3)
			ret += "8.1";
		else if (os.dwMinorVersion == 2)
			ret += "8";
		else if (os.dwMinorVersion == 1)
			ret += "7";
		else
			ret += "Vista";
	}
	else if (os.dwMajorVersion == 5) {
		if (os.dwMinorVersion == 2)
			ret += "XP SP2";
		else if (os.dwMinorVersion == 1)
			ret += "XP";
	}
	send(cli->cs, ret.c_str(), sizeof(ret), 0);
	return true;
}


bool FtpServer::listCmd(Client *cli) {
	string list = "!\t: get system OS's information.";
	list.append("\n?\t: list support commands.");
	list.append("\npwd\t: show location in ftp server.");
	list.append("\nadduser\t: add new user account.");
	list.append("\ndeluser\t: delete a user account");
	list.append("\nupdateuser\t: change a user-account's information");
	list.append("\nls\t: list information about files..");
	list.append("\ndir\t: briefly list directory contents.");
	list.append("\nmk\t: create a file, if it do not already exist.");
	list.append("\nmkdir\t: create a folder, if it do not already exist.");
	list.append("\nget\t: download a file from ftp server.");
	list.append("\nput\t: upload a file to server.");
	list.append("\ndelete\t: delete a file in server.");
	list.append("\nrename\t: rename a file in server .");
	list.append("\nrenamedir\t: rename a folder in server .");
	list.append("\nbye\t: disconnect server.");
	send(cli->cs,(char*) list.c_str(), list.size(), 0);
	return true;
}


bool FtpServer::renamefile(Client *cli, char* arg) {
	string oldname, newname;
	string tmp = arg;
	oldname = tmp.substr(0, tmp.find(":"));
	newname = tmp.substr(tmp.find(":")+1, tmp.length());
	int value = rename(oldname.c_str(), newname.c_str());
	if (!value)
	{
		send(cli->cs, "\nFile name changed successfully" , sizeof(char)*32, 0);
		cout << "\nFile name changed successfully";
		return 1;
	}
	send(cli->cs, "\nError when rename", sizeof(char) * 19, 0);
	perror("Error");
	return 0;
}


bool FtpServer::deletefile(Client *cli, char* filename) {
	if (remove(filename) != 0) {
		perror("Error deleting file");
		send(cli->cs, "\nDelete file failed", sizeof(char) * 20, 0);
		return 0;
	}
	send(cli->cs, "\nDelete file successfully", sizeof(char) * 26, 0);
	return 1;
}


bool FtpServer::makefile(Client* cli, char* arg) {
	string filename, content;
	string tmp = arg;
	filename = tmp.substr(0, tmp.find(":"));
	content = tmp.substr(tmp.find(":") + 1, tmp.length());
	ofstream os(filename);
	os << content << endl;
	os.close();
	send(cli->cs, "\nCreate new file successfully", sizeof(char) * 30, 0);
	return 1;
}


bool FtpServer::showCurDir(Client *cli) {
	char *path = NULL;
	size_t size;
	path = getcwd(path, size);
	send(cli->cs, path, strlen(path), 0);
	return 1;
}


bool FtpServer::makefolder(Client* cli, char* foldername) {
	mkdir(foldername);
	send(cli->cs, "\nCreate new folder successfully", sizeof(char) * 32, 0);
	return 1;
}


bool FtpServer::renamefolder(Client *cli, char* arg) {
	string oldname, newname;
	string tmp = arg;
	oldname = tmp.substr(0, tmp.find(":"));
	newname = tmp.substr(tmp.find(":") + 1, tmp.length());
	int value = rename(oldname.c_str(), newname.c_str());
	if (!value)
	{
		send(cli->cs, "\nFolder name changed successfully", sizeof(char) * 34, 0);
		cout << "\nFolder name changed successfully";
		return 1;
	}
	send(cli->cs, "\nError when rename", sizeof(char) * 19, 0);
	perror("Error");
	return 0;
}
