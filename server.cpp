#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <errno.h>
#include <set>
#include <iostream>
using namespace std;

volatile sig_atomic_t wasSigHup = 0;
volatile sig_atomic_t wasSigInt = 0;

void sigHupHandler(int r)
{
	wasSigHup = 1;
}

void sigIntHandler(int r)
{
	wasSigInt = 1;
}

struct sigaction sa;

void setupSignalHandling()
{
	sigaction(SIGHUP, NULL, &sa);
	sa.sa_handler = sigHupHandler;
	sa.sa_flags |= SA_RESTART;
	sigaction(SIGHUP, &sa, NULL);

	sigaction(SIGINT, NULL, &sa);
	sa.sa_handler = sigIntHandler;
	sa.sa_flags |= SA_RESTART;
	sigaction(SIGINT, &sa, NULL);
}

void cleanup(int server_socket, const set<int> &clients)
{
	for (auto client_socket: clients)
	{
		close(client_socket);
	}
	close(server_socket);
	exit(0);
}

int main()
{
	setupSignalHandling();
	char buffer[1024];
	const int PORT = 12345;
	int server_socket = socket(AF_INET, SOCK_STREAM, 0);
	if (server_socket == -1)
	{
		cout << "Error creating socket";
		exit(0);
	}

	struct sockaddr_in server_addr;
	memset(&server_addr, 0, sizeof(server_addr));
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = INADDR_ANY;
	server_addr.sin_port = htons(PORT);

	if (bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr)) == -1)
	{
		cout << "Error binding socket" << endl;
		cleanup(server_socket, {});
	}
	if (listen(server_socket, 5) == -1)
	{
		cout << "Error listening on socket" << endl;
		cleanup(server_socket, {});
	}

	sigset_t blockedMask, origMask;
	sigemptyset(&blockedMask);
	sigaddset(&blockedMask, SIGHUP);
	sigprocmask(SIG_BLOCK, &blockedMask, &origMask);

	set<int> clients;

	while (1)
	{
		fd_set fds;
		FD_ZERO(&fds);
		FD_SET(server_socket, &fds);
		int max_fd = server_socket;
		for (auto client_socket: clients)
		{
			FD_SET(client_socket, &fds);
			max_fd = max(max_fd, client_socket);
		}
		if (pselect(max_fd + 1, &fds, NULL, NULL, NULL, &origMask) == -1)
		{
			if (errno == EINTR)
			{
				if (wasSigHup)
				{
					cout << "Received SIGHUP signal" << endl;
					wasSigHup = 0;
					cleanup(server_socket, clients);
				}

				if (wasSigInt)
				{
					cout << "Received SIGINT signal. Shutting down" << endl;
					cleanup(server_socket, clients);
				}
			}
			else
			{
				cout << "Error in pselect" << endl;
				cleanup(server_socket, clients);
			}
		}

		if (FD_ISSET(server_socket, &fds))
		{
			struct sockaddr_in client_addr;
			socklen_t client_len = sizeof(client_addr);
			int client_socket = accept(server_socket, (struct sockaddr *) &client_addr, &client_len);
			if (client_socket == -1)
			{
				cout << "Error accepting connection" << endl;
				exit(0);
			}
			string claddr = inet_ntoa(client_addr.sin_addr);
			cout << "Accepted connection from " << claddr << endl;
			if (!clients.empty())
			{
				cout << "Blocking new connections" << endl;
				close(client_socket);
			}
			else
			{
				clients.insert(client_socket);
			}
		}

		for (auto client_socket: clients)
		{
			if (FD_ISSET(client_socket, &fds))
			{
				ssize_t bytes_received = recv(client_socket, buffer, sizeof(buffer), 0);
				if (bytes_received > 0)
				{
					cout << "Bytes received: " << bytes_received << endl;
				}
				else if (bytes_received == 0)
				{
					cout << "Client " << client_socket << " closed the connection" << endl;
					close(client_socket);
					clients.erase(client_socket);
				}
				else
				{
					cout << "Error receiving data" << endl;
					close(client_socket);
					clients.erase(client_socket);
				}
			}
		}
	}
	return 0;
}
