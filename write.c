#include <windows.h>
#include <TCHAR.H>
#include <STRSAFE.H>

/************************************ *
 * This prigram MUST NOT use Unicode. *
 * ************************************/

//// 定数 ////

size_t PrintfBufferSize = 65536; // *Printf() 系関数のバッファサイズ
WORD IdItemId = 0x4449;
DWORD IdItemLen = 7;

//// グローバル変数 ////

HANDLE hStdOut, hStdIn; // 標準出力, 入力
HANDLE hHeap;						// ヒープ領域
DWORD Error;

//// 邪悪なマクロ ////

// printf() の互換
#define ConsolePrintf(...) (HandlePrintf(hStdOut, __VA_ARGS__))

//// 関数プロトタイプ ////

DWORD HandlePrintf(HANDLE hOut, LPCTSTR pszFmt, ...); // ハンドル出力の printf()

int GetThreadName(char *Buffer, const char *Query); // クエリからスレッドのファイル名を抽出する。
int GetName(char *Buffer, const char *Query);				// クエリから名前を
int GetContent(char *Buffer, const char *Query);		// クエリからコンテンツを

void XorShift_Init(uint32_t);
uint32_t XorShift_Rand(void);

int main()
{
	LPSTR ThreadName, Name, Content, PName, PContent, Query, Agent, Addr, URLBuf;
	FILETIME ftTime, ftLast, SAGEft, ftLocal;
	DWORD ReadBytes, NameLen, ContentLen, AgentLen, RsvLen = 13;
	double Spd;
	HANDLE hFile;
	unsigned int i, l;
	BOOL AAFlag = FALSE, SAGEFlag = FALSE;
	int URLCount, ImgCount = 0;
	unsigned long long *ImgBuffer = NULL;
	WORD Id = 0, BaseId;
	BYTE IdStr[5];

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

	// メモリ確保
	ThreadName = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (ThreadName == NULL)
	{
		Error = GetLastError();
		goto MemError0;
	}
	Name = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024);
	if (!Name)
	{
		Error = GetLastError();
		goto MemError1;
	}
	Content = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 131072);
	if (!Content)
	{
		Error = GetLastError();
		goto MemError2;
	}
	PName = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024);
	if (!PName)
	{
		Error = GetLastError();
		goto MemError3;
	}
	Query = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 262144);
	if (Query == NULL)
	{
		Error = GetLastError();
		goto MemError4;
	}
	Agent = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 1024);
	if (Agent == NULL)
	{
		Error = GetLastError();
		goto MemError5;
	}
	URLBuf = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 65536);
	if (URLBuf == NULL)
	{
		Error = GetLastError();
		goto MemError6;
	}
	Addr = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 256);
	if (Addr == NULL)
	{
		Error = GetLastError();
		goto MemError7;
	}
	PContent = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 131072);
	if (!PContent)
	{
	QueryError:
	FileError:
	ContentError:
		Error = GetLastError();
		HeapFree(hHeap, 0, Addr);
	MemError7:
		HeapFree(hHeap, 0, URLBuf);
	MemError6:
		HeapFree(hHeap, 0, Agent);
	MemError5:
		HeapFree(hHeap, 0, Query);
	MemError4:
		HeapFree(hHeap, 0, PName);
	MemError3:
		HeapFree(hHeap, 0, Content);
	MemError2:
		HeapFree(hHeap, 0, Name);
	MemError1:
		HeapFree(hHeap, 0, ThreadName);
	MemError0:
		ConsolePrintf(TEXT("Status: 500 Internal Server Error (Write: %#04X)\n\n"), Error);
		ExitProcess((UINT)Error);
	}
	// ID 確定
	GetSystemTimeAsFileTime(&ftTime);
	FileTimeToLocalFileTime(&ftTime, &ftLocal);
	if (GetEnvironmentVariable(TEXT("REMOTE_ADDR"), Addr, 256) != 0)
	{
		PCHAR p;
		int ip[4];
		while ((p = memchr(Addr, '.', 256)) != NULL)
		{ // . は次の構文で引っかかるかも？
			*p = ' ';
		}
		sscanf(Addr, "%d %d %d %d", ip + 0, ip + 1, ip + 2, ip + 3);
		Id = (((ip[0] ^ ip[1] ^ ip[2] ^ ip[3]) << 8) | ((ip[0] + ip[1] + ip[2] + ip[3]) & 0xFF));
		BaseId = Id;
		{
			FILE *fp = fopen("c:/ban.txt", "a");
			if (fp != NULL)
			{
				fprintf(fp, "%04X is %s\n", BaseId, Addr);
				fclose(fp);
			}
		}
		XorShift_Init((DWORD)(*(PLONGLONG)&ftLocal / 864000000000ll));
		Id ^= (WORD)XorShift_Rand();
		StringCbPrintf(IdStr, 5, "%04X", Id);
	}
	// HTTP情報・クエリ解析
	if (GetEnvironmentVariable(TEXT("HTTP_USER_AGENT"), Agent, 1024) == 0)
	{
		goto QueryError;
	}
	if (GetEnvironmentVariable(TEXT("QUERY_STRING"), Query, 262144) != 0)
	{
		goto QueryOK;
	}
	if (GetEnvironmentVariable(TEXT("CONTENT_LENGTH"), Query, 262144) == 0)
	{
		goto QueryError;
	}
	ContentLen = atoi(Query);
	ReadFile(hStdIn, Query, ContentLen, &ReadBytes, NULL);
QueryOK:
	GetContent(Content, Query);
	GetName(Name, Query);
	GetThreadName(ThreadName, Query);
	if (Agent == NULL)
	{
		Agent = "";
	}
	// 名前部
	for (i = l = 0; Name[i]; (void)0)
	{
		if ((i == 0) && (strstr(&Name[i], "%21KOTEHAN%21unei%21") == &Name[i]))
		{ // 超コテハン機能
			StringCbPrintf(PName, 1024, "<span style=\"text-decoration: underline;\" class=\"sfg\">unei</span>");
			StringCbLength(PName, 1024, &l);
			Id = 0;
			StringCbCopy(IdStr, 5, "unei\0");
			i = 20;
			break;
		}
		if ((i == 0) && (strstr(&Name[i], "%21KOTEID%21NULL%21") == &Name[i]))
		{ // ID = 0
			Id = 0;
			StringCbCopy(IdStr, 5, "****\0");
			i = 19;
		}
		if ((i == 0) && (strstr(&Name[i], "%21KOTEID%21unei%21") == &Name[i]))
		{ // ID = 0
			Id = 0;
			StringCbCopy(IdStr, 5, "unei\0");
			i = 19;
		}
		if (strstr(&Name[i], "%3C") == &Name[i])
		{ // < これ。
			StringCbPrintf(PName + l, 1024 - l, "&lt;");
			StringCbLength(PName, 1024, &l);
			i += 3;
		}
		else if (strstr(&Name[i], "%3E") == &Name[i])
		{ // > これ。
			StringCbPrintf(PName + l, 1024 - l, "&gt;");
			StringCbLength(PName, 1024, &l);
			i += 3;
		}
		else if (Name[i] == '%')
		{ // URL エンコードと呼ばれる奴の展開
			int c;
			sscanf(&Name[i + 1], "%02x", &c);
			PName[l++] = c;
			i += 3;
		}
		else if (Name[i] == '+')
		{ // SPACE の処理
			PName[l++] = ' ';
			i++;
		}
		else
		{ // それ以外はそのまま転送
			PName[l++] = Name[i++];
		}
	}
	PName[l] = 0;
	// 本文
	for (i = l = 0; Content[i]; (void)0)
	{
		// フラグ処理
		if ((l == 0) && (strstr(&Content[i], "%21SAGE%21") == &Content[i]))
		{ // SAGE 処理
			SAGEFlag = TRUE;
			i += 10;
		}
		if ((l == 0) && (strstr(&Content[i], "%21AA%21") == &Content[i]))
		{ // AA 処理
			AAFlag = TRUE;
			StringCbPrintf(PContent, 131072, "<span class=\"bfg aa\">");
			StringCbLength(PContent, 131072, &l);
			i += 8;
		}

		// URL 発見時
		if (strstr(&Content[i], "http") == &Content[i])
		{
			int URLStartPos;
			StringCbCopy(URLBuf, 65536, "http");
			URLCount = 4;
			i += 4;
			StringCbPrintf(&PContent[l], 131072 - l, "<a class=\"sfg\" target=\"_blank\" href=\"");
			StringCbLength(PContent, 131072, &l);
			while (1)
			{
				if (Content[i] == '%')
				{ // URL エンコードと呼ばれる奴の展開
					int c;
					sscanf(&Content[i + 1], "%02x", &c);
					if (0x20 < c && c < 0x7f && c % 0x20 != 0x1C && c != ' ' && c!= '\"' && c!=0x60 && (c < 0x7B || c > 0x7D))
					{
						URLBuf[URLCount++] = c;
					}
					else
					{
						break;
					}
					i += 3;
				}
				else if (Content[i] == '+')
				{ // SPACE の処理
					break;
				}
				else if (Content[i] == '\0')
				{
					break;
				}
				else
				{ // そのまま転送
					URLBuf[URLCount++] = Content[i++];
				}
			}
			URLBuf[URLCount] = 0;
			StringCbPrintf(&PContent[l], 131072 - l, "%s\">%s</a> ", URLBuf, URLBuf);
			StringCbLength(PContent, 131072, &l);
		}
		// エンコード処理等
		if (strstr(&Content[i], "%0A") == &Content[i])
		{ // 改行処理 Linux/Windows 互換のため 0Ah のみ置換
			StringCbPrintf(PContent + l, 131072 - l, "<br />");
			StringCbLength(PContent, 131072, &l);
			i += 3;
		}
		else if (strstr(&Content[i], "%3E%3E") == &Content[i])
		{ // >>0 形式
			unsigned long ul;
			char *ep;
			ul = strtoul(&Content[i + 6], &ep, 10);
			if (ep != &Content[i + 6])
			{
				StringCbPrintf(PContent + l, 131072 - l, "<a href=\'javascript: (void)0;\' onclick=\"GoRes(\'RES%08lX\');\" class=\"sfg\">&gt;&gt;%ld</a> ", ul, ul);
				StringCbLength(PContent, 131072, &l);
			}
			else
			{
				goto NormalGt;
			}
			i = (int)(ep - Content);
		}
		else if (strstr(&Content[i], "sm") == &Content[i])
		{ // sm9 形式
			unsigned long ul;
			char *ep;
			ul = strtoul(&Content[i + 2], &ep, 10);
			if (ep != &Content[i + 2])
			{
				StringCbPrintf(PContent + l, 131072 - l, "<a href=\"http://nico.ms/sm%ld\" target=\"_blank\" class=\"sfg\">sm%ld</a> ", ul, ul);
				StringCbLength(PContent, 131072, &l);
			}
			else
			{
				StringCbPrintf(PContent + l, 131072 - l, "sm");
				StringCbLength(PContent, 131072, &l);
				i += 2;
			}
			i = (int)(ep - Content);
		}
		else if (strstr(&Content[i], "tb") == &Content[i])
		{ // tb12345 形式
			unsigned long long ull;
			char *ep;
			ull = strtoull(&Content[i + 2], &ep, 16);
			if (ep != &Content[i + 2])
			{
				StringCbPrintf(PContent + l, 131072 - l, "<a href=\"./read.cgi?thread=tb%016llx\" target=\"_blank\" class=\"sfg\">tb%016llx</a> ", ull, ull);
				StringCbLength(PContent, 131072, &l);
			}
			else
			{
				StringCbPrintf(PContent + l, 131072 - l, "tb");
				StringCbLength(PContent, 131072, &l);
				i += 2;
			}
			i = (int)(ep - Content);
		}
		else if (strstr(&Content[i], "mn") == &Content[i])
		{ // mn12345 形式
			unsigned long long ull;
			char *ep;
			ull = strtoull(&Content[i + 2], &ep, 16);
			if (ep != &Content[i + 2])
			{
				StringCbPrintf(PContent + l, 131072 - l, "<a href=\"../bbsimg/mn%016llx.jpg\" target=\"_blank\" class=\"sfg\">mn%016llx</a> ", ull, ull);
				StringCbLength(PContent, 131072, &l);
				ImgCount++;
				if (ImgBuffer == NULL)
				{
					ImgBuffer = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, ImgCount * sizeof(unsigned long long));
				}
				else
				{
					ImgBuffer = HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, ImgBuffer, ImgCount * sizeof(unsigned long long));
				}
				if (ImgBuffer != NULL)
				{
					ImgBuffer[ImgCount - 1] = ull;
				}
			}
			else
			{
				StringCbPrintf(PContent + l, 131072 - l, "tb");
				StringCbLength(PContent, 131072, &l);
				i += 2;
			}
			i = (int)(ep - Content);
		}
		else if (strstr(&Content[i], "%3C") == &Content[i])
		{ // < これ。
			StringCbPrintf(PContent + l, 131072 - l, "&lt;");
			StringCbLength(PContent, 131072, &l);
			i += 3;
		}
		else if (strstr(&Content[i], "%3E") == &Content[i])
		{ // > これ。
		NormalGt:
			StringCbPrintf(PContent + l, 131072 - l, "&gt;");
			StringCbLength(PContent, 131072, &l);
			i += 3;
		}
		else if (Content[i] == '%')
		{ // URL エンコードと呼ばれる奴の展開
			int c;
			sscanf(&Content[i + 1], "%02x", &c);
			PContent[l++] = c;
			i += 3;
		}
		else if (Content[i] == '+')
		{ // SPACE の処理
			PContent[l++] = ' ';
			i++;
		}
		else
		{ // そのまま転送
			PContent[l++] = Content[i++];
		}
	}

	if (AAFlag)
	{
		StringCbPrintf(PContent + l, 131072 - l, "</span>");
		StringCbLength(PContent, 131072, &l);
	}

	if (ImgCount)
	{
		if (ImgBuffer)
		{
			int i;
			StringCbPrintf(PContent + l, 131072 - l, "<div class=\"ManjiList\">");
			StringCbLength(PContent, 131072, &l);
			for (i = 0; i < ImgCount; i++)
			{
				StringCbPrintf(PContent + l, 131072 - l, "<a href=\"../bbsimg/mn%016llx.jpg\" target=\"_blank\" class=\"sfg\"><img src=\"../bbsimg/mn%016llx.jpg\" width=\"64px\" class=\"manji\" /></a>", ImgBuffer[i], ImgBuffer[i]);
				StringCbLength(PContent, 131072, &l);
			}
			StringCbPrintf(PContent + l, 131072 - l, "</div>");
			StringCbLength(PContent, 131072, &l);
			HeapFree(hHeap, 0, ImgBuffer);
		}
	}

	PContent[l] = 0;

	// 文字数計算
	StringCbLength(PContent, 131072, (size_t *)&ContentLen);
	if (ContentLen == 0)
	{
		goto ContentError;
	}
	ContentLen++;
	StringCbLength(PName, 1024, (size_t *)&NameLen);
	NameLen++;
	StringCbLength(Agent, 65536, (size_t *)&AgentLen);
	AgentLen++;

	// 書き込み
	hFile = CreateFile(ThreadName, GENERIC_WRITE, (FILE_SHARE_READ), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		goto FileError;
	}
	if (SAGEFlag)
	{
		GetFileTime(hFile, NULL, NULL, &SAGEft);
	}
	GetSystemTimeAsFileTime(&ftTime);
	SetFilePointer(hFile, 0, NULL, FILE_END);
	WriteFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
	WriteFile(hFile, &NameLen, sizeof(DWORD), &ReadBytes, NULL);
	WriteFile(hFile, PName, NameLen, &ReadBytes, NULL);
	WriteFile(hFile, &ContentLen, sizeof(DWORD), &ReadBytes, NULL);
	WriteFile(hFile, PContent, ContentLen, &ReadBytes, NULL);
	WriteFile(hFile, &AgentLen, sizeof(DWORD), &ReadBytes, NULL);
	WriteFile(hFile, Agent, AgentLen, &ReadBytes, NULL);
	WriteFile(hFile, &RsvLen, sizeof(DWORD), &ReadBytes, NULL);
	WriteFile(hFile, &IdItemId, sizeof(WORD), &ReadBytes, NULL);
	WriteFile(hFile, &IdItemLen, sizeof(DWORD), &ReadBytes, NULL);
	WriteFile(hFile, IdStr, 5, &ReadBytes, NULL);
	WriteFile(hFile, &BaseId, sizeof(WORD), &ReadBytes, NULL);
	if (SAGEFlag)
	{
		SetFileTime(hFile, NULL, NULL, &SAGEft);
	}
	CloseHandle(hFile);
	StringCbPrintf(Name, 1024, "%s.Res", ThreadName);
	ftLast = ftTime;
	hFile = CreateFile(Name, (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		SetFilePointer(hFile, 20, NULL, FILE_BEGIN);
		ReadFile(hFile, &ftLast, sizeof(FILETIME), &ReadBytes, NULL);
		SetFilePointer(hFile, 20, NULL, FILE_BEGIN);
		WriteFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
		if (*(long long *)&ftLast != *(long long *)&ftTime)
		{
			Spd = ((double)(((double)864000000000.0) / ((*((long long *)&ftTime)) - (*((long long *)&ftLast)))));
			SetFilePointer(hFile, 12, NULL, FILE_BEGIN);
			WriteFile(hFile, &Spd, sizeof(double), &ReadBytes, NULL);
		}
		CloseHandle(hFile);
	}

	ConsolePrintf("Location: ./read.cgi?%s\n\n", (Query ? Query : ""));

	HeapFree(hHeap, 0, Query);
	HeapFree(hHeap, 0, PContent);
	HeapFree(hHeap, 0, Agent);
	HeapFree(hHeap, 0, PName);
	HeapFree(hHeap, 0, Content);
	HeapFree(hHeap, 0, Name);
	HeapFree(hHeap, 0, ThreadName);

	return 0;
}

int GetThreadName(char *Buffer, const char *Query)
{
	char *ThreadName, *Temp;
	if (Query == NULL)
	{
		return sprintf(Buffer, "");
	}
	if ((ThreadName = strstr(Query, "thread=")) == NULL)
	{
		return sprintf(Buffer, "");
	}
	if ((Temp = strchr(ThreadName, '&')) != NULL)
	{
		*Temp = '\0';
	}
	return sprintf(Buffer, "\\\\homeserver\\storage\\bbsdata\\%s.dat", ThreadName + strlen("thread="));
}

int GetName(char *Buffer, const char *Query)
{
	char *Name, *Temp;
	if (Query == NULL)
	{
		return sprintf(Buffer, "");
	}
	if ((Name = strstr(Query, "name=")) == NULL)
	{
		return sprintf(Buffer, "Anonymous");
	}
	if ((Temp = strchr(Name, '&')) != NULL)
	{
		*Temp = '\0';
	}
	if (*(Name + strlen("name=")) == '\0')
	{
		return sprintf(Buffer, "Anonymous");
	}
	return sprintf(Buffer, "%s", Name + strlen("name="));
}

int GetContent(char *Buffer, const char *Query)
{
	char *Content, *Temp;
	if (Query == NULL)
	{
		return sprintf(Buffer, "");
	}
	if ((Content = strstr(Query, "content=")) == NULL)
	{
		return sprintf(Buffer, "");
	}
	if ((Temp = strchr(Content, '&')) != NULL)
	{
		*Temp = '\0';
	}
	return sprintf(Buffer, "%s", Content + strlen("content="));
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

static uint32_t xorsft_x = 123456789;
static uint32_t xorsft_y = 362436069;
static uint32_t xorsft_z = 521288629;
static uint32_t xorsft_w = 88675123;

void XorShift_Init(uint32_t seed)
{
	do
	{
		seed = seed * 1812433253 + 1;
		seed ^= seed << 13;
		seed ^= seed >> 17;
		xorsft_x = 123464980 ^ seed;

		seed = seed * 1812433253 + 1;
		seed ^= seed << 13;
		seed ^= seed >> 17;
		xorsft_y = 3447902351 ^ seed;

		seed = seed * 1812433253 + 1;
		seed ^= seed << 13;
		seed ^= seed >> 17;
		xorsft_z = 2859490775 ^ seed;

		seed = seed * 1812433253 + 1;
		seed ^= seed << 13;
		seed ^= seed >> 17;
		xorsft_w = 47621719 ^ seed;
	} while (xorsft_x == 0 && xorsft_y == 0 && xorsft_z == 0 && xorsft_w == 0);
}

uint32_t XorShift_Rand(void)
{
	uint32_t t;
	t = xorsft_x ^ (xorsft_x << 11);
	xorsft_x = xorsft_y;
	xorsft_y = xorsft_z;
	xorsft_z = xorsft_w;
	xorsft_w ^= t ^ (t >> 8) ^ (xorsft_w >> 19);
	return xorsft_w;
}
