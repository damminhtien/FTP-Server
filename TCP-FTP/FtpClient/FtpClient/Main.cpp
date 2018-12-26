#include "FtpClient.h"

int main(int argc, char argv[]){
	int running = true;

	// Start the client and listen
	FtpClient* client = new FtpClient();

	client->run();

	return 0;
}