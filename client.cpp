#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <arpa/inet.h>
#include <iostream>
using namespace std;
const char* SERVER_IP = "127.0.0.1";  
const int SERVER_PORT = 12345;   

int main()
{
	int client_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (client_socket == -1)
	{
		cout << "Error creating socket";
		exit(0);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(SERVER_IP);
	server_addr.sin_port = htons(SERVER_PORT);

	if (connect(client_socket, (struct sockaddr*)&server_addr, sizeof(server_addr)) == -1)
	{
		cout << "Error connecting to server";
		close(client_socket);
		exit(0);
	}
	while (1)
	{
		cout << "Enter a message to send to the server (or type 'exit' to quit): " << endl;
		char input[1024];
		cin >> input;
		string ex = "exit";

		if (input == ex)
		{
			close(client_socket);
			exit(0);
		}

		if (send(client_socket, input, strlen(input), 0) == -1)
		{
			cout << "Error sending message to server";
			close(client_socket);
			exit(0);
		}
	}
	return 0;
}
