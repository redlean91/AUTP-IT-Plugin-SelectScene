#include "pch.h"
#include "SelectScene.h"

//--------------------------------------------------------------------

BOOL func_init(AviUtl::FilterPlugin* fp)
{
	MY_TRACE(_T("func_init()\n"));

	// Ottieni gli indirizzi relativi all'Extended Edit.
	if (!g_auin.initExEditAddress())
		return FALSE;

	return TRUE;
}

BOOL func_exit(AviUtl::FilterPlugin* fp)
{
	MY_TRACE(_T("func_exit()\n"));

	return TRUE;
}

BOOL func_WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam, AviUtl::EditHandle* editp, AviUtl::FilterPlugin* fp)
{
//	MY_TRACE(_T("func_WndProc(0x%08X, 0x%08X, 0x%08X)\n"), message, wParam, lParam);

	switch (message)
	{
	case AviUtl::FilterPlugin::WindowMessage::Init:
		{
			MY_TRACE(_T("func_WndProc(Init, 0x%08X, 0x%08X)\n"), wParam, lParam);

			g_themeWindow = ::OpenThemeData(hwnd, VSCLASS_WINDOW);
			MY_TRACE_HEX(g_themeWindow);

			g_themeButton = ::OpenThemeData(hwnd, VSCLASS_BUTTON);
			MY_TRACE_HEX(g_themeButton);

			calcLayout(hwnd);

			// Ridisegna.
			::InvalidateRect(hwnd, 0, FALSE);

			break;
		}
	case AviUtl::FilterPlugin::WindowMessage::Exit:
		{
			MY_TRACE(_T("func_WndProc(Exit, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// Salva le impostazioni.
			saveConfig();

			::CloseThemeData(g_themeWindow), g_themeWindow = 0;
			::CloseThemeData(g_themeButton), g_themeButton = 0;

			break;
		}
	case AviUtl::FilterPlugin::WindowMessage::FileOpen:
		{
			MY_TRACE(_T("func_WndProc(FileOpen, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// Ridisegna.
			::InvalidateRect(hwnd, 0, FALSE);

			break;
		}
	case AviUtl::FilterPlugin::WindowMessage::Command:
		{
			MY_TRACE(_T("func_WndProc(Command, 0x%08X, 0x%08X)\n"), wParam, lParam);

			if (wParam == 0 && lParam == 0) return TRUE; // Ridisegna.

			break;
		}
	case WM_SIZE:
		{
			MY_TRACE(_T("func_WndProc(WM_SIZE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			calcLayout(hwnd, TRUE);

			break;
		}
	case WM_PAINT:
		{
			MY_TRACE(_T("func_WndProc(WM_PAINT, 0x%08X, 0x%08X)\n"), wParam, lParam);

			onPaint(hwnd, editp, fp);

			break;
		}
	case WM_LBUTTONDOWN:
		{
			MY_TRACE(_T("func_WndProc(WM_LBUTTONDOWN, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// Ottieni le coordinate del cursore del mouse.
			POINT point = LP2PT(lParam);

			// Ottieni la scena da cui iniziare il trascinamento.
			g_dragScene = hitTest(hwnd, point);
			MY_TRACE_INT(g_dragScene);

			// Se la scena trascinata non è valida
			if (!isSceneIndexValid(g_dragScene))
				break; // Non fare nulla.

			// Inizia la cattura del mouse.
			::SetCapture(hwnd);

			// Ridisegna.
			onPaint(hwnd, editp, fp);

			break;
		}
	case WM_LBUTTONUP:
		{
			MY_TRACE(_T("func_WndProc(WM_LBUTTONUP, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// Ottieni le coordinate del cursore del mouse.
			POINT point = LP2PT(lParam);

			// Se la cattura del mouse è attiva
			if (::GetCapture() == hwnd)
			{
				// Termina la cattura del mouse.
				::ReleaseCapture();

				// Ottieni la scena evidenziata.
				g_hotScene = hitTest(hwnd, point);
				MY_TRACE_INT(g_hotScene);

				// Se la scena trascinata e la scena evidenziata sono uguali
				if (g_dragScene == g_hotScene)
				{
					// Se la scena trascinata è valida e diversa dalla scena attuale
					if (isSceneIndexValid(g_dragScene) && g_dragScene != g_auin.GetCurrentSceneIndex())
					{
						playVoice(g_voice);

						// Il pulsante è stato premuto, quindi cambia scena.

						/*

							Vecchio metodo

							// Vedendo se il filtro esiste effettivamente
							AviUtl::FilterPlugin* exeditFilter = g_auin.GetFilter(fp, "Mod.Avan");

							if (exeditFilter == nullptr)
								// andando in fallback con un altro if statement 
								AviUtl::FilterPlugin* exeditFilter = g_auin.GetFilter(fp, "Adv.Edit");
						

							if (exeditFilter == nullptr)
								AviUtl::FilterPlugin* exeditFilter = g_auin.GetFilter(fp, "Advanced Editing");
						
						
							if (exeditFilter == nullptr)
								MessageBoxA(hwnd,
									"Impossibile trovare la finestra di Exedit con i titoli: (Mod.Avan/Adv.Edit/Advanced Editing)",
									"Errore di SelectScene", MB_OK | MB_ICONERROR);
								break;
						
							*/

						constexpr const char* filterNames[] = {
							"Mod.Avan",
							"Adv.Edit",
							"Advanced Editing"
						};

						AviUtl::FilterPlugin* exeditFilter = nullptr;

						for (const char* name : filterNames) {
							exeditFilter = g_auin.GetFilter(fp, name);
							if (exeditFilter != nullptr) break;
						}

						g_auin.SetScene(g_dragScene, exeditFilter, editp);

						if (exeditFilter == nullptr) {
							MessageBoxA(hwnd,
								"Impossibile trovare la finestra di Exedit con i titoli: (Mod.Avan/Adv.Edit/Advanced Editing)",
								"Errore di SelectScene", MB_OK | MB_ICONERROR);
						}

						// Ridisegna la finestra di anteprima di AviUtl.
						::PostMessage(hwnd, AviUtl::FilterPlugin::WindowMessage::Command, 0, 0);
					}
				}

				// Reimposta la scena trascinata al valore iniziale.
				g_dragScene = -1;

				// Ridisegna.
				onPaint(hwnd, editp, fp);
			}

			break;
		}
	case WM_MOUSEMOVE:
		{
//			MY_TRACE(_T("func_WndProc(WM_MOUSEMOVE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// Ottieni le coordinate del cursore del mouse.
			POINT point = LP2PT(lParam);

			if (::GetCapture() == hwnd)
			{
				// Ottieni la scena sotto le coordinate del mouse.
				int scene = hitTest(hwnd, point);

				// Se la scena trascinata è diversa dalla scena sotto il mouse
				if (g_dragScene != scene)
					scene = -1; // Invalida la scena sotto il mouse.

				// Se la scena evidenziata è diversa da quella sotto il mouse
				if (g_hotScene != scene)
				{
					// Aggiorna la scena evidenziata.
					g_hotScene = scene;

					// Ridisegna.
					onPaint(hwnd, editp, fp);
				}
			}
			else
			{
				// Ottieni la scena sotto le coordinate del mouse.
				int scene = hitTest(hwnd, point);

				// Se la scena evidenziata è diversa da quella sotto il mouse
				if (g_hotScene != scene)
				{
					// Aggiorna la scena evidenziata.
					g_hotScene = scene;

					// Se la scena evidenziata è valida e il mouse non è catturato
					if (g_hotScene >= 0)
					{
						// Abilita l'invio di WM_MOUSELEAVE.
						TRACKMOUSEEVENT tme = { sizeof(tme) };
						tme.dwFlags = TME_LEAVE;
						tme.hwndTrack = hwnd;
						::TrackMouseEvent(&tme);
					}

					// Ridisegna.
					onPaint(hwnd, editp, fp);
				}
			}

			break;
		}
	case WM_MOUSELEAVE:
		{
//			MY_TRACE(_T("func_WndProc(WM_MOUSELEAVE, 0x%08X, 0x%08X)\n"), wParam, lParam);

			// Se la scena evidenziata è valida
			if (g_hotScene >= 0)
			{
				// Reimposta la scena evidenziata al valore iniziale.
				g_hotScene = -1;

				// Ridisegna.
				onPaint(hwnd, editp, fp);
			}

			break;
		}
	case WM_RBUTTONUP:
		{
			MY_TRACE(_T("func_WndProc(WM_RBUTTONUP, 0x%08X, 0x%08X)\n"), wParam, lParam);

			onContextMenu(hwnd);

			break;
		}
	}

	return FALSE;
}

extern BOOL APIENTRY DllMain(HINSTANCE instance, DWORD reason, LPVOID reserved)
{
	switch (reason)
	{
	case DLL_PROCESS_ATTACH:
		{
			// Imposta la localizzazione.
			// Esegui prima questo per evitare problemi di codifica con il testo giapponese.
			_tsetlocale(LC_CTYPE, _T(""));

			MY_TRACE(_T("DLL_PROCESS_ATTACH\n"));

			// Salva l'handle di questa DLL in una variabile globale.
			g_instance = instance;
			MY_TRACE_HEX(g_instance);

			break;
		}
	case DLL_PROCESS_DETACH:
		{
			MY_TRACE(_T("DLL_PROCESS_DETACH\n"));

			break;
		}
	}

	return TRUE;
}

extern AviUtl::FilterPluginDLL* WINAPI GetFilterTable()
{
	MY_TRACE(_T("GetFilterTable()\n"));

	// Carica le impostazioni.
	loadConfig();

	LPCSTR name = "SelectScene";
	LPCSTR information = "SelectScene 2.1.1 by hebiiro, tradotto da Redlean";

	static AviUtl::FilterPluginDLL filter =
	{
		.flag =
			AviUtl::FilterPluginDLL::Flag::AlwaysActive |
			AviUtl::FilterPluginDLL::Flag::DispFilter |
			AviUtl::FilterPluginDLL::Flag::WindowThickFrame |
			AviUtl::FilterPluginDLL::Flag::WindowSize |
			AviUtl::FilterPluginDLL::Flag::ExInformation,
		.x = 400,
		.y = 400,
		.name = name,
		.func_init = func_init,
		.func_exit = func_exit,
		.func_WndProc = func_WndProc,
		.information = information,
	};

	if (g_fixedSize)
	{
		// Rimuovi il bordo di ridimensionamento in modalità a dimensione fissa.
		filter.flag &= ~AviUtl::FilterPluginDLL::Flag::WindowThickFrame;
	}

	return &filter;
}

//--------------------------------------------------------------------
