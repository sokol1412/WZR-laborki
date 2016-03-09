/************************************************************
    Obs³uga sieci - multicast, unicast
*************************************************************/
#define WIN32
#define _WINSOCK_DEPRECATED_NO_WARNINGS
//#include <windows.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <time.h>
//#include <unistd.h>     /* for close() */

#ifdef WIN32// VERSION FOR WINDOWS
//#include <ws2tcpip.h>
//#include <winsock.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#else // VERSION FOR LINUX
#include <sys/socket.h> /* for socket(), connect(), sendto(), and recvfrom() */
#include <arpa/inet.h>  /* for sockaddr_in and inet_addr() */
#endif


#include "siec.h"

extern FILE *f;

HANDLE threadSend;

long czasy_wys[20];       // czasy wyslania ostatnich 20 ramek 
long liczba_wyslanych = 0;          // liczba wyslanych ramek
extern long czas_start;                // czas od poczatku dzialania aplikacji  


void DieWithError(char *errorMessage)
{
    perror(errorMessage);
}


unicast_net::unicast_net(unsigned short usPort)
{
	
	printf("UDP socket ");
#ifdef WIN32
	if (WSAStartup(MAKEWORD(2,2), &localWSA) != 0)
    {
        printf("WSAStartup error");
    }
#endif
 /* Create a datagram/UDP socket */
    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
  
    memset(&udpServAddr, 0, sizeof(udpServAddr));   /* Zero out structure */
    udpServAddr.sin_family = AF_INET;                /* Internet address family */
    udpServAddr.sin_addr.s_addr = htonl(INADDR_ANY); /* Any incoming interface */
    udpServAddr.sin_port = usPort;      /* Local port */

    if (bind(sock, (struct sockaddr *) &udpServAddr, sizeof(udpServAddr)) < 0)
        DieWithError("bind() failed");
    printf("OK\n");
}

unicast_net::~unicast_net()
{
  //close(sock);
#ifdef WIN32
  WSACleanup(); // ********************* For Windows
#endif
}


int unicast_net::send(char *buffer,char *servIP,unsigned short size)
{
   udpServAddr.sin_addr.s_addr = inet_addr(servIP);  

    if (sendto(sock, buffer, size, 0, (struct sockaddr *)
               &udpServAddr, sizeof(udpServAddr)) == size) return size;
    else DieWithError("sendto() sent a different number of bytes than expected");    
}         

int unicast_net::send(char *buffer,unsigned long servIP,unsigned short size)
{
   udpServAddr.sin_addr.s_addr = servIP;

    if (sendto(sock, buffer, size, 0, (struct sockaddr *)
               &udpServAddr, sizeof(udpServAddr)) == size) return size;
    else DieWithError("sendto() sent a different number of bytes than expected");   
}         


int unicast_net::reciv(char *buffer,unsigned long *IP_Sender,unsigned short size)  
{
	int ucleng = sizeof(udpClntAddr);
	
	  if ((sSize = recvfrom(sock, buffer, size, 0,
			(struct sockaddr *) &udpClntAddr, (socklen_t*)&ucleng)) < 0)
			DieWithError("recvfrom() failed ");
	  else
	  {
			*IP_Sender=udpClntAddr.sin_addr.s_addr;
			return sSize;          
	  }         
}  
         
// end
