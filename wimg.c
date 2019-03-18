#include <windows.h>
#include <TCHAR.H>
#include <STRSAFE.H>

/************************************ *
 * This prigram MUST NOT use Unicode. *
 * ************************************/

typedef enum tagBASE64_TYPE
{
	BASE64_TYPE_STANDARD,
	BASE64_TYPE_MIME,
	BASE64_TYPE_URL
} BASE64_TYPE;

char *base64Decode(const char *base64, size_t *retSize, const BASE64_TYPE type);

//// 定数 ////

size_t PrintfBufferSize = 65536; // *Printf() 系関数のバッファサイズ

//// グローバル変数 ////

HANDLE hStdOut, hStdIn; // 標準出力, 入力
HANDLE hHeap;						// ヒープ領域
DWORD Error = 0;

//// 邪悪なマクロ ////

// printf() の互換
#define ConsolePrintf(...) (HandlePrintf(hStdOut, __VA_ARGS__))

//// 関数プロトタイプ ////

DWORD HandlePrintf(HANDLE hOut, LPCTSTR pszFmt, ...); // ハンドル出力の printf()

int main()
{
	LPSTR Content, ImgName;
	DWORD ContentLen, ReadBytes;
	LPVOID ImgData;
	size_t ImgLen;
	FILETIME ftNow;
	HANDLE hFile;

	// 各種初期化 絶対に失敗してはいけないのでエラーを返すことはできない
	if ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE)
	{
		ExitProcess((UINT)GetLastError());
	}
	if ((hStdIn = GetStdHandle(STD_INPUT_HANDLE)) == INVALID_HANDLE_VALUE)
	{
		ExitProcess((UINT)GetLastError());
	}
	if ((hHeap = GetProcessHeap()) == NULL)
	{
		ExitProcess((UINT)GetLastError());
	}

	// ContentLengthを格納するメモリを確保
	Content = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (Content == NULL)
	{
		Error = GetLastError() + 0x10000;
		goto MemError0;
	}
	if (GetEnvironmentVariable("CONTENT_LENGTH", Content, MAX_PATH) == 0)
	{
		Error = GetLastError() + 0x10000;
		goto GetLenError;
	}
	ContentLen = (DWORD)strtoul(Content, NULL, 10);
	HeapFree(hHeap, 0, Content);

	// Content受け取りバッファ
	Content = VirtualAlloc(NULL, (SIZE_T)ContentLen + 1l, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
	if (Content == NULL)
	{
		Error = GetLastError() + 0x20000;
		goto MemError1;
	}

	GetSystemTimeAsFileTime(&ftNow);
	ImgName = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (ImgName == NULL)
	{
		Error = GetLastError() + 0x30000;
		goto ImgNameError;
	}

	// 読み出し
	ReadFile(hStdIn, Content, ContentLen, &ReadBytes, NULL);
	Content[ContentLen] = 0;

	ImgData = base64Decode(Content + 2, &ImgLen, BASE64_TYPE_URL);
	if (ImgData == NULL)
	{
		Error = GetLastError() + 0x40000;
		goto DecError;
	}

	if (memcmp(ImgData, "\xff\xd8", 2) == 0) // jpg
	{
		StringCbPrintf(ImgName, MAX_PATH, "\\\\HomeServer\\Storage\\img\\mn%08X%08X.jpg", ftNow.dwHighDateTime, ftNow.dwLowDateTime);
	}
	else
	{
		Error = 0x4ffff;
		goto ImgError;
	}

	hFile = CreateFile(ImgName, GENERIC_WRITE, FILE_SHARE_READ, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Error = GetLastError() + 0x40000;
		goto ImgError;
	}

	WriteFile(hFile, ImgData, (DWORD)ImgLen, &ReadBytes, NULL);

	ConsolePrintf("Location: ./mimg.cgi?r=mn%08X%08X\n\n", ftNow.dwHighDateTime, ftNow.dwLowDateTime);

	HeapFree(hHeap, 0, ImgData);
	HeapFree(hHeap, 0, ImgName);
	VirtualFree(Content, 0, MEM_RELEASE);

	// エラー処理
	if (0)
	{
	ImgError:
		HeapFree(hHeap, 0, ImgData);
	DecError:
		HeapFree(hHeap, 0, ImgName);
	ImgNameError:
		VirtualFree(Content, 0, MEM_RELEASE);
	MemError1:
		if (0)
		{
		GetLenError:
			HeapFree(hHeap, 0, Content);
		}
	MemError0:
		ConsolePrintf("status: 500\ncontent-type: text/plain; charset=utf-8\n\n");
		ConsolePrintf("エラーが起こりました．以下の値を運営に通知してください．\n");
		ConsolePrintf("\t\tWIMG_ERROR_%05lX\n", Error);
	}
	return (int)Error;
}

DWORD HandlePrintf(HANDLE hOut, LPCTSTR pszFmt, ...)
{
	va_list ap;
	LPTSTR Buffer = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, PrintfBufferSize);
	size_t BytesOfStr;
	DWORD BytesWritten;
	va_start(ap, pszFmt);
	if (Buffer == NULL)
	{
		return -1;
	}
	StringCbVPrintf(Buffer, PrintfBufferSize, pszFmt, ap);
	va_end(ap);
	StringCbLength(Buffer, PrintfBufferSize, &BytesOfStr);
	WriteFile(hOut, Buffer, (DWORD)BytesOfStr, &BytesWritten, NULL);
	HeapFree(hHeap, 0, Buffer);
	return BytesWritten;
}

static const char BASE64_TABLE[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};
static const char BASE64_TABLE_URL[] = {"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};
static const int BASE64_TABLE_LENGTH = {sizeof(BASE64_TABLE) / sizeof(BASE64_TABLE[0]) - 1};

typedef struct tagBASE64_SPEC
{
	BASE64_TYPE type;
	const char *table;
	char pad;
	int maxLineLength;
	char *lineSep;
	int lineSepLength;
} BASE64_SPEC;
static const BASE64_SPEC BASE64_SPECS[] = {
		{BASE64_TYPE_STANDARD, BASE64_TABLE, '=', 0, NULL, 0},
		{BASE64_TYPE_MIME, BASE64_TABLE, '=', 76, "\r\n", 2},
		{BASE64_TYPE_URL, BASE64_TABLE_URL, 0, 0, NULL, 0}};
static const size_t BASE64_SPECS_LENGTH = {
		sizeof(BASE64_SPECS) / sizeof(BASE64_SPECS[0])};

char *base64Decode(const char *base64, size_t *retSize, const BASE64_TYPE type)
{
	BASE64_SPEC spec;
	char table[0x80];
	size_t length;
	char *data;
	char *cursor;
	int i;
	int j;

	if (base64 == NULL)
	{
		return NULL;
	}

	spec = BASE64_SPECS[0];
	for (i = 0; i < (int)BASE64_SPECS_LENGTH; i++)
	{
		if (BASE64_SPECS[i].type == type)
		{
			spec = BASE64_SPECS[i];
			break;
		}
	}
	length = strlen(base64);
	data = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, length * 3 / 4 + 2 + 1);
	if (data == NULL)
	{
		return NULL;
	}

	memset(table, 0x80, sizeof(table));
	for (i = 0; i < BASE64_TABLE_LENGTH; i++)
	{
		table[spec.table[i] & 0x7f] = i;
	}

	cursor = data;
	for (i = 0, j = 0; i < (int)length; i++, j = i % 4)
	{
		char ch;

		if (base64[i] == spec.pad)
		{
			break;
		}

		ch = table[base64[i] & 0x7f];
		if (ch & 0x80)
		{
			continue;
		}
		if (j == 0)
		{
			*cursor = ch << 2 & 0xfc;
		}
		else if (j == 1)
		{
			*(cursor++) |= ch >> 4 & 0x03;
			*cursor = ch << 4 & 0xf0;
		}
		else if (j == 2)
		{
			*(cursor++) |= ch >> 2 & 0x0f;
			*cursor = ch << 6 & 0xc0;
		}
		else
		{
			*(cursor++) |= ch & 0x3f;
		}
	}
	*cursor = 0;
	*retSize = cursor - data;

	return data;
}
