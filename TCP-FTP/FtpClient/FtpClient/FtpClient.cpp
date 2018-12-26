#include "FtpClient.h"
#include "string"
#include "mystring.h"


FtpClient::FtpClient(){
	// Load basic starting routine  (WSAstartup & local hostname)
	Ftp::Init();

	// Create the socket
	sock = Ftp::createSocket();
	
	cout << "Please enter the hostname to connect to: ";
	cin >> remoteHostname;

	//connect to the server
	memset(&ServerAddr, 0, sizeof(ServerAddr));     /* Zero out structure */

	ServerAddr.sin_family      = AF_INET;             /* Internet address family */
	ServerAddr.sin_addr.s_addr = ResolveName(remoteHostname);   /* Server IP address */
	ServerAddr.sin_port        = htons(CONNECT_ON_PORT); /* Server port */

	if (connect(sock, (struct sockaddr *) &ServerAddr, sizeof(ServerAddr)) < 0)
		err_sys("Socket Creating Error");
	cout << "\nConnected to the server successfully\n";
}


FtpClient::~FtpClient(){
	WSACleanup();
}


bool FtpClient::run(){
	char buf[1024];
	bool running = true;
	bool logging = true;
	cin.ignore(256, '\n');

	while (logging) {
		debug = recv(sock, buf, sizeof(buf), 0);
		buf[debug] = 0;
		cout << buf;
		string username;
		fflush(stdin);
		getline(cin, username);
		//cin.ignore(256, '\n');
		send(sock, username.c_str(), strlen(username.c_str()), 0);
		debug = recv(sock, buf, sizeof(buf), 0);
		buf[debug] = 0;
		cout << buf;
		string password;
		fflush(stdin);
		getline(cin, password);
		//cin.ignore(256, '\n');
		send(sock, password.c_str(), strlen(password.c_str()), 0);
		debug = recv(sock, buf, sizeof(buf), 0);
		buf[debug] = 0;
		cout << buf;
		if (strcmp(buf, "ok") == 0) break;
	}

	while(running){
		cout << "\nFTP > ";
		string cmd;
		fflush(stdin);
		getline(cin, cmd);
		//cin.ignore(256, '\n');
		send(sock, cmd.c_str(), strlen(cmd.c_str()), 0);
		debug = recv(sock, buf, sizeof(buf), 0);
		buf[debug] = 0;
		if (strcmp(buf, "\nData stream to upload established") == 0) {
			cout << "Uploading...";
			send(sock, "ok", sizeof(char)*3, 0);
			debug = recv(sock, buf, sizeof(buf), 0);
			buf[debug] = 0;
			Ftp::readFileAndSend(buf, sock);
		}
		if (strcmp(buf, "\nData stream to download established") == 0) {
			cout << "\nDownloading...";
			send(sock, "ok", sizeof(char) * 3, 0);
			debug = recv(sock, buf, sizeof(buf), 0);
			buf[debug] = 0;
			Ftp::recvFileAndWrite(buf, sock);
		}
		cout << buf;
	}
	cin >> debug;
	return 1;
}


bool FtpClient::doUpload(Frame f){
	if (!file_exists(f.filename))
		err_sys("That file doesn't exist!\n");
	return 1;
}

bool FtpClient::doDownload(Frame f){
	return 1;
}


bool FtpClient::doShutdown(){
	cout << "\nClosing! <3";

	// shutdown the send half of the connection since no more data will be sent
	if (shutdown(sock, 2) == SOCKET_ERROR) {
		printf("shutdown failed: %d\n", WSAGetLastError());
		closesocket(sock);
		WSACleanup();
		return 1;
	}

	// cleanup
	closesocket(sock);
	WSACleanup();
	return 0;
}


int FtpClient::toCommandCode(string msg) {
	msg = toLowerStr(msg);
	if (msg == "!")						return 1;
	if (msg == "?")						return 2;
	if (msg == "pwd")					return 3;
	if (msg == "lcd")					return 4;
	if (msg == "cd")					return 5;
	if (msg == "ls")					return 6;
	if (msg == "dir")					return 7;
	if (msg == "mkdir")					return 8;
	if (msg == "rmdir")					return 9;
	if (msg == "get")					return 10;
	if (msg == "mget")					return 11;
	if (msg == "put" || msg == "send")	return 12;
	if (msg == "mput")					return 13;
	if (msg == "delete")				return 14;
	if (msg == "rename")				return 15;
	if (msg == "bye")					return 16;
	return -1;
}