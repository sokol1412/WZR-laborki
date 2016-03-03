// Serwer.cpp : Defines the entry point for the console application.
//
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define _CRT_SECURE_NO_WARNINGS
#include "stdafx.h"
#include <WinSock2.h>
#include <stdio.h>      /* for printf() and fprintf() */
#include <stdlib.h>     /* for atoi() and exit() */
#include <string.h>     /* for memset() */
#include <time.h>
#include "siec.h"
#include "wektor.h"
#include "kwaternion.h"
#define MYPORT 10001    // port, z kt�rym u�ytkownicy b�d� si� ��czyli

#define MAXBUFLEN 10000

struct StanObiektu
{
	int iID;                  // identyfikator obiektu
	Wektor3 wPol;             // polozenie obiektu (�rodka geometrycznego obiektu) 
	kwaternion qOrient;       // orientacja (polozenie katowe)
	Wektor3 wV, wA;            // predkosc, przyspiesznie liniowe
	Wektor3 wV_kat, wA_kat;   // predkosc i przyspieszenie liniowe
	float masa;
};
struct Ramka                                    // g��wna struktura s�u��ca do przesy�ania informacji
{
	int typ;
	StanObiektu stan;
};



int _tmain(int argc, _TCHAR* argv[])
{
	unicast_net *multi_reciv;
	unicast_net *multi_send;
	multi_reciv = new unicast_net(10001);
	multi_send = new unicast_net(10002);    
	unsigned long adres = 0;

	Ramka ramka;
	while (1){
	
		multi_reciv->reciv((char*)&ramka, &adres, sizeof(Ramka));
		printf("%d\n", ramka.stan.iID);
		multi_send->send((char*)&ramka, "192.168.1.129", sizeof(Ramka));
		multi_send->send((char*)&ramka, "192.168.1.118", sizeof(Ramka));

	}
	
}
