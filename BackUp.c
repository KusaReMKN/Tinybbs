#include <windows.h>
#include <stdio.h>
#include <time.h>
#include <math.h>

const char *BBSData = "\\\\HomeServer\\Storage\\BBSData\\";
const char *BackUp = "\\\\HomeServer\\Storage\\BBSData\\BackUp\\";

int main(void)
{
	HANDLE hFind, hFile;
	WIN32_FIND_DATA win32fd;
	time_t Time = time(NULL);
	FILETIME ft, ftTime, ftLocal;
	char CopyFrom[MAX_PATH], CopyTo[MAX_PATH];
	DWORD ReadBytes, i, Res;
	double spd;
	double Spd, Ins;
	printf("Start: %s\n", ctime(&Time));
	GetSystemTimeAsFileTime(&ft);
	printf("Backing up ... ");
	hFind = FindFirstFile("\\\\HomeServer\\Storage\\BBSData\\*.dat", &win32fd);
	if (hFind == INVALID_HANDLE_VALUE)
	{
		printf("Error. Not found file named *.dat.\n");
		return -1;
	}
	printf("\n");
	do
	{
		printf("  %22s ", win32fd.cFileName);
		sprintf(CopyFrom, "%s%s", BBSData, win32fd.cFileName);
		sprintf(CopyTo, "%s%s", BackUp, win32fd.cFileName);
		if (CopyFile(CopyFrom, CopyTo, FALSE))
		{
			printf("OK    ");
		}
		else
		{
			printf("Error ");
		}
		// 3018
		sprintf(CopyTo, "%s.Res", CopyFrom);
		printf("Dat:");
		if (ft.dwHighDateTime - win32fd.ftLastWriteTime.dwHighDateTime > 3018)
		{
			printf("Yes ");
			if (DeleteFile(CopyFrom))
			{
				DeleteFile(CopyTo);
				printf("OK    ");
			}
			else
			{
				printf("Error ");
			}
		}
		else
		{
			printf("No  ");
		}
		hFile = CreateFile(CopyFrom, GENERIC_READ, (FILE_SHARE_READ | FILE_SHARE_WRITE), NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		if (hFile != INVALID_HANDLE_VALUE)
		{
			ReadFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
			CloseHandle(hFile);
			hFile = CreateFile(CopyTo, (GENERIC_WRITE | GENERIC_READ), (FILE_SHARE_READ), NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
			if (hFile != INVALID_HANDLE_VALUE)
			{
				ReadFile(hFile, &i, sizeof(DWORD), &ReadBytes, NULL);
				printf("%04d ", i);
				Res = i;
				spd = ((double)(((double)864000000000.0) * i / ((*((long long *)&ft)) - (*((long long *)&ftTime)))));
				SetFilePointer(hFile, 4, NULL, FILE_BEGIN);
				WriteFile(hFile, &spd, sizeof(double), &ReadBytes, NULL);
				SetFilePointer(hFile, 20, NULL, FILE_BEGIN);
				ReadFile(hFile, &ftTime, sizeof(FILETIME), &ReadBytes, NULL);
				spd = ((double)(((double)864000000000.0) / ((*((long long *)&ft)) - (*((long long *)&ftTime)))));
				SetFilePointer(hFile, 12, NULL, FILE_BEGIN);
				WriteFile(hFile, &spd, sizeof(double), &ReadBytes, NULL);
				SetFilePointer(hFile, 4, NULL, FILE_BEGIN);
				ReadFile(hFile, &Spd, sizeof(double), &ReadBytes, NULL);
				ReadFile(hFile, &Ins, sizeof(double), &ReadBytes, NULL);
				i = 0;
				do
				{
					double foo = (Ins + Spd) / 2.0;
					Ins = sqrt(Ins * Spd);
					Spd = foo;
					i++;
				} while (Spd > Ins && i < 1048576);
				printf("%02d(%g)->(%d)", ((i == 1048576) ? (-1) : (i)), Spd, (DWORD)Spd);
				i = (DWORD)Spd;
				SetFilePointer(hFile, 28, NULL, FILE_BEGIN);
				WriteFile(hFile, &i, sizeof(DWORD), &ReadBytes, NULL);
				CloseHandle(hFile);
			}
		}
		printf(" >10Dat:");
		if (Res < 10 && ft.dwHighDateTime - win32fd.ftLastWriteTime.dwHighDateTime > 603)
		{
			printf("Yes ");
			if (DeleteFile(CopyFrom))
			{
				DeleteFile(CopyTo);
				printf("OK    ");
			}
			else
			{
				printf("Error ");
			}
		}
		else
		{
			printf("No  ");
		}
		printf("\n");
	} while (FindNextFile(hFind, &win32fd));
	CloseHandle(hFind);
	printf("Done.\n");
	system("move \\\\homeserver\\storage\\img\\*.* \\\\Homeserver\\wwwroot\\bbsimg\\");
	printf("Finish: %s\n", ctime(&Time));
	return 0;
}
