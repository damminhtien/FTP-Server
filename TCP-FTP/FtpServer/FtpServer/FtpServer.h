#include "Ftp.h"


#define LISTEN_ON_PORT 8888
#define MAX_CLIENTS 10

// Status messages
#define CLIENT_DISC 0
#define CLIENT_CONN 1
#define CLIENT_X 2


struct Client {
	bool conn;			// cet true if a client is connected
	sockaddr_in addr;	// client's ip address
	SOCKET cs;			// client socket
	fd_set set;			// used to check if there is data in the socket
	int i;				// any piece of additional info
	bool runner;		// set true if client is running
};


class FtpServer : public Ftp {
	private:
		struct sockaddr_in ServerAddr;		/* Server address */
		char servername[HOSTNAME_LENGTH];
		int sock;

	public:
		FtpServer();
		~FtpServer();
		void accept_new_clients();
		bool accept_client(Client * cli);			//accept client connections
		
		static bool check_server_status();
		static void updateStatus(int status);
		static bool disconnectClient(Client *cli);
		static void notiCmd(bool flag, char* cmd);

		static DWORD WINAPI ReceiveCmds(LPVOID lpParam);
		static bool handleFrame(Client* cli);

		static void checkLogin(Client* cli);
		static bool addUser(Client* cli, char* arg);
		static bool delUser(Client* cli, char* arg);
		static bool updateUser(Client* cli, char* arg);
		static bool doUpload(Client* cli, char* filename);
		static bool doDownload(Client* cli, char* filename);
		static bool checkOS(Client *cli);
		static bool listCmd(Client *cli);
		static bool renamefile(Client *cli, char* arg);
		static bool deletefile(Client *cli, char* filename);
		static bool makefile(Client* cli, char* arg);
		static bool showCurDir(Client *cli);
		static bool makefolder(Client* cli, char* foldername);
		static bool renamefolder(Client *cli, char* arg);
	
};