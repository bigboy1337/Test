/*
* Should probably re-write this from scratch
* Needs optimization(implementation of scatter reads, just better/more optimized code, etc..) and better fail-safes, logging etc.
* More readable/cleaner code by for example creating a console header for output to reduce lines written for setting color
* etc..
* 
* Works as is, but lots to do if you want the "perfect" code.
* 
* I tried commenting it as best I could, if I missed something. Soz.
* I don't recommend actually using this as a base and instead trying to improve it or re-write it from scratch.
*/

#pragma warning(disable:4200) // ignore vmm header and lc header throwing warnings

#include <Windows.h>                    // include Windows API header                               [system(), SetConsoleTextAttribute(), ...]
#include <cstdio>                       // include standard input/output functions                  [printf()]
#include <conio.h>                      // include console input/output functions                   [_getch()]
#include <string>                       // include string class for string manipulation             [std::string]
#include <thread>                       // include thread library for multithreading                [std::thread, std::this_thread::sleep_for()]
#include <fstream>                      // include file stream library for file input/output        [std::ifstream, std::ofstream]
#include <sstream>                      // include string stream library for string manipulation    [std::stringstream]
#include <iomanip>                      // include manipulators library for formatted output        [std::setw(), std::setfill()]
#include <unordered_map>                // include unordered map class for fast key-value pairs     [std::unordered_map]
#include "file-parser.h"                // include file parser header
#include "pcileech/vmmdll.h"            // include PCILeech memory manipulation library header
#pragma comment(lib, "pcileech/vmm")    // include PCILeech memory manipulation library

// define constants for arrow keys and enter key
#define UP_ARROW   72
#define DOWN_ARROW 80
#define ENTER      13

// pre-declarations
void SETUP_Handler();
void EXIT_Handler();
void CHEAT_Handler();
void GET_FPGA();
bool GET_Process();
bool GET_Offsets();
void GET_InGame();
void UPDATE_PlayerList();
void UPDATE_LocalPlayer();

namespace globals
{
    // application
    int	                    INIT_CHOICE             = NULL;
    VMM_HANDLE              hVMM                    = NULL;
    bool                    INIT_SUCCESS            = NULL;
    bool		            CHEAT_RUNNING           = true;
    bool		            IN_GAME                 = NULL;

    // board data
    ULONG64		            qwID                    = NULL;
    ULONG64		            qwVersionMajor          = NULL;
    ULONG64		            qwVersionMinor          = NULL;
    std::string             fpgaName                = "";

    // process data
    DWORD		            dwPID                   = NULL;
    QWORD		            qProcessBase            = NULL;

    // memory
    DWORD		            dwEntityList            = NULL;
    DWORD		            dwLocalPlayer           = NULL;
    DWORD		            dwHighlightEnable       = NULL; // HighlightSettings->m_highlightServerContextID + 0x8
    DWORD		            dwHighlightOcclusion    = NULL; // HighlightSettings->m_highlightServerContextID + 0x10
    DWORD		            dwHighlightColor        = NULL; // HighlightSettings->m_highlightParams + 0x18
    DWORD		            dwSignifierName         = NULL; // DT_BaseEntity->m_iSignifierName
    DWORD		            dwTeamNum               = NULL; // DT_BaseEntity->m_iTeamNum
    DWORD		            dwHealth                = NULL; // DT_Player->m_iHealth
    DWORD		            dwShieldHealth          = NULL; // DT_BaseEntity->m_shieldHealth
    DWORD		            dwLevelName             = NULL;
    std::vector<uintptr_t>  pEntityVector;
    uintptr_t				pLocalPlayer            = NULL;
    int						localTeam               = NULL;

    std::string             sEntityList             = "";
    std::string             sLocalPlayer            = "";
    std::string             sHighlightEnable        = "";
    std::string             sHighlightOcclusion     = "";
    std::string             sHighlightColor         = "";
    std::string             sSignifierName          = "";
    std::string             sTeamNum                = "";
    std::string             sHealth                 = "";
    std::string             sShieldHealth           = "";
    std::string             sLevelName              = "";
}

void Pause()
{
    printf("PRESS ANY KEY TO CONTINUE ...\n");
    Sleep(250);
    _getch();
}

int main()
{
    // local variable to store the user's choice
    int choice = 1;

    // loop until the user makes a selection
    while (true)
    {
        // clear the screen
        system("cls");

        // print menu options
        printf("How do you want to initalize your board:\n");

        // if choice is 1, highlight mmap option
        if (choice == 1)
        {
            // set text color to intense purple
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
            printf("--> MEMORY MAP\n");
            // set text color back to normal
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else
        {
            // choice is not 1, so don't highlight mmap option
            printf("-> MEMORY MAP\n");
        }

        // if choice is 2, highlight direct option
        if (choice == 2)
        {
            // set text color to intense purple
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
            printf("--> DIRECT\n");
            // set text color back to normal
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else
        {
            // choice is not 2, so don't highlight direct option
            printf("-> DIRECT\n");
        }

        // if choice is 3, highlight exit option
        if (choice == 3)
        {
            // set text color to intense purple
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
            printf("--> EXIT\n");
            // set text color back to normal
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);
        }
        else
        {
            // choice is not 3, so don't highlight exit option
            printf("-> EXIT\n");
        }

        // get user's input character
        char input = _getch();

        // check which key the user pressed
        if (input == UP_ARROW)
        {
            // decrement choice, wrap around if necessary
            choice--;
            if (choice < 1)
            {
                choice = 3;
            }
        }
        else if (input == DOWN_ARROW)
        {
            // increment choice, wrap around if necessary
            choice++;
            if (choice > 3)
            {
                choice = 1;
            }
        }
        else if (input == ENTER)
        {
            if (choice == 1 || choice == 2)
            {
                globals::INIT_CHOICE = choice;
            }

            // exit loop
            break;
        }
    }
    if (choice == 3)
    {
        return EXIT_SUCCESS;
    }

    // clear the screen
    system("cls");

    const std::string ASCII_ART = R"(
   _____   _____ _____ _      ______ ______ _____ _    _
  |  __ \ / ____|_   _| |    |  ____|  ____/ ____| |  | |
  | |__) | |      | | | |    | |__  | |__ | |    | |__| |
  |  ___/| |      | | | |    |  __| |  __|| |    |  __  |
  | |    | |____ _| |_| |____| |____| |___| |____| |  | |
  |_|     \_____|_____|______|______|______\_____|_|  |_|
            _____  ______ _   __
      /\   |  __ \|  ____\ \ / /
     /  \  | |__) | |__   \ V /
    / /\ \ |  ___/|  __|   > <
   / ____ \| |    | |____ / . \
  /_/    \_\_|    |______/_/ \_\


)";

    // set text color to intense purple
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE);
    printf(ASCII_ART.c_str());
    // set text color back to normal
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);

    // create a new thread to handle setup
    std::thread setupThread(SETUP_Handler);
    setupThread.join(); // <-- main() has to wait for setup to complete before proceeding

    if (globals::INIT_SUCCESS)
    {
        // grab information about the fpga board
        GET_FPGA();

        // grab process data
        if (GET_Process())
        {
            // grab offsets
            if (GET_Offsets())
            {
                // create new threads for smoother operation
                std::thread exitThread(EXIT_Handler);                 // thread for handling exiting the application
                std::thread statusThread(GET_InGame);                 // thread for checking if user is in-game
                std::thread playerThread(UPDATE_PlayerList);          // thread for updating player cache
                std::thread localplayerThread(UPDATE_LocalPlayer);    // thread for updating localplayer

                // run the cheat
                CHEAT_Handler();
            }
            else
            {
                // close vmm_handle and exit with status: FAILURE
                VMMDLL_Close(globals::hVMM);
                return EXIT_FAILURE;
            }
        }
        else
        {
            // close vmm_handle and exit with status: FAILURE
            VMMDLL_Close(globals::hVMM);
            return EXIT_FAILURE;
        }
    }
    else
    {
        // close vmm_handle and exit with status: FAILURE
        VMMDLL_Close(globals::hVMM);
        return EXIT_FAILURE;
    }

    // close vmm_handle and exit with status: SUCCESS
    VMMDLL_Close(globals::hVMM);

    // program completed successfully
    return EXIT_SUCCESS;
}

void SETUP_Handler()
{
    bool bResult;
    std::ifstream file;

    if (globals::INIT_CHOICE == 1)
    {
        file.open("mmap.txt");
        if (file)
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
            printf("[INFO] ");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
            printf("mmap.txt found!\n       Attempting to initiate your FPGA device with mmap.txt..\n");

            LPSTR args[] = { (LPSTR)"", (LPSTR)"-device", (LPSTR)"fpga", (LPSTR)"-memmap", (LPSTR)"mmap.txt" };
            globals::hVMM = VMMDLL_Initialize(3, args);
            if (globals::hVMM)
            {
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
                printf("[OK] ");
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
                printf("  Initialized!\n\n");

                globals::INIT_SUCCESS = true;
            }
            else
            {
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
                printf("[ERROR] ");
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
                printf("Failure! Try deleting your mmap.txt and re-generating it.\n\n");

                globals::INIT_SUCCESS = false;

                Pause();
            }
        }
        else
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
            printf("[INFO] ");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
            printf("No mmap.txt found!\n       Attempting to generate a new one..\n\n");

            LPSTR args[] = { (LPSTR)"-device", (LPSTR)"fpga", (LPSTR)"-v" };
            globals::hVMM = VMMDLL_Initialize(3, args);
            if (globals::hVMM)
            {
                PVMMDLL_MAP_PHYSMEM pPhysMemMap = NULL;
                bResult = VMMDLL_Map_GetPhysMem(globals::hVMM, &pPhysMemMap);

                if (!bResult)
                {
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
                    printf("[ERROR] ");
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
                    printf("Map_GetPhysMem() returned no entries!\n          Maybe try restarting your computer(s) and make sure your FPGA device is properly seated.\n\n");

                    globals::INIT_SUCCESS = false;

                    Pause();
                }
                if (pPhysMemMap->dwVersion != VMMDLL_MAP_PHYSMEM_VERSION)
                {
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
                    printf("[ERROR] ");
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
                    printf("Map_GetPhysMem() - BAD VERSION\n\n");

                    VMMDLL_MemFree(pPhysMemMap);
                    pPhysMemMap = NULL;

                    globals::INIT_SUCCESS = false;

                    Pause();
                }
                {
                    std::stringstream sb;
                    for (DWORD i = 0; i < pPhysMemMap->cMap; i++)
                    {
                        printf("       %04i %12llx - %12llx\n\n", i, pPhysMemMap->pMap[i].pa, pPhysMemMap->pMap[i].pa + pPhysMemMap->pMap[i].cb - 1);
                        sb << std::setfill('0') << std::setw(4) << i << "  " << std::hex << pPhysMemMap->pMap[i].pa << "  -  " << (pPhysMemMap->pMap[i].pa + pPhysMemMap->pMap[i].cb - 1) << "  ->  " << pPhysMemMap->pMap[i].pa << std::endl;
                    }
                    std::ofstream nFile("mmap.txt");
                    nFile << sb.str();
                    nFile.close();

                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
                    printf("[OK] ");
                    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
                    printf("  Successfully generated new mmap.txt!\n       Will now close after a few seconds, just run it and select mmap again.\n\n");

                    VMMDLL_MemFree(pPhysMemMap);
                    pPhysMemMap = NULL;

                    globals::INIT_SUCCESS = false;

                    std::this_thread::sleep_for(std::chrono::seconds(8));
                }
            }
            else
            {
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
                printf("[ERROR] ");
                SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
                printf("Unable to initialize FPGA device while attempting to generate new mmap.txt!\n\n");

                globals::INIT_SUCCESS = false;

                // "pause" the program
                Pause();
            }
        }
    }
    else if (globals::INIT_CHOICE == 2)
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("[INFO] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("Attempting to initiate your FPGA device directly..\n");

        LPSTR args[] = { (LPSTR)"-device", (LPSTR)"fpga" };
        globals::hVMM = VMMDLL_Initialize(3, args);
        if (globals::hVMM)
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
            printf("[OK] ");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
            printf("Initialized!\n\n");

            globals::INIT_SUCCESS = true;
        }
        else
        {
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
            printf("[ERROR] ");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
            printf("Failure!\n          Make sure you have all the libraries you need accessible. (vmm, leech and ftdi)\n          If you do and it still does not work, try restarting your target computer with a full power reset.\n\n");

            globals::INIT_SUCCESS = false;

            Pause();
        }
    }
}

void EXIT_Handler()
{
    // loop until CHEAT_RUNNING is set to false
    while (globals::CHEAT_RUNNING)
    {
        // check if the END key is pressed
        if (GetAsyncKeyState(VK_END) & 1)
        {
            // set CHEAT_RUNNING to false to exit the loop and the application
            globals::CHEAT_RUNNING = false;
        }

        // sleep for 1 millisecond before checking again
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}

void GET_FPGA()
{
    // boolean to store the result of checking the board
    bool bResult;

    // print message indicating that we are checking the board
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
    printf("[INFO] ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
    printf("Checking type of FPGA board..\n");

    // check the board
    bResult = VMMDLL_ConfigGet(globals::hVMM, LC_OPT_FPGA_FPGA_ID, &globals::qwID) && VMMDLL_ConfigGet(globals::hVMM, LC_OPT_FPGA_VERSION_MAJOR, &globals::qwVersionMajor) && VMMDLL_ConfigGet(globals::hVMM, LC_OPT_FPGA_VERSION_MINOR, &globals::qwVersionMinor);

    // if the check was successful
    if (bResult)
    {
        // create a lookup table for board names
        std::unordered_map<unsigned long long, std::string> fpgaNameLookup = {
                    { 0x0, "SP605 / FT601"          },
                    { 0x1, "PCIeScreamer R1"        },
                    { 0x2, "AC701 / FT601"          },
                    { 0x3, "PCIeScreamer R2"        },
                    { 0x4, "ScreamerM2"             },
                    { 0x5, "NeTV2 RawUDP"           },
                    { 0x6, "Unsupported"            },
                    { 0x7, "Unsupported"            },
                    { 0x8, "FT2232H #1"             },
                    { 0x9, "Enigma X1"              },
                    { 0xA, "Enigma X1 (FutureUse)"  },
                    { 0xB, "ScreamerM2 x4"          },
                    { 0xC, "PCIeSquirrel"           },
                    { 0xD, "Device #13N"            },
                    { 0xE, "Device #14T"            },
                    { 0xF, "Device #15N"            },
                    { 0x10, "Device #16T"           }
        };

        // if the board is in the lookup table, set the name
        if (fpgaNameLookup.count(globals::qwID))
        {
            globals::fpgaName = fpgaNameLookup[globals::qwID];
        }
        else
        {
            // board is not in the lookup table, set name to "Unsupported"
            globals::fpgaName = "Unsupported";
        }

        // print success message
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
        printf("[OK] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("  %s, V%llu.%llu\n\n", globals::fpgaName.c_str(), globals::qwVersionMajor, globals::qwVersionMinor);
    }
    else
    {
        // print fail message
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
        printf("[ERROR] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("Failed to check FPGA board!\n          Will attempt to proceed anyway..\n\n");
    }
}

bool GET_Process()
{
    // variable to store function return value (success or fail)
    bool bResult;

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
    printf("[INFO] ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
    printf("Attempting to grab the process id of 'r5apex.exe' on target system..\n");

    // try to get the process id of the 'r5apex.exe' process
    bResult = VMMDLL_PidGetFromName(globals::hVMM, (LPSTR)"r5apex.exe", &globals::dwPID);
    if (bResult)
    {
        // process id was successfully retrieved
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
        printf("[OK] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("  Got it!\n       Process ID: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("%lu\n\n", globals::dwPID);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
    }
    else
    {
        // process id could not be retrieved
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
        printf("[ERROR] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("Could not find any active process with that name!\n        Please make sure the game is running..\n\n");

        Pause();

        return false;
    }

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
    printf("[INFO] ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
    printf("Grabbing the base address..\n");

    // pointer to a module entry structure for the 'r5apex.exe' process
    PVMMDLL_MAP_MODULEENTRY pModuleEntryExplorer;

    // try to get the base address of the 'r5apex.exe' process
    bResult = VMMDLL_Map_GetModuleFromNameU(globals::hVMM, globals::dwPID, (LPSTR)"r5apex.exe", &pModuleEntryExplorer);
    if (bResult)
    {
        globals::qProcessBase = pModuleEntryExplorer->vaBase;

        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
        printf("[OK] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("  Got it!\n       Base Address: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("0x%I64X\n\n", globals::qProcessBase);
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
    }
    else
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
        printf("[ERROR] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("Could not grab the base address!\n          Try restarting your target computer if it happens again.\n\n");

        // "pause" the program
        Pause();

        return false;
    }

    return true;
}

bool GET_Offsets()
{
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
    printf("[INFO] ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
    printf("Grabbing offsets from file..\n");

    // open the "offsets.txt" file
    FileParser txtFile("offsets.txt");

    // check if all required keys are present in the file
    if (txtFile.KeyExists("EntityList")
        && txtFile.KeyExists("LocalPlayer")
        && txtFile.KeyExists("LevelName")
        && txtFile.KeyExists("m_highlightEnable")
        && txtFile.KeyExists("m_highlightOcclusion")
        && txtFile.KeyExists("m_highlightColor")
        && txtFile.KeyExists("m_iSignifierName")
        && txtFile.KeyExists("m_iTeamNum")
        && txtFile.KeyExists("m_iHealth")
        && txtFile.KeyExists("m_shieldHealth")) {

        // get the value of the "EntityList" key and convert it to a hexadecimal integer
        globals::sEntityList = txtFile.GetValueOfKey<std::string>("EntityList");
        globals::dwEntityList = std::stoi(globals::sEntityList, nullptr, 16);

        // print the value of dwEntityList in hexadecimal form
        printf("       -> EntityList: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("        0x%X\n", globals::dwEntityList);

        // get the value of the "LocalPlayer" key and convert it to a hexadecimal integer
        globals::sLocalPlayer = txtFile.GetValueOfKey<std::string>("LocalPlayer");
        globals::dwLocalPlayer = std::stoi(globals::sLocalPlayer, nullptr, 16);

        // print the value of dwLocalPlayer in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> LocalPlayer: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("       0x%X\n", globals::dwLocalPlayer);

        // get the value of the "LevelName" key and convert it to a hexadecimal integer
        globals::sLevelName = txtFile.GetValueOfKey<std::string>("LevelName");
        globals::dwLevelName = std::stoi(globals::sLevelName, nullptr, 16);

        // print the value of dwLevelName in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> LevelName: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("         0x%X\n", globals::dwLevelName);

        // get the value of the "m_highlightEnable" key and convert it to a hexadecimal integer
        globals::sHighlightEnable = txtFile.GetValueOfKey<std::string>("m_highlightEnable");
        globals::dwHighlightEnable = std::stoi(globals::sHighlightEnable, nullptr, 16);

        // print the value of dwHighlightEnable in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> HighlightEnable: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("   0x%X\n", globals::dwHighlightEnable);

        // get the value of the "m_highlightOcclusion" key and convert it to a hexadecimal integer
        globals::sHighlightOcclusion = txtFile.GetValueOfKey<std::string>("m_highlightOcclusion");
        globals::dwHighlightOcclusion = std::stoi(globals::sHighlightOcclusion, nullptr, 16);

        // print the value of dwHighlightOcclusion in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> HighlightOcclusion: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("0x%X\n", globals::dwHighlightOcclusion);

        // get the value of the "m_highlightColor" key and convert it to a hexadecimal integer
        globals::sHighlightColor = txtFile.GetValueOfKey<std::string>("m_highlightColor");
        globals::dwHighlightColor = std::stoi(globals::sHighlightColor, nullptr, 16);

        // print the value of dwHighlightColor in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> HighlightColor: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("    0x%X\n", globals::dwHighlightColor);

        // get the value of the "m_iSignifierName" key and convert it to a hexadecimal integer
        globals::sSignifierName = txtFile.GetValueOfKey<std::string>("m_iSignifierName");
        globals::dwSignifierName = std::stoi(globals::sSignifierName, nullptr, 16);

        // print the value of dwSignifierName in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> SignifierName: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("     0x%X\n", globals::dwSignifierName);

        // get the value of the "m_iTeamNum" key and convert it to a hexadecimal integer
        globals::sTeamNum = txtFile.GetValueOfKey<std::string>("m_iTeamNum");
        globals::dwTeamNum = std::stoi(globals::sTeamNum, nullptr, 16);

        // print the value of dwTeamNum in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> TeamNum: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("           0x%X\n", globals::dwTeamNum);

        // get the value of the "m_iHealth" key and convert it to a hexadecimal integer
        globals::sHealth = txtFile.GetValueOfKey<std::string>("m_iHealth");
        globals::dwHealth = std::stoi(globals::sHealth, nullptr, 16);

        // print the value of dwHealth in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> Health: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("            0x%X\n", globals::dwHealth);

        // get the value of the "m_shieldHealth" key and convert it to a hexadecimal integer
        globals::sShieldHealth = txtFile.GetValueOfKey<std::string>("m_shieldHealth");
        globals::dwShieldHealth = std::stoi(globals::sShieldHealth, nullptr, 16);

        // print the value of dwShieldHealth in hexadecimal form
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE); // set text color back to normal
        printf("       -> ShieldHealth: ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
        printf("      0x%X\n\n", globals::dwShieldHealth);
    }
    else
    {
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED); // set text color to intense red
        printf("[ERROR] ");
        SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);  // set text color back to normal
        printf("One or more keys are missing from offsets file!\n          Your offsets.txt is missing a key, example of a key: 'm_highlightEnable='\n\n");

        // "pause" the program
        Pause();

        // return false to indicate that the offsets could not be retrieved
        return false;
    }

    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_GREEN); // set text color to intense green
    printf("[OK] ");
    SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);  // set text color back to normal
    printf("  Offsets grabbed!\n\n");

    // return true to indicate that the offsets were successfully retrieved
    return true;
}

void GET_InGame()
{
    // character array to store the map name
    char levelName[64];

    // string to store the map name
    std::string sLevelName = "";

    // loop until the cheat is no longer running
    while (globals::CHEAT_RUNNING)
    {
        // read the map name from memory
        if (VMMDLL_MemRead(globals::hVMM, globals::dwPID, globals::qProcessBase + globals::dwLevelName, (PBYTE)&levelName, sizeof(levelName)))
        {
            // convert the map name to a string
            sLevelName = levelName;

            // check if we are in the lobby (indicates we are not in-game)
            if (sLevelName == "mp_lobby" || sLevelName == "")
            {
                globals::IN_GAME = false;
            }
            else
            {
                // not in the lobby, so we must be in-game?
                globals::IN_GAME = true;
            }
        }

        // sleep for 5 seconds before checking again
        std::this_thread::sleep_for(std::chrono::seconds(5));
    }
}

void UPDATE_PlayerList()
{
    // pointers to entity and entity name in memory
    uintptr_t   pEntity         = NULL;
    uintptr_t   pEntityName     = NULL;

    // character array to store the entity name
    char        cEntityName[32];

    // string to store the entity name
    std::string entityName      = "";

    // variable to store the entity's health
    int         entityHealth    = NULL;

    // variable to store the number of bytes read by VMMDLL_MemReadEx()
    DWORD       dwReadCache     = NULL;

    // loop until the cheat is no longer running
    while (globals::CHEAT_RUNNING)
    {
        // only update the list if we are in-game
        if (globals::IN_GAME)
        {
            // clear the entity vector
            globals::pEntityVector.clear();

            // loop through all possible entity indices
            for (int i = 1; i <= 60; i++)
            {
                // read the entity pointer from memory
                VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, globals::qProcessBase + globals::dwEntityList + (32 * i), (PBYTE)&pEntity, sizeof(pEntity), &dwReadCache, VMMDLL_FLAG_NOCACHE);
                if (pEntity)
                {
                    // read the entity name pointer from memory
                    VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntity + globals::dwSignifierName, (PBYTE)&pEntityName, sizeof(pEntityName), &dwReadCache, VMMDLL_FLAG_NOCACHE);
                    if (pEntityName)
                    {
                        // read the entity name from memory
                        VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntityName, (PBYTE)&cEntityName, sizeof(cEntityName), &dwReadCache, VMMDLL_FLAG_NOCACHE);
                        entityName = cEntityName;

                        // check if the entity is a player
                        if (entityName == "player")
                        {
                            // read the entity's health from memory
                            VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntity + globals::dwHealth, (PBYTE)&entityHealth, sizeof(entityHealth), &dwReadCache, VMMDLL_FLAG_NOCACHE);
                            if (entityHealth > 0)
                            {
                                // entity is a player and alive, so add it to the vector
                                globals::pEntityVector.push_back(pEntity);
                            }
                        }
                    }
                }
            }

            // print message indicating the vector was updated
            //printf("pEntityVector updated with %zd entities!\n", globals::pEntityVector.size());
        }

        // sleep for 10 seconds before checking again
        std::this_thread::sleep_for(std::chrono::seconds(10));
    }
}

void UPDATE_LocalPlayer()
{
    // variable to store the number of bytes read by VMMDLL_MemReadEx()
    DWORD dwReadCache = NULL;

    // loop until the cheat is no longer running
    while (globals::CHEAT_RUNNING)
    {
        // only update the local player if we are in-game
        if (globals::IN_GAME)
        {
            // read the local player pointer from memory
            VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, globals::qProcessBase + globals::dwLocalPlayer, (PBYTE)&globals::pLocalPlayer, sizeof(globals::pLocalPlayer), &dwReadCache, VMMDLL_FLAG_NOCACHE);
            if (globals::pLocalPlayer)
            {
                // print message indicating the local player was updated
                //printf("pLocalPlayer updated! [0x%I64X]\n", globals::pLocalPlayer);

                // read the local player's team number from memory
                VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, globals::pLocalPlayer + globals::dwTeamNum, (PBYTE)&globals::localTeam, sizeof(globals::localTeam), &dwReadCache, VMMDLL_FLAG_NOCACHE);
                //printf("local team: %d\n", globals::localTeam);
            }
        }

        // sleep for 25 seconds before checking again
        std::this_thread::sleep_for(std::chrono::seconds(25));
    }
}

void CHEAT_Handler()
{
    // variables to store information about each entity
    int         entityTeam          = NULL;
    int         entityShield        = NULL;
    int         entityHighlighted   = NULL;
    int         entityOccluded      = NULL;

    // constants for enabling highlight and occlusion
    const int   highlightEnable     = 1;
    const int   occlusionEnable     = 2;

    // constants for disabling highlight and occlusion
    const int   highlightDisable    = 2;
    const int   occlusionDisable    = 5;

    // variable to store the cache when reading from memory
    DWORD       dwReadCache         = NULL;

    // variables to store the highlight color of each entity
    float       red                 = 0.f;
    float       green               = 0.f;
    float       blue                = 0.f;

    // flag to ensure info prompt is only performed once
    bool        doOnce              = false;

    // loop until the cheat is no longer running
    while (globals::CHEAT_RUNNING)
    {
        // only run if we are in-game
        if (globals::IN_GAME)
        {
            // iterate through each entity in the vector
            for (const auto& pEntity : globals::pEntityVector)
            {
                // read the entity's team from memory
                VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntity + globals::dwTeamNum, (PBYTE)&entityTeam, sizeof(entityTeam), &dwReadCache, VMMDLL_FLAG_NOCACHE);

                // filter entities that have a valid team (1 to 30)
                if (entityTeam >= 1 && entityTeam <= 30)
                {
                    // read whether the entity is highlighted and occluded from memory
                    VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightEnable, (PBYTE)&entityHighlighted, sizeof(entityHighlighted), &dwReadCache, VMMDLL_FLAG_NOCACHE);
                    VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightOcclusion, (PBYTE)&entityOccluded, sizeof(entityOccluded), &dwReadCache, VMMDLL_FLAG_NOCACHE);

                    // filter entities that are not on the local player's team
                    if (entityTeam != globals::localTeam)
                    {
                        // if the entity is not already highlighted and occluded, apply it
                        if (entityHighlighted != 1 && entityOccluded != 2)
                        {
                            VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightEnable, (PBYTE)&highlightEnable, sizeof(highlightEnable));
                            VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightOcclusion, (PBYTE)&occlusionEnable, sizeof(occlusionEnable));

                            // print message indicating that the effect was applied
                            //printf("Applied glow to: 0x%I64X\n", pEntity);
                            //printf("team id: %d\n", entityTeam);
                        }

                        // read the entity's shield health from memory
                        VMMDLL_MemReadEx(globals::hVMM, globals::dwPID, pEntity + globals::dwShieldHealth, (PBYTE)&entityShield, sizeof(entityShield), &dwReadCache, VMMDLL_FLAG_NOCACHE);

                        // set the highlight color based on the entity's shield health
                        if (entityShield > 100)
                        {
                            red = 5.f;
                            green = 0.f;
                            blue = 0.f;
                        }
                        else if (entityShield > 75)
                        {
                            red = 5.f;
                            green = 0.f;
                            blue = 5.f;
                        }
                        else if (entityShield > 50)
                        {
                            red = 0.f;
                            green = 0.f;
                            blue = 5.f;
                        }
                        else if (entityShield >= 0)
                        {
                            red = 5.f;
                            green = 5.f;
                            blue = 5.f;
                        }

                        // write the highlight color to memory
                        VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightColor, (PBYTE)&red, sizeof(red));
                        VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightColor + 4, (PBYTE)&green, sizeof(green));
                        VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightColor + 8, (PBYTE)&blue, sizeof(blue));
                    }
                    if (globals::localTeam == entityTeam)
                    {
                        if (entityHighlighted == 1 || entityOccluded == 2)
                        {
                            VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightEnable, (PBYTE)&highlightDisable, sizeof(highlightDisable));
                            VMMDLL_MemWrite(globals::hVMM, globals::dwPID, pEntity + globals::dwHighlightOcclusion, (PBYTE)&occlusionDisable, sizeof(occlusionDisable));

                            //printf("Removed glow from: 0x%I64X\n", pEntity);
                        }
                    }
                }
                // sleep for 5ms before repeating the for() loop
                // should probably be removed lol
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }
        }

        if (!doOnce)
        {
            // set our flag to indicate that we have prompted the information so that we won't do it twice
            doOnce = true;

            // print message indicating that the cheat is running and how to exit it properly
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_BLUE); // set text color to intense purple
            printf("[INFO] ");
            SetConsoleTextAttribute(GetStdHandle(STD_OUTPUT_HANDLE), FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE);     // set text color back to normal
            printf("Cheat is now running..\n       Press the 'END' key to exit.\n\n");
        }

        // sleep for 1ms before repeating the while() loop
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }
}