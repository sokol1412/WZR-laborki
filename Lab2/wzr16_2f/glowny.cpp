/****************************************************
    Wirtualne zespoly robocze - przykladowy projekt w C++
    Do zada� dotycz�cych wsp�pracy, ekstrapolacji i 
    autonomicznych obiekt�w
 ****************************************************/

#include <windows.h>
#include <math.h>
#include <time.h>

#include <gl\gl.h>
#include <gl\glu.h>

#include "obiekty.h"
#include "grafika.h"
#include "interakcja.h"


//deklaracja funkcji obslugi okna
LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);


HWND okno;                   // uchwyt do okna aplikacji
HDC g_context = NULL;        // uchwyt kontekstu graficznego



//funkcja Main - dla Windows
int WINAPI WinMain(HINSTANCE hInstance,
               HINSTANCE hPrevInstance,
               LPSTR     lpCmdLine,
               int       nCmdShow)
{
	MSG meldunek;		  //innymi slowy "komunikat"
	WNDCLASS nasza_klasa; //klasa g��wnego okna aplikacji

	static char nazwa_klasy[] = "KlasaPodstawowa";

	//Definiujemy klase g��wnego okna aplikacji
	//Okreslamy tu wlasciwosci okna, szczegoly wygladu oraz
	//adres funkcji przetwarzajacej komunikaty
	nasza_klasa.style         = CS_HREDRAW | CS_VREDRAW;
	nasza_klasa.lpfnWndProc   = WndProc; //adres funkcji realizuj�cej przetwarzanie meldunk�w 
 	nasza_klasa.cbClsExtra    = 0 ;
	nasza_klasa.cbWndExtra    = 0 ;
	nasza_klasa.hInstance     = hInstance; //identyfikator procesu przekazany przez MS Windows podczas uruchamiania programu
	nasza_klasa.hIcon         = 0;
	nasza_klasa.hCursor       = LoadCursor(0, IDC_ARROW);
	nasza_klasa.hbrBackground = (HBRUSH) GetStockObject(GRAY_BRUSH);
	nasza_klasa.lpszMenuName  = "Menu" ;
	nasza_klasa.lpszClassName = nazwa_klasy;

    //teraz rejestrujemy klas� okna g��wnego
    RegisterClass (&nasza_klasa);
	
	/*tworzymy okno g��wne
	okno b�dzie mia�o zmienne rozmiary, listw� z tytu�em, menu systemowym
	i przyciskami do zwijania do ikony i rozwijania na ca�y ekran, po utworzeniu
	b�dzie widoczne na ekranie */
 	okno = CreateWindow(nazwa_klasy, "WZR-lab 2016 temat 2 (Nawigacja obl.), wersja f", WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN | WS_CLIPSIBLINGS,
						100, 50, 1000, 650, NULL, NULL, hInstance, NULL);
	
	
	ShowWindow (okno, nCmdShow) ;
    
	//odswiezamy zawartosc okna
	UpdateWindow (okno) ;

	// G��WNA P�TLA PROGRAMU
	
  // pobranie komunikatu z kolejki je�li funkcja PeekMessage zwraca warto�� inn� ni� FALSE,
	// w przeciwnym wypadku symulacja wirtualnego �wiata wraz z wizualizacj�
  ZeroMemory( &meldunek, sizeof(meldunek) );
  while( meldunek.message!=WM_QUIT )
  {
    if( PeekMessage( &meldunek, NULL, 0U, 0U, PM_REMOVE ) )
    {
      TranslateMessage( &meldunek );
      DispatchMessage( &meldunek );
    }
    else
    {
      Cykl_WS();    // Cykl wirtualnego �wiata
      InvalidateRect(okno, NULL, FALSE);
    }
  }
	return (int)meldunek.wParam;
}

/********************************************************************
FUNKCJA OKNA realizujaca przetwarzanie meldunk�w kierowanych do okna aplikacji*/
LRESULT CALLBACK WndProc (HWND okno, UINT kod_meldunku, WPARAM wParam, LPARAM lParam)
{
	    	
    // PONI�SZA INSTRUKCJA DEFINIUJE REAKCJE APLIKACJI NA POSZCZEG�LNE MELDUNKI 
	KlawiszologiaSterowania(kod_meldunku, wParam, lParam);

	switch (kod_meldunku) 
	{
	case WM_CREATE:  //meldunek wysy�any w momencie tworzenia okna
		{
			
			g_context = GetDC(okno);

			srand( (unsigned)time( NULL ) );
            int wynik = InicjujGrafike(g_context);
			if (wynik == 0)
			{
				printf("nie udalo sie otworzyc okna graficznego\n");
				//exit(1);
			}

			PoczatekInterakcji();

			SetTimer(okno, 1, 10, NULL);
						
			return 0;
		}
	
       
	case WM_PAINT:
		{
			PAINTSTRUCT paint;
			HDC kontekst;
			kontekst = BeginPaint(okno, &paint);
		
			RysujScene();			
			SwapBuffers(kontekst);

			EndPaint(okno, &paint);

			

			return 0;
		}

	case WM_SIZE:
		{
			int cx = LOWORD(lParam);
			int cy = HIWORD(lParam);

			ZmianaRozmiaruOkna(cx,cy);
			
			return 0;
		}
  	
	case WM_DESTROY: //obowi�zkowa obs�uga meldunku o zamkni�ciu okna

		ZakonczenieInterakcji();

		ZakonczenieGrafiki();


		ReleaseDC(okno, g_context);
		KillTimer(okno, 1);



		PostQuitMessage (0) ;
		return 0;
    
	default: //standardowa obs�uga pozosta�ych meldunk�w
		return DefWindowProc(okno, kod_meldunku, wParam, lParam);
	}
	
	
}

