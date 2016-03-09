#include <windows.h>
#include "siec.h"
#include "interakcja.h"
#include "obiekty.h"



struct Ramka                                    // g³ówna struktura s³u¿¹ca do przesy³ania informacji
{
	int typ;
	StanObiektu stan;
	int idZmiana;
}ramka;

struct Klient{
	int iID;
	unsigned long ip;
}klienci[3];




DWORD WINAPI odbior()
{
	unicast_net *uni_reciv = new unicast_net(10001);      // obiekt do odbioru ramek sieciowych
	unicast_net *uni_send = new unicast_net(10002);       // obiekt do wysy³ania ramek

	int rozmiar;                                               // liczba bajtów ramki otrzymanej z sieci
	bool nowy=1;
	StanObiektu stan;
	unsigned long ip = 0;
	while (1)
	{
		uni_reciv->reciv((char*)&ramka, &ip, sizeof(Ramka));   // oczekiwanie na nadejœcie ramki - funkcja samoblokuj¹ca siê
		printf("%d\n\n", ramka.stan.iID);

		if (ramka.idZmiana != 0){
			printf("%d\n", ramka.idZmiana);
		}
		
		for (int i = 0; i < 3; i++){
			if (klienci[i].ip == ip){
				nowy = 0;
			}
		}

		if (nowy){
			int i = 0;
			while (klienci[i++].iID != 0);
			klienci[i - 1].iID = ramka.stan.iID;
			klienci[i - 1].ip = ip;
		}
		nowy = 1;
		
		if (ramka.idZmiana != 0){
			for (int i = 0; i < 3; i++){
				if (ip != klienci[i].ip){
					if (ramka.idZmiana == klienci[i].iID){
						ramka.idZmiana = ramka.stan.iID;
						printf("HALO: %d\n", ramka.idZmiana);
						uni_send->send((char*)&ramka, klienci[i].ip, sizeof(Ramka));
					}
					
						ramka.idZmiana = 0;
						uni_send->send((char*)&ramka, klienci[i].ip, sizeof(Ramka));
					
				}
			}
		}
		

			uni_send->send((char*)&ramka, "192.168.1.142", sizeof(Ramka));
			uni_send->send((char*)&ramka, "192.168.1.148", sizeof(Ramka));
			uni_send->send((char*)&ramka, "192.168.1.138", sizeof(Ramka));
		
		
	} 
	return 1;
}


int main(){

	odbior();

}




