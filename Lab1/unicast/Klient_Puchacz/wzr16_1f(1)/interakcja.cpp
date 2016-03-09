/************************************************************
Interakcja:
Wysy�anie, odbi�r komunikat�w, interakcja z innymi
uczestnikami WZR, sterowanie wirtualnymi obiektami  
*************************************************************/

#include <windows.h>
#include <time.h>
#include <stdio.h>   
#include "interakcja.h"
#include "obiekty.h"
#include "siec.h"
#include "grafika.h"

FILE *f = fopen("wzr_plik.log","w");    // plik do zapisu informacji testowych
bool sendId;
int idKlientow[3];
ObiektRuchomy *pMojObiekt;          // obiekt przypisany do tej aplikacji
Teren teren;
int iLiczbaCudzychOb = 0;
ObiektRuchomy *CudzeObiekty[2000];  // obiekty z innych aplikacji lub inne obiekty niz pMojObiekt
int IndeksyOb[4000];                // tablica indeksow innych obiektow ulatwiajaca wyszukiwanie

float fDt;                          // sredni czas pomiedzy dwoma kolejnymi cyklami symulacji i wyswietlania
long czas_cyklu_WS,licznik_sym;     // zmienne pomocnicze potrzebne do obliczania fDt
long czas_start = clock();          // czas uruchomienia aplikacji

unicast_net *multi_reciv;         // wsk do obiektu zajmujacego sie odbiorem komunikatow
unicast_net *multi_send;          //   -||-  wysylaniem komunikatow

HANDLE threadReciv;                 // uchwyt w�tku odbioru komunikat�w
extern HWND okno;       
int SHIFTwcisniety = 0;
bool czy_rysowac_ID = 1;            // czy rysowac nr ID przy ka�dym obiekcie
bool sterowanie_myszkowe = 0;       // sterowanie za pomoc� klawisza myszki
int kursor_x = 0, kursor_y = 0;     // po�o�enie kursora myszy

// Parametry widoku:
Wektor3 kierunek_kamery_1 = Wektor3(11,-3,-14),   // kierunek patrzenia
pol_kamery_1 = Wektor3(-34,6,10),         // po�o�enie kamery
pion_kamery_1 = Wektor3(0,1,0),           // kierunek pionu kamery        
kierunek_kamery_2 = Wektor3(0,-1,0.02),   // to samo dla widoku z g�ry
pol_kamery_2 = Wektor3(0,90,0),
pion_kamery_2 = Wektor3(0,0,-1),
kierunek_kamery = kierunek_kamery_1, pol_kamery = pol_kamery_1, pion_kamery = pion_kamery_1; 
bool sledzenie = 1;                             // tryb �ledzenia obiektu przez kamer�
bool widok_z_gory = 0;                          // tryb widoku z g�ry
float oddalenie = 27.0;                          // oddalenie widoku z kamery
float kat_kam_z = 0;                            // obr�t kamery g�ra-d�
float oddalenie_1 = oddalenie, kat_kam_z_1 = kat_kam_z, oddalenie_2 = oddalenie, kat_kam_z_2 = kat_kam_z,
oddalenie_3 = oddalenie, kat_kam_z_3 = kat_kam_z;


enum typy_ramek{STAN_OBIEKTU};


struct Ramka                                    // g��wna struktura s�u��ca do przesy�ania informacji
{
  int typ;
  StanObiektu stan;  
  int idWymiany;
};


//******************************************
// Funkcja obs�ugi w�tku odbioru komunikat�w 
DWORD WINAPI WatekOdbioru(void *ptr)
{
  unicast_net *pmt_net=(unicast_net*)ptr;                // wska�nik do obiektu klasy multicast_net
  int rozmiar;                                               // liczba bajt�w ramki otrzymanej z sieci
  Ramka ramka;
  StanObiektu stan;
  unsigned long adres_nad=0;

  while(1)
  {
    rozmiar = pmt_net->reciv((char*)&ramka,&adres_nad,sizeof(Ramka));   // oczekiwanie na nadej�cie ramki - funkcja samoblokuj�ca si� 
    switch (ramka.typ) 
		{
			case STAN_OBIEKTU:
			{
        stan = ramka.stan;
		
        if (stan.iID != pMojObiekt->iID)                     // je�li to nie m�j obiekt
        {
	      idKlientow[0] = stan.iID;
          if (IndeksyOb[stan.iID] == -1)                     // nie ma jeszcze takiego obiektu w tablicy -> trzeba go stworzy�
          {
            CudzeObiekty[iLiczbaCudzychOb] = new ObiektRuchomy(&teren);   
            IndeksyOb[stan.iID] = iLiczbaCudzychOb;          // wpis do tablicy indeksowanej numerami ID
                                                             // u�atwia wyszukiwanie, alternatyw� mo�e by� tabl. rozproszona                                                                                               
            iLiczbaCudzychOb++;     
          }                   
		  else if (ramka.idWymiany != 0)
		  {
			  if (CudzeObiekty[0]->iID == ramka.idWymiany)
			  {
				  ObiektRuchomy *tmp;
				  tmp = CudzeObiekty[0];
				  CudzeObiekty[0] = pMojObiekt;
				  pMojObiekt = tmp;
			  }
		  }
          CudzeObiekty[IndeksyOb[stan.iID]]->ZmienStan(stan);   // zmiana stanu obiektu obcego 			
        }
        break;
      } // case STAN_OBIEKTU

    } // switch
  }  // while(1)
  return 1;
}

// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas uruchamiania aplikacji
// ****    poza grafik�   
void PoczatekInterakcji()
{
  DWORD dwThreadId;

  pMojObiekt = new ObiektRuchomy(&teren);    // tworzenie wlasnego obiektu

  for (long i=0;i<4000;i++)            // inicjacja indeksow obcych obiektow
    IndeksyOb[i] = -1;

  czas_cyklu_WS = clock();             // pomiar aktualnego czasu

  // obiekty sieciowe typu multicast (z podaniem adresu WZR oraz numeru portu)
  multi_reciv = new unicast_net(10002);      // obiekt do odbioru ramek sieciowych
  multi_send = new unicast_net(10001);       // obiekt do wysy�ania ramek


  // uruchomienie watku obslugujacego odbior komunikatow
  threadReciv = CreateThread(
      NULL,                        // no security attributes
      0,                           // use default stack size
      WatekOdbioru,                // thread function
      (void *)multi_reciv,               // argument to thread function
      0,                           // use default creation flags
      &dwThreadId);                // returns the thread identifier

}


// *****************************************************************
// ****    Wszystko co trzeba zrobi� w ka�dym cyklu dzia�ania 
// ****    aplikacji poza grafik� 
void Cykl_WS()
{
  licznik_sym++;  

  // obliczenie czasu fDt pomiedzy dwoma kolejnymi cyklami
  if (licznik_sym % 50 == 0)          // je�li licznik cykli przekroczy� pewn� warto��, to
  {                                   // nale�y na nowo obliczy� �redni czas cyklu fDt
    char text[200];
    long czas_pop = czas_cyklu_WS;
    czas_cyklu_WS = clock();
    float fFps = (50*CLOCKS_PER_SEC)/(float)(czas_cyklu_WS-czas_pop);
    if (fFps!=0) fDt=1.0/fFps; else fDt=1;
    sprintf(text,"WZR-lab 2015 temat 1, wersja f (%0.0f fps  %0.2fms)  ",fFps,1000.0/fFps);
    SetWindowText(okno,text); // wy�wietlenie aktualnej ilo�ci klatek/s w pasku okna			
  }   


  pMojObiekt->Symulacja(fDt);                    // symulacja obiektu w�asnego 

  Ramka ramka;                          
  ramka.typ = STAN_OBIEKTU;
  ramka.stan = pMojObiekt->Stan();
  ramka.idWymiany = 0;
  //printf("%d \n", ramka.idWymiany);


  // wys�anie komunikatu o stanie obiektu przypisanego do aplikacji (pMojObiekt): 
  if (sendId)
  {
	  ramka.idWymiany = idKlientow[0];
	  multi_send->send((char*)&ramka, "192.168.1.138", sizeof(Ramka));
	  sendId = false;
  }
  else
  {
	  ramka.idWymiany = 0;
	  multi_send->send((char*)&ramka, "192.168.1.138", sizeof(Ramka));
	  sendId = false;
  }
  
}

// *****************************************************************
// ****    Wszystko co trzeba zrobi� podczas zamykania aplikacji
// ****    poza grafik� 
void ZakonczenieInterakcji()
{
  fprintf(f,"Interakcja zosta�a zako�czona\n");
  fclose(f);
}


// ************************************************************************
// ****    Obs�uga klawiszy s�u��cych do sterowania obiektami lub
// ****    widokami 
void KlawiszologiaSterowania(UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{

  switch (kod_meldunku) 
  {

  case WM_LBUTTONDOWN: //reakcja na lewy przycisk myszki
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
        pMojObiekt->F = 40.0;        // si�a pchaj�ca do przodu
      break;
    }
  case WM_RBUTTONDOWN: //reakcja na prawy przycisk myszki
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
        pMojObiekt->F = -8.0;        // si�a pchaj�ca do tylu
      
      break;
    }
  case WM_MBUTTONDOWN: //reakcja na �rodkowy przycisk myszki : uaktywnienie/dezaktywacja sterwania myszkowego
    {
      sterowanie_myszkowe = 1 - sterowanie_myszkowe;
      kursor_x = LOWORD(lParam);
      kursor_y = HIWORD(lParam);
      break;
    }
  case WM_LBUTTONUP: //reakcja na puszczenie lewego przycisku myszki
    {   
      if (sterowanie_myszkowe)
        pMojObiekt->F = 0.0;        // si�a pchaj�ca do przodu
      break;
    }
  case WM_RBUTTONUP: //reakcja na puszczenie lewy przycisk myszki
    {
      if (sterowanie_myszkowe)
        pMojObiekt->F = 0.0;        // si�a pchaj�ca do przodu
      break;
    }
  case WM_MOUSEMOVE:
    {
      int x = LOWORD(lParam);
      int y = HIWORD(lParam);
      if (sterowanie_myszkowe)
      {
        float kat_skretu = (float)(kursor_x - x)/20;
        if (kat_skretu > 45) kat_skretu = 45;
        if (kat_skretu < -45) kat_skretu = -45;	
        pMojObiekt->alfa = PI*kat_skretu/180;
      }
      break;
    }
  case WM_KEYDOWN:
    {

      switch (LOWORD(wParam))
      {
      case VK_SHIFT:
        {
          SHIFTwcisniety = 1; 
          break; 
        }        
      case VK_SPACE:
        {
          pMojObiekt->ham = 10.0;       // stopie� hamowania (reszta zale�y od si�y docisku i wsp. tarcia)
          break;                       // 1.0 to maksymalny stopie� (np. zablokowanie k�)
        }
      case VK_UP:
        {

          pMojObiekt->F = 50.0;        // si�a pchaj�ca do przodu
          break;
        }
      case VK_DOWN:
        {
          pMojObiekt->F = -15.0;
          break;
        }
      case VK_LEFT:
        {
          if (SHIFTwcisniety) pMojObiekt->alfa = PI*25/180;
          else pMojObiekt->alfa = PI*10/180;

          break;
        }
      case VK_RIGHT:
        {
          if (SHIFTwcisniety) pMojObiekt->alfa = -PI*25/180;
          else pMojObiekt->alfa = -PI*10/180;
          break;
        }
      case 'I':   // wypisywanie nr ID
        {
          czy_rysowac_ID = 1 - czy_rysowac_ID;
          break;
        }
      case 'W':   // oddalenie widoku
        {
          //pol_kamery = pol_kamery - kierunek_kamery*0.3;
          if (oddalenie > 0.5) oddalenie /= 1.2;
          else oddalenie = 0;  
          break; 
        }     
      case 'S':   // przybli�enie widoku
        {
          //pol_kamery = pol_kamery + kierunek_kamery*0.3; 
          if (oddalenie > 0) oddalenie *= 1.2;
          else oddalenie = 0.5;   
          break; 
        }
	  case 'L':
	  {
	      sendId = true;
		  ObiektRuchomy * temp = CudzeObiekty[0];
		  CudzeObiekty[0] = pMojObiekt;
		  pMojObiekt = temp;
		  break;
	  }
      case 'Q':   // widok z g�ry
        {
          //if (sledzenie) break;
          widok_z_gory = 1-widok_z_gory;
          if (widok_z_gory)                // przechodzimy do widoku z g�ry
          {
            pol_kamery_1 = pol_kamery; kierunek_kamery_1 = kierunek_kamery; pion_kamery_1 = pion_kamery;
            oddalenie_1 = oddalenie; kat_kam_z_1 = kat_kam_z; 

            //pol_kamery = pol_kamery_2; kierunek_kamery = kierunek_kamery_2; pion_kamery = pion_kamery_2;
            oddalenie = oddalenie_2; kat_kam_z = kat_kam_z_2; 
            // Po�o�enie kamery, kierunek oraz pion ustawiamy tak, by obiekt widziany by� z g�ry i jecha�
            // pocz�tkowo w g�r� ekranu:
            kierunek_kamery = (pMojObiekt->wPol - teren.srodek).znorm()*(-1);
            pol_kamery = pMojObiekt->wPol - kierunek_kamery*pMojObiekt->dlugosc*10;
            pion_kamery = pMojObiekt->qOrient.obroc_wektor(Wektor3(1,0,0));
          }
          else
          {
            pol_kamery_2 = pol_kamery; kierunek_kamery_2 = kierunek_kamery; pion_kamery_2 = pion_kamery;
            oddalenie_2 = oddalenie; kat_kam_z_2 = kat_kam_z;

            // Po�o�enie kamery, kierunek oraz pion ustawiamy tak, by obiekt widziany by� z prawego boku i jecha�
            // pocz�tkowo ze strony lewej na praw�:
            kierunek_kamery = pMojObiekt->qOrient.obroc_wektor(Wektor3(0,0,1))*-1;
            pol_kamery = pMojObiekt->wPol - kierunek_kamery*pMojObiekt->dlugosc*10;
            pion_kamery = (pMojObiekt->wPol - teren.srodek).znorm();

            //pol_kamery = pol_kamery_1; kierunek_kamery = kierunek_kamery_1; pion_kamery = pion_kamery_1;
            oddalenie = oddalenie_1; kat_kam_z = kat_kam_z_1; 
          }
          break;
        }
      case 'E':   // obr�t kamery ku g�rze (wzgl�dem lokalnej osi z)
        {
          kat_kam_z += PI*5/180; 
          break; 
        }    
      case 'D':   // obr�t kamery ku do�owi (wzgl�dem lokalnej osi z)
        {
          kat_kam_z -= PI*5/180;  
          break;
        }
      case 'A':   // w��czanie, wy��czanie trybu �ledzenia obiektu
        {
          sledzenie = 1 - sledzenie;
          if (sledzenie)
          {
            oddalenie = oddalenie_3; kat_kam_z = kat_kam_z_3; 
          }
          else
          {
            oddalenie_3 = oddalenie; kat_kam_z_3 = kat_kam_z; 
            widok_z_gory = 0;
            pol_kamery = pol_kamery_1; kierunek_kamery = kierunek_kamery_1; pion_kamery = pion_kamery_1;
            oddalenie = oddalenie_1; kat_kam_z = kat_kam_z_1; 
          }
          break;
        }
      case VK_ESCAPE:
        {
          SendMessage(okno, WM_DESTROY,0,0);
          break;
        }
      } // switch po klawiszach

      break;
    }
  case WM_KEYUP:
    {
      switch (LOWORD(wParam))
      {
      case VK_SHIFT:
        {
          SHIFTwcisniety = 0; 
          break; 
        }        
      case VK_SPACE:
        {
          pMojObiekt->ham = 0.0;
          break;
        }
      case VK_UP:
        {
          pMojObiekt->F = 0.0;

          break;
        }
      case VK_DOWN:
        {
          pMojObiekt->F = 0.0;
          break;
        }
      case VK_LEFT:
        {
          pMojObiekt->Fb = 0.00;
          pMojObiekt->alfa = 0;	
          break;
        }
      case VK_RIGHT:
        {
          pMojObiekt->Fb = 0.00;
          pMojObiekt->alfa = 0;	
          break;
        }

      }

      break;
    }

  } // switch po komunikatach
}
