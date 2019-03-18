#include <WINDOWS.H>
#include <STRSAFE.H>

//// 定数 ////

size_t PrintfBufferSize = 65536; // *Printf() 系関数のバッファサイズ
const char *DirName = "\\\\HomeServer\\Storage\\BBSData";

//// グローバル変数 ////

HANDLE hStdOut; // 標準出力
HANDLE hHeap;		// ヒープ領域
DWORD Error;		// エラーナンバ

//// 邪悪なマクロ ////

// printf() の互換
#define ConsolePrintf(...) (HandlePrintf(hStdOut, __VA_ARGS__))

//// 関数プロトタイプ ////

DWORD HandlePrintf(HANDLE hOut, LPTSTR pszFmt, ...); // ハンドル出力の printf()
int CompLastWriteTime(const void *, const void *);

int main(void)
{
	LPSTR ThreadID, ThreadName, ResFile;
	DWORD ReadBytes, NameLen, Res, Ikioi, NumberOfFindData, i;
	HANDLE hFind, hFile, hRes;
	WIN32_FIND_DATA win32fd;
	PWIN32_FIND_DATA DataList;
	PCHAR Extension, Name;
	FILETIME ftTime, ftLocal, ftLast, ftNow;
	SYSTEMTIME sysTime;
	BOOL ReadOnly;
	double Spd, Ins;

	// 各種初期化
	if ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE)
	{
		ExitProcess((UINT)GetLastError());
	}
	if ((hHeap = GetProcessHeap()) == NULL)
	{
		ExitProcess((UINT)GetLastError());
	}

	// メモリの確保等
	ThreadID = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (ThreadID == NULL)
	{
		Error = GetLastError();
		goto MemError0;
	}
	ThreadName = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (ThreadName == NULL)
	{
		Error = GetLastError() + 0x10000;
		goto MemError1;
	}
	DataList = (PWIN32_FIND_DATA)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, sizeof(WIN32_FIND_DATA) * NumberOfFindData);
	if (DataList == NULL)
	{
		Error = GetLastError() + 0x20000;
		goto MemError2;
	}
	ResFile = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (ResFile == NULL)
	{
		Error = GetLastError() + 0x30000;
		goto MemError3;
	}

	// エラー回収
	if (0)
	{
	MemError3:
		HeapFree(hHeap, 0, DataList);
	MemError2:
		HeapFree(hHeap, 0, ThreadName);
	MemError1:
		HeapFree(hHeap, 0, ThreadID);
	MemError0:
		ConsolePrintf("Status: 500 Internal Server Error (Index: %#04X)\nContent-type: text/plain; charset=utf-8\n\n", Error);
		ConsolePrintf("エラーです 運営に通知してください: IBBS_ERROR_%05lX", Error);
		ExitProcess((UINT)Error);
	}

	// HTTP ヘッダ
	ConsolePrintf("content-type: text/html; charset=utf-8\n");
	ConsolePrintf("X-UA-Compatible: IE=edge\n\n");

	// HTML 本体開始
	ConsolePrintf("<!DOCTYPE html>");
	ConsolePrintf("<html lang=\"ja\">");
	ConsolePrintf("<head>");
	ConsolePrintf("<meta charset=\"utf-8\" />");
	ConsolePrintf("<title>スレッド一覧 | TinyBBS</title>");
	ConsolePrintf("<meta name=\"description\" content=\"TinyBBS は誰でも自由に書き込める電子掲示板です。\" />");
	ConsolePrintf("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
	ConsolePrintf("<meta name=\"theme-color\" content=\"#FFFFFF\" id=\"themecolor\" />");
	ConsolePrintf("<link rel=\"stylesheet\" href=\"../css/common.css\" type=\"text/css\" />");
	ConsolePrintf("<link rel=\"shortcut icon\" href=\"../img/icon16.bmp\" type=\"image/bmp\" sizes=\"16x16\" />");
	ConsolePrintf("<link rel=\"shortcut icon\" href=\"../img/iconmini.bmp\" type=\"image/bmp\" sizes=\"32x32\" />");
	ConsolePrintf("<link rel=\"icon\" href=\"../img/icon192.png\" type=\"image/png\" sizes=\"192x192\" />");
	ConsolePrintf("<link rel=\"apple-touch-icon\" href=\"../img/icon.png\" type=\"image/png\" sizes=\"192x192\" />");
	ConsolePrintf("<script src=\"../js/common.js\"></script>");
	ConsolePrintf("</head>");
	ConsolePrintf("<body class=\"bbg\">");

	// 画面上部の奴
	ConsolePrintf("<header>");
	ConsolePrintf("<div class=\"HeadTitleBody bbg\" id=\"HeadTitleBody\">");
	ConsolePrintf("<div class=\"HeadTitle bfg\">スレッド一覧</div>");
	ConsolePrintf("<div class=\"HeadTitleButton\">");
	ConsolePrintf("<a onclick=\"location.replace(\'./\');\"><i class=\"fas fa-redo fa-fw HeadTitleButtonLeft\" title=\"Refresh\"></i></a>");
	ConsolePrintf("<a onclick=\"ChangeColor();\"><i class=\"fas fa-adjust fa-fw HeadTitleButtonRight\" title=\"Color\"></i></a>");
	ConsolePrintf("</div>");
	ConsolePrintf("</div>");
	ConsolePrintf("</header>");
	// タイトル
	ConsolePrintf("<section class=\"ThreadHeader\">");
	ConsolePrintf("<h1 class=\"ThreadName\">スレッド一覧</h1>");
	ConsolePrintf("<div>");
	ConsolePrintf("<p class=\"ThreadDiscription mona\">");
	ConsolePrintf("初めて投稿する・スレッドを立てるときは<a href=\"../guide/\" class=\"sfg\">ガイド</a>をお読みください。<br />");
	ConsolePrintf("</p>");
	ConsolePrintf("<p class=\"ThreadDiscription mona\" style=\"color: red;\">");
	ConsolePrintf("IPアドレスを含む情報の利用方法を変更しました。必ず<a href=\"../guide/\" class=\"sfg\">ガイド</a>をご確認ください。<br />");
	ConsolePrintf("</p>");
	ConsolePrintf("</div>");
	ConsolePrintf("</section>");
	// ディレクトリの走査
	hFind = FindFirstFile("\\\\HomeServer\\Storage\\BBSData\\tb*.dat", &win32fd);
	NumberOfFindData = 0;
	if (hFind == INVALID_HANDLE_VALUE)
	{
		ConsolePrintf("<p class=\"Error\">過疎ってて草生えない</p>");
	}
	do
	{
		DataList = (PWIN32_FIND_DATA)HeapReAlloc(hHeap, HEAP_ZERO_MEMORY, DataList, sizeof(WIN32_FIND_DATA) * (NumberOfFindData + 1));
		memcpy(&DataList[NumberOfFindData], &win32fd, sizeof(WIN32_FIND_DATA));
		NumberOfFindData++;
	} while (FindNextFile(hFind, &win32fd));

	qsort(DataList, NumberOfFindData, sizeof(WIN32_FIND_DATA), CompLastWriteTime);

	// スレッドリストの表示
	ConsolePrintf("<nav>");
	for (i = 0; i < NumberOfFindData; i++)
	{
		if ((Extension = strchr(DataList[i].cFileName, '.')) == NULL || strcmp(Extension, ".dat"))
		{
			continue;
		}
		StringCbPrintf(ThreadName, MAX_PATH, "%s\\%s", DirName, DataList[i].cFileName);
		hFile = CreateFile(ThreadName, GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile == INVALID_HANDLE_VALUE)
		{
			continue;
		}
		ReadOnly = GetFileAttributes(ThreadName) & FILE_ATTRIBUTE_READONLY;
		ReadFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
		if (ReadBytes != sizeof(FILETIME))
		{
			goto ErrorReadingFile;
		}
		ReadFile(hFile, &NameLen, sizeof(DWORD), &ReadBytes, NULL);
		if (ReadBytes != sizeof(DWORD))
		{
			goto ErrorReadingFile;
		}
		Name = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, NameLen);
		if (Name == NULL)
		{
			goto ErrorReadingFile;
		}
		ReadFile(hFile, Name, NameLen, &ReadBytes, NULL);
		if (ReadBytes != NameLen)
		{
			HeapFree(hHeap, 0, Name);
			goto ErrorReadingFile;
		}
		if (0) // ファイル読み込みエラー
		{
		ErrorReadingFile:
			CloseHandle(hFile);
			continue;
		}
		// レス数表示
		StringCbPrintf(ResFile, MAX_PATH, "%s.Res", ThreadName);
		Res = Spd = Ins = Ikioi = 0;
		GetSystemTimeAsFileTime(&ftNow);
		ftLast = ftNow;
		hRes = CreateFile(ResFile, GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hRes != INVALID_HANDLE_VALUE)
		{
			ReadFile(hRes, &Res, sizeof(DWORD), &ReadBytes, NULL);
			ReadFile(hRes, &Spd, sizeof(double), &ReadBytes, NULL);
			ReadFile(hRes, &Ins, sizeof(double), &ReadBytes, NULL);
			ReadFile(hRes, &ftLast, sizeof(FILETIME), &ReadBytes, NULL);
			ReadFile(hRes, &Ikioi, sizeof(DWORD), &ReadBytes, NULL);
			CloseHandle(hRes);
		}
		StringCbCopy(ThreadID, MAX_PATH, DataList[i].cFileName);
		*(strchr(ThreadID, '.')) = '\0';
		ConsolePrintf("<a href=\"./read.cgi?thread=%s\"><div class=\"ThreadListItemBody\">", ThreadID);
		ConsolePrintf("<span class=\"ThreadListItemName sfg\">%s</span>", Name);
		FileTimeToLocalFileTime(&ftTime, &ftLocal);
		FileTimeToSystemTime(&ftLocal, &sysTime);
		ConsolePrintf("<span class=\"ThreadListItemRes\">%dレス(%d勢い)%s</span>", Res, Ikioi, (ReadOnly ? "<i class=\"fas fa-times-circle fa-fw \" title=\"Read only\"></i>" : ""));
		ConsolePrintf("<time class=\"ThreadListItemTime\" datetime=\"%04d-%02d-%02d %02d:%02d:%02d.%03d\">%04d-%02d-%02d %02d:%02d:%02d.%03d</time>", sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds, sysTime.wYear, sysTime.wMonth, sysTime.wDay, sysTime.wHour, sysTime.wMinute, sysTime.wSecond, sysTime.wMilliseconds);
		ConsolePrintf("</div></a>");
		HeapFree(hHeap, 0, Name);
		CloseHandle(hFile);
	}
	ConsolePrintf("</nav>");
	HeapFree(hHeap, 0, ResFile);
	HeapFree(hHeap, 0, DataList);
	HeapFree(hHeap, 0, ThreadName);
	HeapFree(hHeap, 0, ThreadID);
	FindClose(hFind);
	ConsolePrintf("<span class=\"FileInfo\">%dスレ</span>", i);

	// 新スレフォーム
	ConsolePrintf("<form action=\"./make.cgi\" accept-charset=\"utf-8\" method=\"GET\" autocomplete=\"off\" id=\"post\" class=\"FormBody\">");
	ConsolePrintf("<fieldset>");
	ConsolePrintf("<legend class=\"bfg\">新しいスレッドを立てる</legend>");
	ConsolePrintf("<div class=\"FormItem\">");
	ConsolePrintf("<label for=\"name\" class=\"bfg\">スレッド名</label>");
	ConsolePrintf("<input type=\"text\" name=\"name\" placeholder=\"スレッドの内容がわかる名前\" id=\"name\" maxlength=\"30\" class=\"bfg bbg\" required />");
	ConsolePrintf("</div>");
	ConsolePrintf("<div class=\"FormItem\">");
	ConsolePrintf("<label for=\"content\" class=\"bfg\">スレッドの内容</label>");
	ConsolePrintf("<textarea name=\"content\" id=\"content\" rows=\"10\" placeholder=\"スレッドの内容の説明など\" maxlength=\"1000\" class=\"bfg bbg\" required></textarea>");
	ConsolePrintf("</div>");
	ConsolePrintf("<div class=\"ButtonFormSub\">");
	ConsolePrintf("<input type=\"reset\" onclick=\"getElementById(\'content\').focus();\" value=\"リセット\" class=\"bfg bbg\" tabindex=\"2\" />");
	ConsolePrintf("</div><hr />");
	ConsolePrintf("<div class=\"ButtonForm\">");
	ConsolePrintf("<input type=\"submit\" value=\"スレッドを立てる\" class=\"bfg bbg\" tabindex=\"1\" />");
	ConsolePrintf("</div>");
	ConsolePrintf("</fieldset>");
	ConsolePrintf("</form>");
	ConsolePrintf("<b class=\"bfg\">絶対に個人情報を投稿しないでください</b>");
	ConsolePrintf("<hr />");
	// 著作権表示
	ConsolePrintf("<footer>");
	ConsolePrintf("<nav>");
	ConsolePrintf("[<a href=\"../guide/\" class=\"sfg\">ガイド</a>]");
	ConsolePrintf("[<a href=\"https://twitter.com/share?url=http%%3a%%2f%%2fwww%%2etinybbs%%2etk%%2f&text=TinyBBS%%20%%23MKNBBS\" class=\"sfg\"><i class=\"fab fa-twitter fa-fw sfg\" title=\"Twitter\"></i></a>]");
	ConsolePrintf("[<div class=\"line-it-button\" style=\"display: none;\" data-lang=\"ja\" data-type=\"share-b\" data-ver=\"2\" data-url=\"http://www.tinybbs.tk/\" title=\"LINE\"></div>");
	ConsolePrintf("<script src=\"https://d.line-scdn.net/r/web/social-plugin/js/thirdparty/loader.min.js\" async=\"async\" defer=\"defer\"></script>]");

	ConsolePrintf("</nav>");
	ConsolePrintf("<small class=\"copy\">");
	ConsolePrintf("TinyBBS &copy; 2019 <a href=\"https://www.twitter.com/KusaReMKN/\" class=\"copy\">KusaReMKN</a>, FoxAlian. All rights reserved.");
	ConsolePrintf("</small>");
	ConsolePrintf("</footer>");
	// スクリプト
	ConsolePrintf("<script>ReadColor();</script>");
	ConsolePrintf("</body>");
	ConsolePrintf("</html>");

	return 0;
}

int CompLastWriteTime(const void *p1, const void *p2)
{
	if ((((WIN32_FIND_DATA *)p1)->ftLastWriteTime).dwHighDateTime == (((WIN32_FIND_DATA *)p2)->ftLastWriteTime).dwHighDateTime)
	{
		return (((WIN32_FIND_DATA *)p2)->ftLastWriteTime).dwLowDateTime - (((WIN32_FIND_DATA *)p1)->ftLastWriteTime).dwLowDateTime;
	}
	return (((WIN32_FIND_DATA *)p2)->ftLastWriteTime).dwHighDateTime - (((WIN32_FIND_DATA *)p1)->ftLastWriteTime).dwHighDateTime;
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
