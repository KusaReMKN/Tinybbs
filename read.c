#include <stdio.h>
#include <windows.h>
#include <time.h>
#include <STRSAFE.H>
#include <TCHAR.H>

//// 定数 ////

size_t PrintfBufferSize = 65536; // *Printf() 系関数のバッファサイズ
const char *DirName = "\\\\HomeServer\\Storage\\BBSData";
WORD IdItemId = 0x4449;

//// グローバル変数 ////

HANDLE hStdOut; // 標準出力
HANDLE hHeap;		// ヒープ領域
DWORD Error;

//// 邪悪なマクロ ////

// printf() の互換
#define ConsolePrintf(...) (HandlePrintf(hStdOut, __VA_ARGS__))

//// 関数プロトタイプ ////

DWORD HandlePrintf(HANDLE hOut, LPTSTR pszFmt, ...); // ハンドル出力の printf()

int GetThreadName(char *Buffer, const char *Query); // クエリからスレッドのファイル名を抽出する。
int GetResName(char *Buffer, const char *ThreadName);
int GetThreadID(char *Buffer, const char *Query);

int main(void)
{
	LPSTR ThreadName, ThreadID, DateTime, ResName, Query, Name, Content, Title, Writable = "";
	SYSTEMTIME sysTime;
	FILETIME ftTime, ftLocal, ftNow;
	DWORD ReadBytes, NameLen, ContentLen, AgentLen, RsvLen, ItemLen;
	HANDLE hFile;
	DWORD i = 0, FileSize;
	WORD ItemId, Id;
	BYTE IdStr[7];
	BOOL ReadOnly = 0;
	double spd;

	// 各種初期化
	if ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE)
	{
		ExitProcess((UINT)GetLastError());
	}
	if ((hHeap = GetProcessHeap()) == NULL)
	{
		ExitProcess((UINT)GetLastError());
	}

	// メモリ確保
	ThreadID = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (!ThreadID)
	{
		Error = GetLastError() + 0x10000;
		goto MemError0;
	}
	ThreadName = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (!ThreadName)
	{
		Error = GetLastError() + 0x20000;
		goto MemError1;
	}
	Query = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 65536);
	if (Query == NULL)
	{
		Error = GetLastError() + 0x30000;
		goto MemError2;
	}
	ResName = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (ResName == NULL)
	{
		Error = GetLastError() + 0x40000;
		goto MemError3;
	}
	Title = HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (Title == NULL)
	{
		Error = GetLastError() + 0x50000;
		goto MemError4;
	}

	// クエリ解析
	if (GetEnvironmentVariable(TEXT("QUERY_STRING"), Query, 65536) == 0)
	{
		Error = GetLastError();
		goto QueryError;
	}

	GetThreadName(ThreadName, Query);
	GetResName(ResName, ThreadName);
	GetThreadID(ThreadID, Query);
	// ファイルの操作と書き込みの表示
	hFile = CreateFile(ThreadName, GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile == INVALID_HANDLE_VALUE)
	{
		Error = GetLastError();
		goto FileError;
	}
	if ((ReadOnly = GetFileAttributes(ThreadName) & FILE_ATTRIBUTE_READONLY) != 0)
	{
		Writable = "disabled";
	}
	// HTTP ヘッダ
	ConsolePrintf("content-type: text/html\n");
	ConsolePrintf("X-UA-Compatible: IE=edge\n\n");

	// HTML 本体開始
	ConsolePrintf("<!DOCTYPE html>");
	ConsolePrintf("<html class=\"no-js\" lang=\"ja\">");
	ConsolePrintf("<head>");
	ConsolePrintf("<meta charset=\"utf-8\" />");
	ConsolePrintf("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
	ConsolePrintf("<meta name=\"theme-color\" content=\"#FFFFFF\" id=\"themecolor\" />");
	ConsolePrintf("<link rel=\"stylesheet\" href=\"../css/common.css\" type=\"text/css\" />");
	ConsolePrintf("<link rel=\"shortcut icon\" href=\"../img/icon16.bmp\" type=\"image/bmp\" sizes=\"16x16\" />");
	ConsolePrintf("<link rel=\"shortcut icon\" href=\"../img/iconmini.bmp\" type=\"image/bmp\" sizes=\"32x32\" />");
	ConsolePrintf("<link rel=\"icon\" href=\"../img/icon192.png\" type=\"image/png\" sizes=\"192x192\" />");
	ConsolePrintf("<link rel=\"apple-touch-icon\" href=\"../img/icon.png\" type=\"image/png\" sizes=\"192x192\" />");
	ConsolePrintf("<script src=\"../js/common.js\"></script>");
	while (1)
	{
		Id = 0;
		ReadFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
		if (ReadBytes != sizeof(FILETIME))
		{
			break;
		}
		ReadFile(hFile, &NameLen, sizeof(DWORD), &ReadBytes, NULL);
		if (ReadBytes != sizeof(DWORD))
		{
			break;
		}
		Name = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, NameLen);
		if (Name == NULL)
		{
			break;
		}
		ReadFile(hFile, Name, NameLen, &ReadBytes, NULL);
		if (ReadBytes != NameLen)
		{
			break;
		}
		ReadFile(hFile, &ContentLen, sizeof(DWORD), &ReadBytes, NULL);
		if (ReadBytes != sizeof(DWORD))
		{
			break;
		}
		Content = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, ContentLen);
		;
		if (Content == NULL)
		{
			break;
		}
		ReadFile(hFile, Content, ContentLen, &ReadBytes, NULL);
		if (ReadBytes != ContentLen)
		{
			break;
		}
		ReadFile(hFile, &AgentLen, sizeof(DWORD), &ReadBytes, NULL);
		if (ReadBytes != sizeof(DWORD))
		{
			break;
		}
		SetFilePointer(hFile, AgentLen, NULL, FILE_CURRENT);
		ReadFile(hFile, &RsvLen, sizeof(DWORD), &ReadBytes, NULL);
		if (ReadBytes != sizeof(DWORD))
		{
			break;
		}
		if (RsvLen == 2)
		{
			ReadFile(hFile, &Id, sizeof(WORD), &ReadBytes, NULL);
			StringCbPrintf(IdStr, 5, "%04X", Id);
		}
		else if (RsvLen >= 11)
		{
			ReadFile(hFile, &ItemId, sizeof(WORD), &ReadBytes, NULL);
			ReadFile(hFile, &ItemLen, sizeof(DWORD), &ReadBytes, NULL);
			if (ItemId == IdItemId)
			{
				ReadFile(hFile, IdStr, ItemLen, &ReadBytes, NULL);
			}
			else
			{
				SetFilePointer(hFile, ItemLen, NULL, FILE_CURRENT);
			}
		}
		else
		{
			SetFilePointer(hFile, RsvLen, NULL, FILE_CURRENT);
			StringCbPrintf(IdStr, 5, "%04X", Id);
		}
		// 表示と領域解放
		if (i == 0)
		{
			ConsolePrintf("<title>%s | TinyBBS</title>", Name);
			StringCbCopy(Title, MAX_PATH, Name);
			ConsolePrintf("<meta name=\"description\" content=\"TinyBBS は誰でも自由に書き込める電子掲示板です。\" />");
			ConsolePrintf("</head>");
			ConsolePrintf("<body class=\"bbg\">");
			// 画面上部の奴
			ConsolePrintf("<header>");
			ConsolePrintf("<div class=\"HeadTitleBody bbg\" id=\"HeadTitleBody\">");
			ConsolePrintf("<div class=\"HeadTitle bfg\">%s</div>", Name);
			ConsolePrintf("<div class=\"HeadTitleButton\">");
			ConsolePrintf("<a onclick=\"location.replace(\'./\');\"><i class=\"fas fa-arrow-left fa-fw HeadTitleButtonLeft\" title=\"Back\"></i></a>");
			ConsolePrintf("<a onclick=\"ChangeColor();\"><i class=\"fas fa-adjust fa-fw HeadTitleButtonRight\" title=\"Color\"></i></a>");
			ConsolePrintf("</div>");
			ConsolePrintf("</div>");
			ConsolePrintf("</header>");
			// タイトル
			ConsolePrintf("<section class=\"ThreadHeader\" id=\"mess\">");
			ConsolePrintf("<h1 class=\"ThreadName\" id=\"RES00000000\">%s</h1>", Name);
			ConsolePrintf("<div><p class=\"ThreadDiscription mona\">%s</p></div>", Content);
			FileTimeToLocalFileTime(&ftTime, &ftLocal);
			FileTimeToSystemTime(&ftLocal, &sysTime);
			ConsolePrintf("<span class=\"ThreadListItemTime\"><span class=\"ThreadListItemId\">(ID:%s)</span><time datetime=\"%04d-%02d-%02d %02d:%02d:%02d.%03d\">%04d-%02d-%02d %02d:%02d:%02d.%03d</time></span>", IdStr, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
			ConsolePrintf("</section>");
			// 書き込み表示の開始点
			ConsolePrintf("<div>");
		}
		else
		{
			ConsolePrintf("<div class=\"ThreadListItemBody\">");
			ConsolePrintf("<span id=\"RES%08X\">%d:</span> <span class=\"ThreadListItemName sfg\">%s</span>", i, i, Name);
			ConsolePrintf("<div class=\"bfg mona\">%s</div>", Content);
			FileTimeToLocalFileTime(&ftTime, &ftLocal);
			FileTimeToSystemTime(&ftLocal, &sysTime);
			ConsolePrintf("<span class=\"ThreadListItemTime\"><span class=\"ThreadListItemId\">(ID:%s)</span><time datetime=\"%04d-%02d-%02d %02d:%02d:%02d.%03d\">%04d-%02d-%02d %02d:%02d:%02d.%03d</time></span>", IdStr, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
			ConsolePrintf("</div>");
		}
		i++;
		HeapFree(hHeap, 0, Name);
		HeapFree(hHeap, 0, Content);
	}
	FileSize = GetFileSize(hFile, NULL);
	ConsolePrintf("<span class=\"FileInfo\">FileSize: %d Bytes; Ave %d Bytes/Res<br />(ThreadID: %s)</span>", FileSize, FileSize / (i ? i : 1), ThreadID);
	// スレ勢い下準備
	SetFilePointer(hFile, 0, NULL, FILE_BEGIN);
	ReadFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
	FileTimeToLocalFileTime(&ftTime, &ftLocal);
	ftTime = ftLocal;
	CloseHandle(hFile);
	ConsolePrintf("</div>");
	// 投稿フォーム
	ConsolePrintf("<form action=\"./write.cgi\" accept-charset=\"utf-8\" method=\"POST\" autocomplete=\"off\" id=\"post\" class=\"FormBody\">");
	ConsolePrintf("<fieldset>");
	ConsolePrintf("<legend class=\"bfg\">スレッドに書き込む</legend>");
	ConsolePrintf("<input type=\"hidden\" name=\"thread\" value=\"%s\" id=\"ThreadID\" />", ThreadID);
	ConsolePrintf("<div class=\"FormItem\">");
	ConsolePrintf("<label for=\"name\" class=\"bfg\">ハンドルネーム</label>");
	ConsolePrintf("<input type=\"text\" name=\"name\" placeholder=\"指定なしで匿名\" id=\"name\" maxlength=\"30\" class=\"bfg bbg\" tabindex=\"1\" %s />", Writable);
	ConsolePrintf("</div>");
	ConsolePrintf("<div class=\"FormItem\">");
	ConsolePrintf("<label for=\"content\" class=\"bfg\">書き込みの内容</label>");
	ConsolePrintf("<textarea name=\"content\" onInput=\"CountText();\" id=\"content\" rows=\"8\" maxlength=\"5000\" class=\"bfg bbg mona\" tabindex=\"2\" placeholder=\"%s\" %s required></textarea>", (ReadOnly ? "このスレッドには書き込めません。" : "書き込み (5000 文字まで)"), Writable);
	ConsolePrintf("</div>");
	ConsolePrintf("<div class=\"ButtonFormSub\">");
	ConsolePrintf("<input type=\"button\" onclick=\"ResPost();\" value=\"&gt;&gt;00\" class=\"bfg bbg\" tabindex=\"-1\" %s />", Writable);
	ConsolePrintf("<input type=\"button\" onclick=\"SagePost();\" value=\"!SAGE!\" class=\"bfg bbg\" tabindex=\"-1\" %s />", Writable);
	ConsolePrintf("<input type=\"button\" onclick=\"window.open(\'./manji.html\', \'_blank\');\" value=\"画像投稿(β)\" class=\"bfg bbg\" tabindex=\"-1\" id=\"counter\" %s />", Writable);
	ConsolePrintf("<input type=\"reset\" onclick=\"ResetValue(); getElementById(\'content\').focus();\" value=\"リセット\" class=\"bfg bbg\" tabindex=\"4\" %s />", Writable);
	ConsolePrintf("</div><hr />");
	ConsolePrintf("<div class=\"ButtonForm\">");
	ConsolePrintf("<input type=\"submit\" onclick=\"WriteValue();\" value=\"書き込む\" class=\"bfg bbg\" tabindex=\"3\" %s />", Writable);
	ConsolePrintf("</div>");
	ConsolePrintf("</fieldset>");
	ConsolePrintf("</form>");
	ConsolePrintf("<b class=\"bfg\">絶対に個人情報を投稿しないでください</b>");
	ConsolePrintf("<hr />");
	// 右下の奴
	ConsolePrintf("<nav id=\"guide\">");
	ConsolePrintf("<a onclick=\"GoRes(\'mess\');\" title=\"Page Top\"><i class=\"fas fa-arrow-up fa-fw\" title=\"Page Top\"></i></a>");
	ConsolePrintf("<a onclick=\"location.reload(true);\" title=\"Refresh\"><i class=\"fas fa-redo fa-fw\" title=\"Refresh\"></i></a>");
	ConsolePrintf("<a onclick=\"GoRes(\'post\');\" title=\"Page Bottom\"><i class=\"fas fa-arrow-down fa-fw\" title=\"Page Bottom\"></i></a>");
	ConsolePrintf("</nav>");
	// 著作権表示
	ConsolePrintf("<footer>");
	ConsolePrintf("<nav>");
	ConsolePrintf("[<a href=\"../guide/\" class=\"sfg\">ガイド</a>] ");
	ConsolePrintf("[<a href=\"https://twitter.com/share?url=http%%3a%%2f%%2fwww%%2etinybbs%%2etk%%2fbbs%%2fread%%2ecgi%%3fthread%%3d%s&text=%s%%20%%7c%%20TinyBBS%%20%%23MKNBBS%%0d%%0a\" class=\"sfg\"><i class=\"fab fa-twitter fa-fw sfg\" title=\"Twitter\"></i></a>]", ThreadID, Title);
	ConsolePrintf("[<div class=\"line-it-button\" style=\"display: none;\" data-lang=\"ja\" data-type=\"share-b\" data-ver=\"2\" data-url=\"%s%%20%%7c%%20TinyBBS%%20%%23MKNBBS%%0d%%0ahttp://www.tinybbs.tk/bbs/read.cgi?thread=%s\" title=\"LINE\"></div>", Title, ThreadID);
	ConsolePrintf("<script src=\"https://d.line-scdn.net/r/web/social-plugin/js/thirdparty/loader.min.js\" async=\"async\" defer=\"defer\"></script>]");
	ConsolePrintf("</nav>");
	ConsolePrintf("<small class=\"copy\">");
	ConsolePrintf("TinyBBS &copy; 2019 <a href=\"https://www.twitter.com/KusaReMKN/\" class=\"copy\">KusaReMKN</a>, FoxAlian. All rights reserved.");
	ConsolePrintf("</small>");
	ConsolePrintf("</footer>");
	// スクリプト
	ConsolePrintf("<script>ReadValue();</script>");
	ConsolePrintf("</body>");
	ConsolePrintf("</html>");
	// レス数更新
	hFile = CreateFile(ResName, (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
	if (hFile != INVALID_HANDLE_VALUE)
	{
		i--;
		WriteFile(hFile, &i, sizeof(DWORD), &ReadBytes, NULL);
		GetSystemTimeAsFileTime(&ftNow);
		FileTimeToLocalFileTime(&ftNow, &ftLocal);
		spd = ((double)(((double)864000000000.0) * i / ((*((long long *)&ftLocal)) - (*((long long *)&ftTime)))));
		SetFilePointer(hFile, 4, NULL, FILE_BEGIN);
		WriteFile(hFile, &spd, sizeof(double), &ReadBytes, NULL);
		SetFilePointer(hFile, 20, NULL, FILE_BEGIN);
		ReadFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
		ftNow = ftLocal;
		FileTimeToLocalFileTime(&ftTime, &ftLocal);
		spd = ((double)(((double)864000000000.0) / ((*((long long *)&ftNow)) - (*((long long *)&ftLocal)))));
		SetFilePointer(hFile, 12, NULL, FILE_BEGIN);
		WriteFile(hFile, &spd, sizeof(double), &ReadBytes, NULL);
		CloseHandle(hFile);
	}
	// メモリ解放
FileError:
QueryError:
	HeapFree(hHeap, 0, Title);
MemError4:
	HeapFree(hHeap, 0, ResName);
MemError3:
	HeapFree(hHeap, 0, Query);
MemError2:
	HeapFree(hHeap, 0, ThreadName);
MemError1:
	HeapFree(hHeap, 0, ThreadID);
MemError0:
	if (Error)
	{
		ConsolePrintf("Status: 500\nContent-type: text/plain; charset=utf-8\n\n");
		ConsolePrintf("何かエラーが発生しました\n以下の値を運営に通知してください\n問題の解決に役立てられます:\n");
		ConsolePrintf("\t\tRBBS_ERROR_%05lX", Error);
	}

	return 0;
}

int GetThreadName(char *Buffer, const char *Query)
{
	char *ThreadName, *Temp;
	size_t len;
	if (Query == NULL)
	{
		*Buffer = 0;
		return 0;
	}
	if ((ThreadName = strstr(Query, "thread=")) == NULL)
	{
		*Buffer = 0;
		return 0;
	}
	if ((Temp = strchr(ThreadName, '&')) != NULL)
	{
		*Temp = '\0';
	}
	StringCbPrintf(Buffer, MAX_PATH, "\\\\homeserver\\storage\\bbsdata\\%s.dat", ThreadName + 7);
	StringCbLength(Buffer, MAX_PATH, &len);
	return len;
}

int GetResName(char *Buffer, const char *ThreadName)
{
	size_t len;
	StringCbPrintf(Buffer, MAX_PATH, "%s.Res", ThreadName);
	StringCbLength(Buffer, MAX_PATH, &len);
	return len;
}

int GetThreadID(char *Buffer, const char *Query)
{
	char *ThreadID, *Temp;
	size_t len;
	if (Query == NULL)
	{
		*Buffer = 0;
		return 0;
	}
	if ((ThreadID = strstr(Query, "thread=")) == NULL)
	{
		*Buffer = 0;
		return 0;
	}
	if ((Temp = strchr(ThreadID, '&')) != NULL)
	{
		*Temp = '\0';
	}
	StringCbPrintf(Buffer, MAX_PATH, "%s", Query + 7);
	StringCbLength(Buffer, MAX_PATH, &len);
	return len;
}

DWORD HandlePrintf(HANDLE hOut, LPTSTR pszFmt, ...)
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
