#include <stdio.h>
#include <windows.h>
#include <psapi.h>
#include <tchar.h>
#include <stdint.h>

int checkSkinRow(BYTE* buffer);
HANDLE findProcessByName(const TCHAR* processName);

int main (){
    HANDLE hProcess;
    hProcess = findProcessByName( _T("Minecraft.Windows.exe"));

    if (hProcess) {
        printf("Minecraft: Bedrock process found automatically.\n");
    }else{ 
        fprintf(stderr, "Minecraft was not found to be running\n");
        return 1;
    }
    
    printf("Searching for 64x64 skins in program memory... (may take a while)\n");
    system("mkdir skins");
    // Iterate through all memory regions of the target process
    MEMORY_BASIC_INFORMATION memoryInfo;
    LPVOID baseAddress = 0;
    SIZE_T skincount = 0;

    while (VirtualQueryEx(hProcess, baseAddress, &memoryInfo, sizeof(memoryInfo)) == sizeof(memoryInfo)) {
        // Allocate memory for the region
        SIZE_T bytesRead;
        BYTE* buffer = (BYTE*)malloc(memoryInfo.RegionSize);
        
        // Read the memory from the target process
        if (ReadProcessMemory(hProcess, memoryInfo.BaseAddress, buffer, memoryInfo.RegionSize, &bytesRead)) {
            SIZE_T count0=0;

            // Process the data in the buffer (e.g., print it)
            for (SIZE_T i = 0; i + (64*64*4-32)< bytesRead; i++) { //make sure we have enough memory left for a skin, otheriwse don't even bother checking
                if (buffer[i]==0){
                    count0++;
                } else {                 //since skins always start with 32 zeroes followed by a non-zero value, this saves a lot of checks
                    if (count0>=32){    
                        for (SIZE_T j=0; j<7; j++){
                            if (!checkSkinRow(buffer+i+(j*64*4))){     //check 7 times, one for each row that follows that pattern
                                count0=0;
                                break;
                            }
                        }

                        if (count0){
                            printf("SKIN %03d found\n", skincount);
                            char fname[32];
                            sprintf(fname,"skins/%03d.png",skincount);
                            skincount++;

                            //This next bit improperly creates an uncompressed PNG file in a terrible hard-coded way

                            FILE* png = fopen(fname,"wb");
                            if (!png)
                                fprintf(stderr, "ERROR couldn't write file \"%s\"\n",fname);
                            else {
                                 const BYTE PNG_HEADER[] = { 0x89, 'P', 'N', 'G', 0x0d, 0x0a, 0x1a, 0x0a,  //SIGNATURE

                                                            0, 0, 0, 13,         //length of IHDR
                                                            'I', 'H', 'D', 'R',  //start of IMAGE HEADER
                                                            0, 0, 0, 64,         //width (=64px) big endian for some reason
                                                            0, 0, 0, 64,         //height (=64px) 
                                                            8,                   //bits per channel 
                                                            6,                   //colour type (6 = RGBA)
                                                            0,                   //compression
                                                            0,                   //Filter
                                                            0,                   //interlace
                                                            0xAA,0x69,0x71,0xde, //IHDR checksum 

                                                            0, 0, 0x40, 0x47,    //length of IDATA (w*h*4 [bytes/channel] + 7 [deflate headers] + h [1 filter per row] ) 
                                                            'I','D','A','T',     //start of IMAGE DATA HEADER
                                                            
                                                            0x78, 0x01,          //deflate: CM=8, CINFO=7;   FLG=1
                                                            0x01,                //deflate: final block with no compression (just bypassing the compression alltogether)
                                                            0x40, 0x40,          //deflate: LEN (w*h + h [1 filter per row]) Little endian so it's not even consistent
                                                            ~0x40,~0x40          //deflate: NLEN (btw what is this even about? why randomly use another two bytes for nothing?
                                                            };  
                                fwrite(PNG_HEADER,sizeof(PNG_HEADER), 1, png);

                                BYTE* start = buffer+i-32;
                                for (SIZE_T i = 0; i<64; i++){        
                                    fputc(0,png);                      //set the row filter to 0 for this row
                                    fwrite(start+(i*64*4),4, 64, png); //write raw pixel row data
                                }

                                const BYTE IEND[] = {0,0,0,0, //"Checksum" for IDATA. I could easily calculate it for real but apparently no one actually checks the checsksum so I'll just leave this zeros, it's useless anyways
                                                    0,0,0,0,'I','E','N','D', //IEND
                                                    0xAE,0x42,0x60,0x82};    //IEND checksum

                                fwrite(IEND,sizeof(IEND), 1, png);
                                fclose(png);
                            }
                        }
                    }
                    count0=0;
                }
            }
        }

        // Move to the next memory region
        baseAddress = (LPVOID)((BYTE*)memoryInfo.BaseAddress + memoryInfo.RegionSize);

        // Free the buffer
        free(buffer);
    }

    // Close the handle to the target process
    CloseHandle(hProcess);

    printf("DONE!\n %lu skins found\n", skincount);
    return 0;
}

HANDLE findProcessByName(const TCHAR* processName){
    HANDLE hProcess;
	DWORD aProcesses[1024], cbNeeded, cProcesses;
    if (!EnumProcesses(aProcesses, sizeof(aProcesses), &cbNeeded)) {
        fprintf(stderr, "EnumProcesses failed, Error: %lu\n", GetLastError());
        exit(1);
    }
	
    // Calculate how many process identifiers were returned.
    cProcesses = cbNeeded / sizeof(DWORD);

    for (DWORD i = 0; i < cProcesses; i++) {
        if (aProcesses[i] != 0) {
            hProcess = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, aProcesses[i]);
            if (hProcess != NULL) {
                TCHAR szProcessName[MAX_PATH] = TEXT("<unknown>");
                HMODULE hMod;
                DWORD cbNeeded;
                if (EnumProcessModules(hProcess, &hMod, sizeof(hMod), &cbNeeded)) {
                    GetModuleBaseName(hProcess, hMod, szProcessName, sizeof(szProcessName) / sizeof(TCHAR));
                }

                //look for Minecraft bedrock process
                if (_tcsicmp(szProcessName, processName) == 0) {
                    return hProcess;
                }

                CloseHandle(hProcess);
            }
        }
    }
    return NULL;
}

int checkSkinRow(BYTE* buffer) {
    SIZE_T i;
    for (i = 3; i < 64; i += 4) {
        if (buffer[i] != 0xff) {
            return 0;  // Check alpha channel is opaque in base layer of the head of the skin
        }
    }
    for (i = 64; i < 128; i++) {
        if (buffer[i] != 0x00) {
            return 0;  // Check all pixels and alpha are 0 in the area between base layer and overlay
        }
    }

    for (i = 192; i < 256; i++) {
        if (buffer[i] != 0x00) {
            return 0;  // Check all pixels and alpha are 0 in between overlay and next row base layer
        }
    }
    return 1;
}