

#include <Windows.h>
#include <tchar.h>
#include <stdio.h>
#include <strsafe.h>
#pragma comment(lib, "User32.lib")


bool get_files_in_directory(const std::string& path, std::vector<std::string> &filenames)
{
	filenames.clear();

	std::string path_slash = path;
	if (path_slash.back() != '/' && path_slash.back() != '\\')
		path_slash.append("/");

	TCHAR szDir[MAX_PATH];
	size_t ret;
	mbstowcs_s(&ret, szDir, path_slash.c_str(), path_slash.length());
	StringCchCat(szDir, MAX_PATH, TEXT("/*"));

	WIN32_FIND_DATA ffd;
	HANDLE hFind = FindFirstFile(szDir, &ffd);
	if (hFind == INVALID_HANDLE_VALUE)
		return false;

	char ch[MAX_PATH];
	std::string filename;
	do {
		WideCharToMultiByte(CP_ACP, NULL, ffd.cFileName, -1, ch, MAX_PATH, NULL, NULL);
		wcstombs_s(&ret, ch, ffd.cFileName, MAX_PATH);
		if (strcmp(ch, ".") != 0 && strcmp(ch, "..") != 0) {
			filename = path_slash;
			filename.append(ch);
			filenames.push_back(filename);
		}
	} while (FindNextFile(hFind, &ffd) != 0);

	DWORD dwError = GetLastError();
	if (dwError != ERROR_NO_MORE_FILES) {
		FindClose(hFind);
		return false;
	}

	FindClose(hFind);
	return true;
}

