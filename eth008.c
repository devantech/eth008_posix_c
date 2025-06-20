/*
 * POSIX C example program for controlling an ETH008.
 *
 * allows viewing of the IO states, and toggling outputs.
 *
 * compile with:
 *		gcc eth008.c -o eth008
 *
 *	by James Hendrson, 2024.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>

#define GET_INFO				0x10
#define GET_UNLOCK				0x7A
#define SEND_PASSWORD			0x79
#define LOGOUT					0x7B
#define GET_DIGITAL_OUTPUTS		0x24
#define SET_OUTPUT_ACTIVE		0x20
#define SET_OUTPUT_INACTIVE		0x21

/*
 * Print help text to the screen.
 */
void printHelp(void) {
  printf("usage: eth008 [options] ip_address\n");
  printf("  options:\n");
  printf("    -p <port> Set the port number to talk to (defaults to 17494)\n");
  printf("    -P <pass> The password used for unlocking the module if tcp password is enabled\n");
  printf("    -m        Display the module information.\n");
  printf("    -o        Display the digital output states.\n");
  printf("    -t <io>   Toggle digital output <io> (1 - 8).\n");
  printf("    -h        This help text.\n");
}


/*
 * Tries to open a socket connection to the given ip address and port.
 *
 * char * ip	- The ip address.
 * int port		- The port number.
 *
 * returns -1 on failure, otherwise 0.
 */
int openSocket(char * ip, int port) {

	// Get the socket
	int module_socket;
    if ((module_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0) {
        // Error
		perror("openSocket - ");
		return -1;
    }

	// Connect to the module
    struct sockaddr_in serv_addr;
    serv_addr.sin_family = AF_INET;                     
    serv_addr.sin_addr.s_addr = inet_addr(ip);     // Set IP address to connect to
    serv_addr.sin_port = htons(port);              // Set port to connect to

    if (connect(module_socket, (struct sockaddr *) &serv_addr, sizeof(serv_addr)) < 0) {
		// Error
		perror("openSocket - ");
		return -1;
    }
	
	// Return the socket handle
	return module_socket;

}


/*
 * Tries to read a number of bytes from the given file descriptor
 * into the given buffer.
 *
 * int socket		- the file descriptor of the socket.
 * uint8_t *buffer	- the buffer in ti which data is to be read.
 * int num			- the number of bytes to try and read.
 *
 * returns -1 on an error, otherwise the number of bytes read.
 */
int readData(int socket, uint8_t *buffer, int num) {

	struct pollfd fds[1];
	fds[0].fd = socket;
	fds[0].events = POLLIN;

	// Check to see if data is ready to read on the socket
	int ev = poll(fds, 1, 500);

	if (ev == -1) {
		// Error
		perror("readData - ");
		return -1;
	} else if (ev == 0) {
		// Timeout
		perror("readData - ");
		return -1;
	} else if (fds[0].revents & POLLIN) {
	
		// Socket ready for reading
		int count = 0;

		while (count < num) {

			int rd = read(socket, buffer + count, num - count);
			
			if (rd == 0) {
				// End of file
				return count;
			} else if (rd == -1) {
				// Error
				perror("readData - ");
				return -1;
			}
			
			count += rd;

		}

		return count;

	}

	return -1;

}


/*
 * Tries to write an ammount of data to a module.
 *
 * int socket		- The file descriptor to write to.
 * uint8_t * data	- A buffer containing the data to write.
 * int num			- The number of bytes to write.
 */
int writeData(int socket, uint8_t * data, int num) {
	

	struct pollfd fds[1];
	fds[0].fd = socket;
	fds[0].events = POLLOUT;

	int ev = poll(fds, 1, 500);
	
	if (ev == -1) {
		// Error
		perror("writeData - ");
		return -1;
	} else if (ev == 0) {
		// Timeout
		perror("writeData - ");
		return -1;
	} else if (fds[0].revents & POLLOUT) {

		// Can write to socket now
		int written = write(socket, data, num); // Try and write data to the socket

		if (written < 0) {
			perror("writeData: ");
			return -1;
		} else if (written != num) {
			printf("%u bytes written out of %u requested\n", written, num);
			return -1;
		}

		return written;
	}

	return -1;

}


/**
 * Prints the module data to standard output.
 *
 * int socket		- The file descriptor of the module socket.
 *
 */
void printModuleInfo(int socket) {
	
	uint8_t buffer[10] = {0};
	buffer[0] = GET_INFO;	// command to get back the module info 

	if (writeData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

	if (readData(socket, buffer, 3) < 0) {
		exit(EXIT_FAILURE);
	}

	printf("Module ID: %d\nHardware version: %d\nFirmware version: %d\n", buffer[0], buffer[1], buffer[2]); 

}


/*
 * Get the unlock time back from the module.
 *
 * int socket		- The socket descriptor of the module.
 *
 * returns the unlock time.
 */
uint8_t getUnlockTime(int socket) {

	uint8_t buffer[1] = { GET_UNLOCK }; // The command to get the unlock time

	if (writeData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

	if (readData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

	return buffer[0];

}


/*
 * Send the a given password to the module.
 *
 * int socket			- The socket descriptor.
 * uint8_t * password	- the password to send.
 */
void sendPassword(int socket, char * password) {

	uint8_t buffer[100] = { 0 };
	
	// Put the password in the buffer to write out starting at index 1 so that the
	// send password command can be placed in the buffer before it.
	int len = strlen(password);
	for (int pi = 0; pi < len; pi++) {
		buffer[pi+1] = password[pi];
	}
	buffer[0] = SEND_PASSWORD; // Put the send password command in front of the password

	if (writeData(socket, buffer, strlen(password) + 1) < 0) {
		exit(EXIT_FAILURE);
	}

	if (readData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

	if (buffer[0] != 1) {
		printf("Password error.");
		exit(EXIT_FAILURE);
	}

}


/*
 * Send the logout command to lock the module again.
 *
 * int socket		- the socket descriptor.
 */
void sendLogout(int socket) {

	uint8_t buffer[1] = { LOGOUT }; // The command to log out

	if (writeData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

	if (readData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

}


/*
 * Get the digital output states from the module.
 *
 * int socket		- The socket descriptor.
 * uint8_t buffer	- The buffer the states are placed in.
 */
void getDigitalOutputStates(int socket, uint8_t * buffer) {

	buffer[0] = GET_DIGITAL_OUTPUTS; // Command to get the output states back from the module

	if (writeData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

	if (readData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

}


/*
 * Prints the states of the digital outputs to the screen.
 *
 * int socket		- The socket descriptor to talk on.
 */
void printOutputStates(int socket) {

	uint8_t buffer[3] = { 0 };

	getDigitalOutputStates(socket, buffer);

	// Print out the states of the relays
	char *rs;
	for (int r = 0; r < 8; r++) {
		rs = (buffer[0] & (0x01 << r)) != 0 ? "ACTIVE" : "INACTIVE";
		printf("Relay %d: %s\n", r + 1, rs);
	}

}


/*
 * Tries to toggle a digital output.
 *
 * int socket		- The socket descriptor.
 * uint8_t output	- the output to toggle.
 */
void toggleDigitalOutput(int socket, uint8_t output) {

	uint8_t buffer[3] = { 0 };

	getDigitalOutputStates(socket, buffer);
	
	// Check the state of the bit representing the optput to toggle,
	// and get the command to switch it to the opposite state.
	uint8_t command;
	if (output > 0 && output < 9) {
		command = (buffer[0] & (0x01 << (output - 1))) != 0 ? SET_OUTPUT_INACTIVE : SET_OUTPUT_ACTIVE;
	} else {
		return;	// Not a valid input number so do nothing.
	}

	buffer[0] = command; // The command tosend either the output active, or inactive.
	buffer[1] = output;	// The output to switch.
	buffer[2] = 0x00; // A pulse time, 0 in this case to make the change permanent.

	if (writeData(socket, buffer, 3) < 0) {
		exit(EXIT_FAILURE);
	}

	if (readData(socket, buffer, 1) < 0) {
		exit(EXIT_FAILURE);
	}

}


int main(int argc, char ** argv) {

	int info = 0; // Used to indicate if we should print the module information.
	int outputs = 0; // Used to indicate if we should show the digital output states.
	uint8_t toggle = 0; // Used to indicate if we want to toggle a digital output.
	int port = 17494; // The port that the module is on.
	char *password = NULL; // The password used to unlock the module

	int opt;

	while ((opt = getopt(argc, argv, "omiaP:p:t:h")) != -1) {

		switch (opt) {

			case 'h':
				printHelp();
				break;

			/*
			 * The m option displays the module information
			 */
			case 'm':
				info = 1;
				break;
			
			/*
			 * The p option allows us to set the port. it defaulte to 17494.
			 */
			case 'p':
				port = atoi(optarg);
				break;
			
			/*
			 * The P option allows the user to supply a password to unlock the module.
			 */
			case 'P':
				password = strdup(optarg);
				break;

			/*
			 * The o option is is used to display the digital outputs to the screen.
			 */
			case 'o':
				outputs = 1;
				break;

			/*
			 * The t command is used to toggle adigital output.
			 */
			case 't':
				toggle = atoi(optarg);

			case '?':
				break;
		}
	}

	if (optind >= argc) {
		printf("No IP address was supplied.\n");
		printHelp();
		exit(EXIT_FAILURE);
	}

	// The ip address is the non argument input given.
	char buffer[1024] = {0};
	int socket = openSocket(argv[optind], port);

	if (socket == -1) {
		exit(EXIT_FAILURE);
	}

	// check unlock time to see if we need to send a password.
	if (getUnlockTime(socket) == 0) {

		// We need to send a password before we can control this module
		if (password == NULL) {
			printf("A password is needed.\n");
			close(socket);
			return 0;
		}

		sendPassword(socket, password); // send the password
										//
		if (getUnlockTime(socket) == 0) { // Check to see if the password has unlocked the module
			printf("Unable to unlock module,\n");
			close(socket);
			free(password);
			return 0;
		}

	}

	// If the i argument was passed then print the module information.
	if (info) {	
		printModuleInfo(socket);
	}

	// If the t argument was passed then toggel the output.
	if (toggle) {
		toggleDigitalOutput(socket, toggle);
	}

	// if the o argument was passed then show the states of the outputs.
	if (outputs) {
		printOutputStates(socket);
	}

	sendLogout(socket);
	close(socket);
	return 0;

}
