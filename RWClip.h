#include <Windows.h>
#include <io.h>
#include <iostream>
#include <assert.h>
#include <iostream>
#include <fstream>
#include <fcntl.h>
#include <stdio.h>
#include <time.h>

#define _O_U16TEXT	0x20000

static BOOL OpenClipboard_ButTryABitHarder(HWND hWnd);
static INT GetPixelDataOffsetForPackedDIB(const BITMAPINFOHEADER *BitmapInfoHeader);
void WriteFile(char* source, size_t size);

int ReadIMGClip() {
   if (!OpenClipboard_ButTryABitHarder(NULL))
        return 1;
    HGLOBAL ClipboardDataHandle = (HGLOBAL)GetClipboardData(CF_DIB);
    if (!ClipboardDataHandle)   {
        CloseClipboard();
        return 0;
    }

    BITMAPINFOHEADER *BitmapInfoHeader = (BITMAPINFOHEADER *)GlobalLock(ClipboardDataHandle);
    assert(BitmapInfoHeader); // This can theoretically fail if mapping the HGLOBAL into local address space fails. Very pathological, just act as if it wasn't a bitmap in the clipboard.

    SIZE_T ClipboardDataSize = GlobalSize(ClipboardDataHandle);
    assert(ClipboardDataSize >= sizeof(BITMAPINFOHEADER)); // Malformed data. While older DIB formats exist (e.g. BITMAPCOREHEADER), they are not valid data for CF_DIB; it mandates a BITMAPINFO struct. If this fails, just act as if it wasn't a bitmap in the clipboard.

    INT PixelDataOffset = GetPixelDataOffsetForPackedDIB(BitmapInfoHeader);
    // The BMP file layout:
    // @offset 0:                              BITMAPFILEHEADER
    // @offset 14 (sizeof(BITMAPFILEHEADER)):  BITMAPINFOHEADER
    // @offset 14 + BitmapInfoHeader->biSize:  Optional bit masks and color table
    // @offset 14 + DIBPixelDataOffset:        pixel bits
    // @offset 14 + ClipboardDataSize:         EOF
    size_t TotalBitmapFileSize = sizeof(BITMAPFILEHEADER) + ClipboardDataSize;
    wprintf(L"BITMAPINFOHEADER size:          %u\r\n", BitmapInfoHeader->biSize);
    wprintf(L"Format:                         %hubpp, Compression %u\r\n", BitmapInfoHeader->biBitCount, BitmapInfoHeader->biCompression);
    wprintf(L"Pixel data offset within DIB:   %u\r\n", PixelDataOffset);
    wprintf(L"Total DIB size:                 %zu\r\n", ClipboardDataSize);
    wprintf(L"Total bitmap file size:         %zu\r\n", TotalBitmapFileSize);

    BITMAPFILEHEADER BitmapFileHeader = {};
    BitmapFileHeader.bfType = 0x4D42;
    BitmapFileHeader.bfSize = (DWORD)TotalBitmapFileSize; // Will fail if bitmap size is nonstandard >4GB
    BitmapFileHeader.bfOffBits = sizeof(BITMAPFILEHEADER) + PixelDataOffset;


    time_t t = time(0);   // get time now
    struct tm * now = localtime( & t );

    char buffer [20];
    strftime (buffer,20,"%Y%m%d%H%M",now);
    char* pre = "IMG";
    char* full_text= (char*)malloc(strlen(pre)+strlen(buffer)+1); 
    strcpy(full_text, pre);
    strcat(full_text, buffer);
    char* post=".bmp";
    char* full= (char*)malloc(strlen(full_text)+strlen(post)+1); 
    strcpy(full, full_text);
    strcat(full, post);

    const size_t cSize = strlen(full)+1;
    wchar_t* wc = new wchar_t[cSize];
    mbstowcs (wc, full, cSize);

    HANDLE FileHandle = CreateFileW(wc, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD dummy = 0;
        BOOL Success = true;

        char* buf = new char[TotalBitmapFileSize];
        memcpy(reinterpret_cast<void*>(buf), &BitmapFileHeader, sizeof(BITMAPFILEHEADER));
        memcpy(reinterpret_cast<void*>(buf+sizeof(BITMAPFILEHEADER)), BitmapInfoHeader, ClipboardDataSize);

        Success &= WriteFile(FileHandle, reinterpret_cast<LPCVOID>(buf), (DWORD)TotalBitmapFileSize, &dummy, NULL);
        Success &= CloseHandle(FileHandle);

        if (Success)
            std::cout<<"File saved.\n";
        else
            std::cout<<"Error\n";
    }

    GlobalUnlock(ClipboardDataHandle);
    CloseClipboard();
    return 0;
}

static INT GetPixelDataOffsetForPackedDIB(const BITMAPINFOHEADER *BitmapInfoHeader) {
    INT OffsetExtra = 0;
    if (BitmapInfoHeader->biSize == sizeof(BITMAPINFOHEADER) /* 40 */)  {
        // This is the common BITMAPINFOHEADER type. In this case, there may be bit masks following the BITMAPINFOHEADER
        // and before the actual pixel bits (does not apply if bitmap has <= 8 bpp)
        if (BitmapInfoHeader->biBitCount > 8)   {
            if (BitmapInfoHeader->biCompression == BI_BITFIELDS)    {
                OffsetExtra += 3 * sizeof(RGBQUAD);
            }
            else if (BitmapInfoHeader->biCompression == 6 /* BI_ALPHABITFIELDS */)  {
                // Not widely supported, but valid.
                OffsetExtra += 4 * sizeof(RGBQUAD);
            }
        }
    }

    if (BitmapInfoHeader->biClrUsed > 0)    {
        // We have no choice but to trust this value.
        OffsetExtra += BitmapInfoHeader->biClrUsed * sizeof(RGBQUAD);
    }
    else    {
        // In this case, the color table contains the maximum number for the current bit count (0 if > 8bpp)
        if (BitmapInfoHeader->biBitCount <= 8)  {
            // 1bpp: 2
            // 4bpp: 16
            // 8bpp: 256
            OffsetExtra += sizeof(RGBQUAD) << BitmapInfoHeader->biBitCount;
        }
    }
    return BitmapInfoHeader->biSize + OffsetExtra;
}

void addString(char* a, char* b) {
    char* full_text= (char*)malloc(strlen(a)+strlen(b)+1);
    strcpy(full_text, a);
    strcat(full_text, b);
}

void WriteFile(char* source, size_t size) {
    time_t t = time(0);   // get time now
     struct tm * now = localtime( & t );

    char buffer [20];
    strftime (buffer,20,"%Y%m%d%H%M",now);
    char* pre = "TXT";
    char* full_text= (char*)malloc(strlen(pre)+strlen(buffer)+1); 
    strcpy(full_text, pre);
    strcat(full_text, buffer);
    char* post=".txt";
    char* full= (char*)malloc(strlen(full_text)+strlen(post)+1); 
    strcpy(full, full_text);
    strcat(full, post);

    std::ofstream file (full, std::ios::binary);
    if(!file.is_open())
        return;
    file.write(source,size-2);
    cout<<"File Written";
}


void ReadUTFClip() {
    wchar_t* point_str;
    OpenClipboard(NULL);
    HGLOBAL hg=GetClipboardData(CF_UNICODETEXT);
    if(hg==NULL)
        return;
    _setmode(_fileno(stdout), _O_U16TEXT);
    size_t point_size = GlobalSize(hg);
    point_str =(wchar_t*)malloc(point_size);
    wcscpy(point_str, (wchar_t*)hg);
    CloseClipboard();
    WriteFile((char*)point_str, point_size);
}

static BOOL OpenClipboard_ButTryABitHarder(HWND hWnd)   {
    for (int i = 0; i < 20; ++i)    {
        // This can fail if the clipboard is currently being accessed by another application.
        if (OpenClipboard(hWnd))
            return true;
        Sleep(10);
    }
    return false;
}
