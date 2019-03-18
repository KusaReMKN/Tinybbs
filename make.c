#include <WINDOWS.H>
#include <TCHAR.H>
#include <STRSAFE.H>

/************************************ *
 * This prigram MUST NOT use Unicode. *
 * UTF8 としてファイルは保存されるべきです。
 * ************************************/

//// 定数 ////

size_t PrintfBufferSize = 65536;	// *Printf() 系関数のバッファサイズ

//// グローバル変数 ////

HANDLE hStdOut;	// 標準出力
HANDLE hHeap;	// ヒープ領域
DWORD Error;

//// 邪悪なマクロ ////

// printf() の互換
#define ConsolePrintf(...) (HandlePrintf(hStdOut, __VA_ARGS__))

//// 関数プロトタイプ ////

DWORD HandlePrintf(HANDLE hOut, LPCTSTR pszFmt, ...);	// ハンドル出力の printf()

int main(void)
{
	FILETIME ft;
	LPTSTR Query;
	// 各種初期化 絶対に失敗してはいけないのでエラーを返すことはできない
	if ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE) {
		ExitProcess((UINT)GetLastError());
	}
	if ((hHeap = GetProcessHeap()) == NULL) {
		ExitProcess((UINT)GetLastError());
	}
	// 処理開始
	Beep(784, 10);
	// 現在時刻取得
	GetSystemTimeAsFileTime(&ft);
	// クエリ取得用領域確保
	if ((Query = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, 65536)) == NULL) {
		Error = GetLastError();
		ConsolePrintf("Status: 500 Internal Server Error (Make: %#04X)\n\n", Error);
		ExitProcess(Error);
	}
	// クエリ取得失敗時処理
	if (GetEnvironmentVariable("QUERY_STRING", Query, 65536) == 0) {
		Error = GetLastError();
		ConsolePrintf("Location: ./\n\n");
		HeapFree(hHeap, 0, Query);
		ExitProcess(Error);
	}
	// コンテンツ出力開始
	ConsolePrintf("Location: ./write.cgi?thread=tb%08lx%08lx&%s\n\n", ft.dwHighDateTime, ft.dwLowDateTime, Query);
	// 利用領域解放
	HeapFree(hHeap, 0, Query);
	// 処理終了
	Beep(1568, 25);
	return 0;
}

DWORD HandlePrintf(HANDLE hOut, LPCTSTR pszFmt, ...)
{
	va_list ap;
	LPTSTR Buffer = (LPTSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, PrintfBufferSize);
	size_t BytesOfStr;
	DWORD BytesWritten;
	va_start(ap, pszFmt);
	if (Buffer == NULL) {
		Error = GetLastError();
		return -1;
	}
	StringCbVPrintf(Buffer, PrintfBufferSize, pszFmt, ap);
	va_end(ap);
	StringCbLength(Buffer, PrintfBufferSize, &BytesOfStr);
	WriteFile(hOut, Buffer, (DWORD)BytesOfStr, &BytesWritten, NULL);
	HeapFree(hHeap, 0, Buffer);
	return BytesWritten;
}
