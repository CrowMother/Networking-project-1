#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fstream>
#include <sstream>

#define SERVERPORT "4950"	// the port users will be connecting to
std::string SERVERIP = "192.168.0.11";
#define MAXBUFLEN 100

void set_server_ip(std::string actualIP)
{
    SERVERIP = actualIP;
}
std::string get_server_ip()
{
    return SERVERIP;
}

std::string read_conf_server_IP()
{
    std::string server_ip;
    std::ifstream conf_file("client.conf");
    std::string line;
    while (std::getline(conf_file, line)) {
        if (line.find("SERVER_IP=") == 0) {
            server_ip = line.substr(10);
            break;
        }
    }
    conf_file.close();
    return server_ip;
}

std::string read_conf_server_port()
{
    std::string server_port;
    std::ifstream conf_file("client.conf");
    std::string line;
    while (std::getline(conf_file, line)) {
        if (line.find("SERVER_PORT=") == 0) {
            server_port = line.substr(10);
            break;
        }
    }
    conf_file.close();
    return server_port;
}

void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

std::string get_input()
{
  std::string userInput;
  std::cout << "Enter message: ";
  std::getline(std::cin, userInput);
  return userInput;
}

std::string main_reciever_from_server(std::string port)
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
    const char* ServerConnectionport = port.c_str();
    

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, ServerConnectionport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return "0";
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return "0";
	}

	freeaddrinfo(servinfo);

	//printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	//printf("listener: got packet from %s\n",
		// inet_ntop(their_addr.ss_family,
		// 	get_in_addr((struct sockaddr *)&their_addr),
		// 	s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);

    //get their ip address
    

	close(sockfd);
    return std::string(buf);
}

int main_connection_out(std::string userInput, std::string port)
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
    const char* ServerConnectionport = port.c_str();
    const char* server_ip = SERVERIP.c_str();

	if (userInput.empty()) {
		fprintf(stderr,"usage: talker hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(server_ip, ServerConnectionport, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, userInput.c_str(), userInput.size(), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	//printf("talker: sent %d bytes to %s\n", numbytes, server_ip);
	close(sockfd);

}

int initial_connection(std::string userInput)
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
    const char* server_ip = SERVERIP.c_str();

	if (userInput.empty()) {
		fprintf(stderr,"usage: talker hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(server_ip, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return 1;
	}

	// loop through all the results and make a socket
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("talker: socket");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "talker: failed to create socket\n");
		return 2;
	}

	if ((numbytes = sendto(sockfd, userInput.c_str(), userInput.size(), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	//printf("talker: sent %d bytes to %s\n", numbytes, server_ip);
	close(sockfd);



}


std::string initial_reciever_from_server()
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];
    

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, SERVERPORT, &hints, &servinfo)) != 0) {
		fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
		return "0";
	}

	// loop through all the results and bind to the first we can
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype,
				p->ai_protocol)) == -1) {
			perror("listener: socket");
			continue;
		}

		if (bind(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("listener: bind");
			continue;
		}

		break;
	}

	if (p == NULL) {
		fprintf(stderr, "listener: failed to bind socket\n");
		return "0";
	}

	freeaddrinfo(servinfo);

	//printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	//printf("listener: got packet from %s\n",
		// inet_ntop(their_addr.ss_family,
		// 	get_in_addr((struct sockaddr *)&their_addr),
		// 	s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);

    //get their ip address
    

	close(sockfd);
    return std::string(buf);
}
int main()
{
    std::string server = read_conf_server_IP();

    int i = 0;
    std::string input = get_input();
	initial_connection(input);
    std::string port = initial_reciever_from_server();

    //client does not need to be multithreaded
     std::string CPORT = port;
     int j = 0;
     do
    {
        std::string arguments = get_input();
        main_connection_out(arguments, CPORT);
        std::string recieved = main_reciever_from_server(CPORT);
        if (recieved.find("BYE") != std::string::npos) {
            j = 1;
            i = 1;
        }
    }
    while(j == 0);

    
	return 0;
}
