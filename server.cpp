/*
** listener.c -- a datagram sockets "server" demo
*/
#include <fstream>
#include <iostream>
#include <string>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <sstream>
#include <thread>
#include <vector>

std::string initialPort = "4950";	// the port users will be connecting to
int port = 8000;
#define MAXBUFLEN 100
std::string HOSTNAME = "CALCO";

std::string read_conf()
{
    //reads in server.conf, if this file is errored out then return error message
    std::ifstream confFile("server.conf");
    if (!confFile) {
        std::cout << "Error opening config file." << std::endl;
        return "0";
    }
    std::string line;
    int udpPort = 0;
    while (std::getline(confFile, line)) {
        if (line.find("UDP_PORT") != std::string::npos) {
            std::string portStr = line.substr(line.find("=") + 1);
            udpPort = std::stoi(portStr);
            break;
        }
    }
    if (!udpPort) {
        std::cerr << "UDP_PORT not found in config file." << std::endl;
        return "0";
    }
    std::cout << "UDP Port: "<< udpPort << std::endl;

    std::string port;
    std::stringstream ss;
    ss << udpPort;
    port = ss.str();
    
    return port;
}

void set_initial_port(std::string port){
    initialPort = port;
}

std::string get_initial_port(){
    return initialPort;
}

// get sockaddr, IPv4 or IPv6:
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &(((struct sockaddr_in*)sa)->sin_addr);
	}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

// get talker ip address for a specific message
std::string get_talker_ip(struct sockaddr_storage their_addr) {
  char s[INET6_ADDRSTRLEN];
  inet_ntop(their_addr.ss_family,
            get_in_addr((struct sockaddr *)&their_addr), s, sizeof s);
  return std::string(s);
}

//gets the next port address
std::string increment_and_get_port() {
  port++;
  std::stringstream ss;
  ss << port;
  return ss.str();
}

std::string main_reciver_from_client(std::string port)
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN]; 
    const char* clientPort = port.c_str();


	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, clientPort, &hints, &servinfo)) != 0) {
		//fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
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

	/* printf("listener: got packet from %s\n",
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s)); */
	//printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);

    std::string str(buf);

    std::string outMessage = std::string(buf, buf + sizeof buf / sizeof buf[0]);

	close(sockfd);
    return(outMessage);
}

int main_message_to_Client(std::string message, std::string IP, std::string port)
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
    const char* clientIP = IP.c_str();
    const char* clientPort = port.c_str();
   

	if (message.empty()) {
		fprintf(stderr,"usage: talker hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(clientIP, clientPort, &hints, &servinfo)) != 0) {
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

	if ((numbytes = sendto(sockfd, message.c_str(), message.size(), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	//printf("talker: sent %d bytes to %s\n", numbytes, clientIP);

    struct sockaddr_storage their_addr;
    // Fill in the `their_addr` structure with information about the client


	close(sockfd);
}

std::string initial_connection_to_client()
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
	struct sockaddr_storage their_addr;
	char buf[MAXBUFLEN];
	socklen_t addr_len;
	char s[INET6_ADDRSTRLEN];  
    const char* mainPort = get_initial_port().c_str(); 

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET6; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;
	hints.ai_flags = AI_PASSIVE; // use my IP

	if ((rv = getaddrinfo(NULL, mainPort, &hints, &servinfo)) != 0) {
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

	printf("listener: waiting to recvfrom...\n");

	addr_len = sizeof their_addr;
	if ((numbytes = recvfrom(sockfd, buf, MAXBUFLEN-1 , 0,
		(struct sockaddr *)&their_addr, &addr_len)) == -1) {
		perror("recvfrom");
		exit(1);
	}

	printf("listener: got packet from %s\n",
		inet_ntop(their_addr.ss_family,
			get_in_addr((struct sockaddr *)&their_addr),
			s, sizeof s));
	//printf("listener: packet is %d bytes long\n", numbytes);
	buf[numbytes] = '\0';
	printf("listener: packet contains \"%s\"\n", buf);

    //get their ip address
    std::string clientIP = get_talker_ip(their_addr);
    

	close(sockfd);
    return(clientIP);
}

int initial_message_to_Client(std::string message, std::string IP)
{
    int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;
	int numbytes;
    const char* clientIP = IP.c_str();
   const char* mainPort = get_initial_port().c_str();

	if (message.empty()) {
		fprintf(stderr,"usage: talker hostname message\n");
		exit(1);
	}

	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_INET; // set to AF_INET to use IPv4
	hints.ai_socktype = SOCK_DGRAM;

	if ((rv = getaddrinfo(clientIP, mainPort, &hints, &servinfo)) != 0) {
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

	if ((numbytes = sendto(sockfd, message.c_str(), message.size(), 0,
			 p->ai_addr, p->ai_addrlen)) == -1) {
		perror("talker: sendto");
		exit(1);
	}

	freeaddrinfo(servinfo);

	printf("talker: sent %d bytes to %s\n", numbytes, clientIP);

    struct sockaddr_storage their_addr;
    // Fill in the `their_addr` structure with information about the client


	close(sockfd);
}

//local thread this in the future
thread_local double answer;
void set_answer(double setTo)
{
    answer = setTo;
}
double get_answer()
{
    return answer;
}
//local thread this for mode selection per user
thread_local int mode;
void set_mode(int setTo)
{
    mode = setTo;
}
int get_mode()
{
    return mode;
}

//calculate volumes to liters and back out
double gallonsToLiters(double gallons) {
  return gallons * 3.78541;
}
double empiricalGallonsToLiters(double gallons) {
  return gallons * 4.54609;
}
double cubic_meters_to_liters(double cubic_meters) {
  return cubic_meters * 1000;
}

double liters_to_gallons(double liters) {
  return liters * 0.264172;
}
double liters_to_empirical_gallons(double liters) {
  return liters * 0.219969;
}
double liters_to_cubic_meters(double liters) {
  return liters / 1000.0;
}
//calculate volumes

void vol_calc(std::string arr[], int size, double amount) {
    double liters;
  if (arr[0] == "LTR") {
    liters = amount;

  } else if (arr[0] == "GALNU") {
    liters = gallonsToLiters(amount);

  } else if (arr[0] == "GALNI") {
    liters = empiricalGallonsToLiters(amount);

  } else if (arr[0] == "CUBM") {
    liters = cubic_meters_to_liters(amount);

  } else {
    set_answer(-1);

  }
    //convert to
  if (arr[1] == "LTR") {
    set_answer(liters);

  } else if (arr[1] == "GALNU") {
    set_answer(liters_to_gallons(liters));

  } else if (arr[1] == "GALNI") {
    set_answer(liters_to_empirical_gallons(liters));

  } else if (arr[1] == "CUBM") {
    set_answer(liters_to_cubic_meters(liters));

  } else {
    set_answer(-1);
  }

}
//calculate area
double square_inches_to_square_meters(double square_inches) {
return square_inches * 0.00064516;
}
double square_feet_to_square_meters(double square_feet) {
return square_feet * 0.092903;
}
double square_miles_to_square_meters(double square_miles) {
return square_miles * 2589988.11;
}

double square_meters_to_square_inches(double square_meters) {
return square_meters * 1550.0031;
}
double square_meters_to_square_feet(double square_meters) {
return square_meters * 10.764;
}
double square_meters_to_square_miles(double square_meters) {
return square_meters * 3.861e-7;
}
void area_calc(std::string arr[], int size, double amount) {
   long double sqrMeter;
  if (arr[0] == "SQRMT") {
    sqrMeter = amount;

  } else if (arr[0] == "SQRML") {
    sqrMeter = square_miles_to_square_meters(amount);

  } else if (arr[0] == "SQRIN") {
    sqrMeter = square_inches_to_square_meters(amount);

  } else if (arr[0] == "SQRFT") {
    sqrMeter = square_feet_to_square_meters(amount);

  } else {
    set_answer(-2);

  }
    //convert to
  if (arr[1] == "SQRMT") {
   set_answer(sqrMeter);

  } else if (arr[1] == "SQRML") {
    set_answer(square_meters_to_square_miles(sqrMeter));

  } else if (arr[1] == "SQRIN") {
    set_answer(square_meters_to_square_inches(sqrMeter));

  } else if (arr[1] == "SQRFT") {
    set_answer(square_meters_to_square_feet(sqrMeter));

  } else {
    set_answer(-2);
  }

}

//calc weight
double kilo_to_pounds(double kilo) {
    return kilo * 2.20462;
}
double kilo_to_carrots(double kilo) {
    return kilo * 5000;
}
void weight_calc(std::string arr[], int size, double amount) {
    double kilo;
    if (arr[0] == "KILO") {
        kilo = amount;
    } else if (arr[0] == "PND") {
        kilo = amount * 0.453592;
    } else if (arr[0] == "CART") {
        kilo = amount / 5000;
    } else {
        set_answer(-2);
        return;
    }

    if (arr[1] == "KILO") {
        set_answer(kilo);
    } else if (arr[1] == "PND") {
        set_answer(kilo_to_pounds(kilo));
    } else if (arr[1] == "CART") {
        set_answer(kilo_to_carrots(kilo));
    } else {
        set_answer(-2);
    }
}

//calc temp
void temp_calc(std::string arr[], int size, double amount) {
  double celsius;
  if (arr[0] == "CELS") {
    celsius = amount;
  } else if (arr[0] == "FAHR") {
    celsius = (amount - 32) * 5/9;
  } else if (arr[0] == "KELV") {
    celsius = amount - 273.15;
  } else {
    set_answer(-2);
    return;
  }

  // convert to
  if (arr[1] == "CELS") {
    set_answer(celsius);
  } else if (arr[1] == "FAHR") {
    set_answer((celsius * 9/5) + 32);
  } else if (arr[1] == "KELV") {
    set_answer(celsius + 273.15);
  } else {
    set_answer(-2);
  }
}

//goes through commands that can be used to change modes and other stuff
std::string commands(std::string wholeText, std::string CIP)
{
    std::string command = wholeText.c_str();
    std::string outMsg;
    if(command == "HELP")
    {
        return "<HELP-MENU>";
    }
    else if(command == "AREA")
    {
        set_mode(210);
        outMsg =  ""+ std::to_string(get_mode()) +" AREA MODE ready!" ;
        return outMsg;
    }
    else if(command == "VOL")
    {
        set_mode(220);
        outMsg =  ""+ std::to_string(get_mode()) +" VOLUME MODE ready!" ;
        return outMsg;
    }
    else if(command == "WGT")
    {
        set_mode(230);
        outMsg =  ""+ std::to_string(get_mode()) +" WEIGHT MODE ready!" ;
        return outMsg;
    }
    else if(command == "TEMP")
    {
        set_mode(240);
        outMsg =  ""+ std::to_string(get_mode()) +" TEMP MODE ready!" ;
        return outMsg;
    }
    else if(command == "BYE " + HOSTNAME)
    {
        return("BYE " + CIP);
    }

    //modes are at the bottom so you can change modes
    else if(get_mode() == 210 ) //AREA
    {
        std::string words[20];  // Array to store the individual words
        int count = 0;     // Counter for the number of words
        std::stringstream ss(command);  // Stringstream to split the string

        // Split the string and store the words in the array
        while (ss >> words[count]) {
            count++;
        }

        if (count == 3)
        {
            //calculate here
            double d;
            std::stringstream(words[2]) >> d;
            std::cout << d << std::endl;
            area_calc(words, count, d);
        }
        else
        {
            return "500 Syntax Error, command unrecognized";
        }
        std::string result = std::to_string(get_answer());
        if (get_answer() >= 0)
        {
            return "250: " + result +" " +  words[1];
        }
        else
        {
            return "501: bad syntax error in arguments";
        }
        
    }

    else if(get_mode()== 220) //volume
    {
        std::string words[20];  // Array to store the individual words
        int count = 0;     // Counter for the number of words
        std::stringstream ss(command);  // Stringstream to split the string

        // Split the string and store the words in the array
        while (ss >> words[count]) {
            count++;
        }

        if (count == 3)
        {
            //calculate here
            double d;
            std::stringstream(words[2]) >> d;
            std::cout << d << std::endl;
            vol_calc(words, count, d);
        }
        else
        {
            return "500 Syntax Error, command unrecognized";
        }
        std::string result = std::to_string(get_answer());
        if (get_answer() >= 0)
        {
            return "250: " + result +" " +  words[1];
        }
        else
        {
            return "501: bad syntax error in arguments";
        }
        
    }
    
    else if(get_mode() == 230) //WGT
    {
        std::string words[20];  // Array to store the individual words
        int count = 0;     // Counter for the number of words
        std::stringstream ss(command);  // Stringstream to split the string

        // Split the string and store the words in the array
        while (ss >> words[count]) {
            count++;
        }

        if (count == 3)
        {
            //calculate here
            double d;
            std::stringstream(words[2]) >> d;
            std::cout << d << std::endl;
            weight_calc(words, count, d);
        }
        else
        {
            return "500 Syntax Error, command unrecognized";
        }
        std::string result = std::to_string(get_answer());
        if (get_answer() >= 0)
        {
            return "250: " + result +" " +  words[1];
        }
        else
        {
            return "501: bad syntax error in arguments";
        }
        
    }

    else if(get_mode() == 240) //TEMP
    {
        std::string words[20];  // Array to store the individual words
        int count = 0;     // Counter for the number of words
        std::stringstream ss(command);  // Stringstream to split the string

        // Split the string and store the words in the array
        while (ss >> words[count]) {
            count++;
        }

        if (count == 3)
        {
            //calculate here
            double d;
            std::stringstream(words[2]) >> d;
            std::cout << d << std::endl;
            temp_calc(words, count, d);
        }
        else
        {
            return "500 Syntax Error, command unrecognized";
        }
        std::string result = std::to_string(get_answer());
        if (get_answer() >= 0)
        {
            return "250: " + result +" " +  words[1];
        }
        
    }

    else if (mode == 0) {
        if (command.find("SQRMT") != std::string::npos ||
        command.find("SQRML") != std::string::npos ||
        command.find("SQRIN") != std::string::npos ||
        command.find("SQRFT") != std::string::npos ||
        command.find("LTR") != std::string::npos ||
        command.find("GALNU") != std::string::npos ||
        command.find("GALNI") != std::string::npos ||
        command.find("CUBM") != std::string::npos ||
        command.find("KILO") != std::string::npos ||
        command.find("PND") != std::string::npos ||
        command.find("CART") != std::string::npos ||
        command.find("CELS") != std::string::npos ||
        command.find("FAHR") != std::string::npos ||
        command.find("KELV") != std::string::npos) {
            return "503 Bad sequence of commands";
        }
}

    else
    {
        return "unknown command: " + wholeText;
    }

    
}

//multithreading happens here boys
void handle_client_connection(const std::string& clientIP, const std::string& clientPort) {
    set_mode(0);
    int j = 0;
    do {
        std::string clientMessage = main_reciver_from_client(clientPort);
        std::string message = commands(clientMessage, clientIP);
        main_message_to_Client(message, clientIP, clientPort);
        if (message.find("BYE") != std::string::npos) {
            j = 1;
        }
    } while (j == 0);
}

int main(void)
{
    
    set_initial_port(read_conf());
    int i = 1;
    std::vector<std::thread> threads;
    //i will represent the number of connections that are out there; could make it a do while
    while( i >=1)
    {
        std::string clientIP = initial_connection_to_client();

        i ++;
        std::string clientPort = increment_and_get_port();
        initial_message_to_Client(clientPort, clientIP);

        //make the threading happen here and make sure to tie these values to the thread
        threads.push_back(std::thread(handle_client_connection, clientIP, clientPort));

    }
    //join threads back together
    for (auto& t : threads) {
        t.join();
    }

	return 0;
}