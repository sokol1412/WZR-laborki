/************************************************************
    Obs³uga sieci - multicast, unicast 
*************************************************************/

//#include <windows.h>
#include <stdio.h>      
#include <stdlib.h>     
#include <string.h>     
#include <time.h>
//#include <unistd.h>    

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

long czasy_wys[20];                 // czasy wyslania ostatnich 20 ramek 
long liczba_wyslanych = 0;          // liczba wyslanych ramek
extern long czas_start;             // czas od poczatku dzialania aplikacji  
extern float sr_czestosc;           // srednia czestosc wysylania ramek w [1/s]


// 
DWORD WINAPI SendThread(void *ptr)
{
    multicast_net *pmt_net=(multicast_net*)ptr;  // poiner to class multicast_net

	while(1)
	{
	  SEND_QUEUE *qn = pmt_net->q,*qn_prv = 0;
      
	  //if (qn == 0) fprintf(f,"brak ramek\n");
	  while(qn)                       // while not NULL member of queue 
	  {
	
		  //fprintf(f,"czas = %d spr ram %d\n",clock(),qn);  fclose(f); f=fopen("plik.txt","a");
		  if (qn->delay <= clock())   // timeout:  
		  {
			  pmt_net->send(qn->buf,qn->size);  // sending a message
			  //fprintf(f,"czas = %d, wyslano ramke z czasem %d\n",clock(),qn->delay); 
              SEND_QUEUE *do_usun = qn;
			 		  
			  if (qn == pmt_net->q)             // if 1-st member  
			  {
			    pmt_net->q = qn->next;
				qn = pmt_net->q;
			  }
			  else
			  {
				qn_prv->next = qn->next;
				qn = qn_prv;
			  }	
			  delete do_usun->buf;                   // buffer and queue member are delated
			  delete do_usun;
			  break;
		  }
		  qn_prv = qn;
		  qn = qn->next;              // next queue member
		 
	  } // while(qn)
		
	}  // while(1)
	return 1;
}

void DieWithError(char *errorMessage)
{
    perror(errorMessage);
    //exit(1);
}


multicast_net::multicast_net(char* ipAdress,unsigned short port)
{

	 #ifdef WIN32
	 if (WSAStartup(MAKEWORD(2,2), &localWSA) != 0)
	 {
			printf("WSAStartup error");
	 } 
	 #endif
	 // wpisanie adresu i portu ( zgodne juz z formatem )
	 multicastIP=inet_addr(ipAdress);
	 multicastPort=htons(port);
	 multicastTTL = 1;  
	  
	  
	 if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
			DieWithError("socket() failed");

	 q = NULL;
	 mean_delay = var_delay = 0;
	 

	 initS=0;
	 initR=0;
		printf("socket ok\n");
}

multicast_net::~multicast_net()
{
  //close(sock);
#ifdef WIN32
  WSACleanup(); // ********************* For Windows
#endif
  if (q)
  {
      SEND_QUEUE *qn = q;
	  while (q)
	  {
		  qn = q->next;
		  delete q->buf;
		  delete q;
		  q = qn;
	  }
  }
}

int multicast_net::init_recive()
{
    int on=1;
	fprintf(stderr,"init receiver ");
	setsockopt (sock, SOL_SOCKET, SO_REUSEADDR, (char *)&on, sizeof (on));

	if (!initS)
	{	
		memset(&multicastAddr, 0, sizeof(multicastAddr));   
		multicastAddr.sin_family = AF_INET;                  
		multicastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  
		multicastAddr.sin_port = multicastPort;      

		// Bind to the multicast port 
		if (bind(sock, (struct sockaddr *) &multicastAddr, sizeof(multicastAddr)) < 0)
			DieWithError("bind() failed");
	}
	
    // Specify the multicast group 
    multicastRequest.imr_multiaddr.s_addr = multicastIP;
    // Accept multicast from any interface 
    multicastRequest.imr_interface.s_addr = htonl(INADDR_ANY);
    // Join the multicast address 
    if (setsockopt(sock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *) &multicastRequest,
          sizeof(multicastRequest)) < 0)
        DieWithError("setsockopt() failed");
	//if (setsockopt(rsock, IPPROTO_IP, IP_ADD_MEMBERSHIP, (char *)&mreq, sizeof (mreq)) != 0)
	fprintf(stderr," ok\n");
	initR=1;
	return 1;

}

int multicast_net::init_send()
{
	fprintf(stderr,"init sender");

    // Set TTL of multicast packet 
    if (setsockopt(sock, IPPROTO_IP, IP_MULTICAST_TTL, (char *) &multicastTTL, 
          sizeof(multicastTTL)) < 0)
        DieWithError("setsockopt() failed");

    
    // Construct local address structure 
    memset(&multicastAddr, 0, sizeof(multicastAddr));   
    multicastAddr.sin_family = AF_INET;                 
    multicastAddr.sin_addr.s_addr = multicastIP;   
    multicastAddr.sin_port = multicastPort;         

    fprintf(stderr," ok\n");

	initS=1;

	
	return 1;

}

int multicast_net::send(char* buffer,int size)
{
	int sendsize;
	if (!initS) init_send();

	if ((sendsize=sendto(sock, buffer, size, 0, (struct sockaddr *) 
              &multicastAddr, sizeof(multicastAddr))) != size)
            DieWithError("sendto() sent a different number of bytes than expected");

	    return sendsize;
}

int multicast_net::send_delayed(char* buffer,int size)
{

    // average frame sending velocity computation:
   

	int indeks_akt = liczba_wyslanych % 20;
    long czas_akt = czasy_wys[indeks_akt] = clock();
	long czas_pop = (indeks_akt + 1 == 20 ? czasy_wys[0] : czasy_wys[indeks_akt + 1]); 
	if (czas_start == 0) czas_start = clock();
    if (liczba_wyslanych > 0)
	{
		int liczba_w = (liczba_wyslanych >= 20 ? 20 : liczba_wyslanych);
		//sr_czestosc = (float)liczba_w/((float)(czas_akt-czas_pop)/CLOCKS_PER_SEC );
        sr_czestosc = (float)liczba_wyslanych/((float)(czas_akt-czas_start)/CLOCKS_PER_SEC );
	}
	else 
	{
		for (long i=0;i<20;i++) czasy_wys[i] = clock();
	}
    liczba_wyslanych++;


	if ((mean_delay == 0)&&(var_delay == 0)) return send(buffer,size);  // original send
	else                                                                 
	{
		float delay = var_delay*2*((float)rand()/RAND_MAX-0.5) + mean_delay;  // delay in [sec]
		SEND_QUEUE *nowy = new SEND_QUEUE;
        nowy->buf = new char[size];
		memcpy(nowy->buf,buffer,size); 
		nowy->next = NULL;
		nowy->prev = NULL;
		nowy->delay = clock() + (long)((float)delay*CLOCKS_PER_SEC);
		nowy->size = size;
		//fprintf(f,"czas = %d, umieszczono ramke %d z czasem %d\n",clock(),nowy,nowy->delay);
		//fclose(f);
		//f = fopen("plik.txt","a");
		if (q == NULL) 
			q = nowy; 
		else
		{
			SEND_QUEUE *qn = q;
			while (qn->next)
				qn = qn->next;
			qn->next = nowy;
		}
		
	}
    
	
	return 1;
}

void multicast_net::PrepareDelay(float mean, float var)
{
	mean_delay = mean;
	var_delay = var;

	// thread for sending messages from queue
    DWORD dwThreadId;
    threadSend = CreateThread(
          NULL,                        // no security attributes
          0,                           // use default stack size
          SendThread,                // thread function
          (void *)this,               // argument to thread function
          0,                           // use default creation flags
          &dwThreadId);                // returns the thread identifier
}

int multicast_net::reciv(char* buffer,int maxsize)
{
	int recvLen;

	if (!initR) init_recive();

        
	if ((recvLen = recvfrom(sock, buffer, maxsize, 0, NULL, 0)) < 0)    DieWithError("recvfrom() failed");

	return recvLen;
	
}
// #########################################
// ##############   UNICAST  ###############
// #########################################

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
