#include <windows.h>
#include <TCHAR.H>
#include <STRSAFE.H>

/************************************ *
 * This program MUST NOT use Unicode. *
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

HANDLE hStdOut; // 標準出力, 入力
HANDLE hHeap;		// ヒープ領域
DWORD Error = 0;

//// 邪悪なマクロ ////

// printf() の互換
#define ConsolePrintf(...) (HandlePrintf(hStdOut, __VA_ARGS__))

//// 関数プロトタイプ ////

DWORD HandlePrintf(HANDLE hOut, LPCTSTR pszFmt, ...); // ハンドル出力の printf()

int main()
{
	LPSTR Value;
	// 各種初期化 絶対に失敗してはいけないのでエラーを返すことはできない
	if ((hStdOut = GetStdHandle(STD_OUTPUT_HANDLE)) == INVALID_HANDLE_VALUE)
	{
		ExitProcess((UINT)GetLastError());
	}
	if ((hHeap = GetProcessHeap()) == NULL)
	{
		ExitProcess((UINT)GetLastError());
	}

	Value = (LPSTR)HeapAlloc(hHeap, HEAP_ZERO_MEMORY, MAX_PATH);
	if (Value == NULL)
	{
		Error = GetLastError();
		goto MemError0;
	}
	ConsolePrintf("status: 200 OK\ncontent-type: text/html; charset=utf-8\n\n");
	ConsolePrintf("<!DOCTYPE html><html><head>");
	ConsolePrintf("<title>送信完了</title>");
	ConsolePrintf("<meta name=\"viewport\" content=\"width=device-width, initial-scale=1.0\" />");
	ConsolePrintf("</head><body>");
	ConsolePrintf("<h1>現在テスト中です</h1>");

	if (GetEnvironmentVariable("QUERY_STRING", Value, MAX_PATH) == 0)
	{
		goto EnvError;
	}

	ConsolePrintf("<p>ファイルがアップロードされました<br />以下の ImageID を掲示板に書き込むとリンクが生成されます<p>");
	ConsolePrintf("<tt>%s</tt>", Value + 2);
EnvError:
	ConsolePrintf("<p>ここに現在サーバーにある画像のリストが表示されます。<br />ここで画像の延命処置を行うことができます。</p>");
	ConsolePrintf("</body></html>");

	HeapFree(hHeap, 0, Value);
MemError0:
	if (Error)
	{
		ConsolePrintf("status: 500\ncontent-type: text/plain; charset=utf-8\n\n");
		ConsolePrintf("何かエラーが発生しました。\n次の値を運営に通知してください。\n\n");
		ConsolePrintf("M%08lX", Error);
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
