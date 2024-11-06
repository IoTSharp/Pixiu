
// file: rs485.c
// serial port communiction
// rs485
// tc400 communication protocol
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <termios.h>
#include <errno.h>
#include <string.h>
#include <time.h>
#include  "app.h"
#include  "crc.h"


int open_dev(char *Dev)
{
	int    fd = open(Dev, O_RDWR);
	if (-1 == fd)    
	{             
		perror("Can't Open Serial Port");
		return -1;        
	}
	if (fcntl(fd, F_SETFL, O_NONBLOCK) < 0)
	{
		printf("Unable set to NONBLOCK mode");
		return -1;
	}
	return fd;
}

// set serial speed
int speed_arr[] = {
	B38400,
	B19200,
	B9600,
	B4800,
	B2400,
	B1200,
	B300,
	\
                    B38400,
	B19200,
	B9600,
	B4800,
	B2400,
	B1200,
	B300,
};
int name_arr[] = {
	38400,
	19200,
	9600,
	4800,
	2400,
	1200,
	300,
	38400,
	\
                   19200,
	9600,
	4800,
	2400,
	1200,
	300,
};
void set_speed(int fd, int speed)
{
	int i;
	int status;
	struct termios Opt;
    
	tcgetattr(fd, &Opt);
	for (i = 0; i < sizeof(speed_arr) / sizeof(int); i++)
	{
		if (speed == name_arr[i])
		{     
			tcflush(fd, TCIOFLUSH);     
			cfsetispeed(&Opt, speed_arr[i]);  
			cfsetospeed(&Opt, speed_arr[i]);   
			status = tcsetattr(fd, TCSANOW, &Opt);  
			if (status != 0)
			{        
				perror("tcsetattr fd1");  
				return;     
			}    
			tcflush(fd, TCIOFLUSH);   
		}  
	}
}

// set serial port data, stop and crc bit.
int set_parity(int fd, int databits, int stopbits, int parity)
{
	struct termios options;
	if (tcgetattr(fd, &options) != 0)
	{
		perror("SetupSerial 1");     
		return (FALSE);  
	}
	options.c_cflag &= ~CSIZE;
    
	//  data bit
	switch (databits)
	{   
	case 7:        
		options.c_cflag |= CS7;
		break;
	case 8:     
		options.c_cflag |= CS8;
		break;   
	default:    
		fprintf(stderr, "Unsupported data size\n");
		return (FALSE);  
	}
    
	switch (parity)
	{   
	case 'n':
	case 'N':    
		options.c_cflag &= ~PARENB; // Clear parity enable
		options.c_iflag &= ~INPCK; // Enable parity checking
		options.c_lflag &= 0; // add comment it's very important to set this parameter.
		options.c_oflag  &= ~OPOST; /*Output*/
		break;  
        
	case 'o':   
	case 'O':     
		options.c_cflag |= (PARODD | PARENB); // set odd parity  
		options.c_iflag |= INPCK; // disnable parity checking
		break;  
        
	case 'e':  
	case 'E':   
		options.c_cflag |= PARENB; // enable parity     
		options.c_cflag &= ~PARODD; // change into even     
		options.c_iflag |= INPCK; // disnable parity checking
		break;
        
	case 'S':
	case 's':  // as no parity
		options.c_cflag &= ~PARENB;
		options.c_cflag &= ~CSTOPB; break;  
        
	default:   
		fprintf(stderr, "Unsupported parity\n");    
		return (FALSE);  
	}  
    
	// set stop bit  
	switch (stopbits)
	{   
	case 1:    
		options.c_cflag &= ~CSTOPB;  
		break;  
	case 2:    
		options.c_cflag |= CSTOPB;  
		break;
	default:    
		fprintf(stderr, "Unsupported stop bits\n");  
		return (FALSE);
	}
	// Set input parity option
	if (parity != 'n')   
		options.c_iflag |= INPCK;
	tcflush(fd, TCIFLUSH);
	options.c_cc[VTIME] = 150; // set time out for 15 seconds   
	options.c_cc[VMIN] = 0; // update the options and do it now
	if (tcsetattr(fd, TCSANOW, &options) != 0)   
	{
		perror("SetupSerial 3");   
		return (FALSE);  
	}
	return (TRUE);  
}

void config_s(int fd)
{
	struct termios oldtio, newtio;
       
	tcgetattr(fd, &oldtio);
	memset(&newtio, 0, sizeof(newtio));
       
	newtio.c_cflag = B9600 | CS8 | CLOCAL | CREAD | CSTOPB;
	newtio.c_cflag &= ~CRTSCTS;
	newtio.c_iflag = IGNPAR | ICRNL;
	newtio.c_iflag &= ~(IXON | IXOFF | IXANY);
	newtio.c_oflag = 0;
	newtio.c_lflag = ICANON;
       
	tcflush(fd, TCIFLUSH);
	tcsetattr(fd, TCSANOW, &newtio);
}