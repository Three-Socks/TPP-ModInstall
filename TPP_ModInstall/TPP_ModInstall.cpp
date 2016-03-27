#include <stdio.h>
#include <tchar.h>

#include <string>
#include <vector>
#include <sstream>
#include <fstream>
#include <iostream>
#include <iterator>

#include <conio.h>
#include <windows.h>

#include "resource.h"

#include "SimpleIni.h"

// CityHash v1.0.3
#include "city.h"

using namespace std;

void clear_screen() {
	COORD topLeft = { 0, 0 };
	HANDLE console = GetStdHandle(STD_OUTPUT_HANDLE);
	CONSOLE_SCREEN_BUFFER_INFO screen;
	DWORD written;

	GetConsoleScreenBufferInfo(console, &screen);
	FillConsoleOutputCharacterA(
		console, ' ', screen.dwSize.X * screen.dwSize.Y, topLeft, &written
		);
	FillConsoleOutputAttribute(
		console, FOREGROUND_GREEN | FOREGROUND_RED | FOREGROUND_BLUE,
		screen.dwSize.X * screen.dwSize.Y, topLeft, &written
		);
	SetConsoleCursorPosition(console, topLeft);

	cout << "TPP_ModInstall v1.0\n\n";
}

void pause()
{
	cout << "\nPress any key to continue..." << endl;
	_getch();
}


int mI_search_and_replace(string Tpp_mod, string Tpp, string Tpp_find, string Tpp_replace)
{
	ifstream Tppin(Tpp);
	ostringstream Tpp_out_stream;
	Tpp_out_stream << Tpp << ".temp";
	string Tpp_out = Tpp_out_stream.str();
	ofstream Tppout(Tpp_out);

	if (!Tppin)
	{
		cerr << Tpp << " was not found. Make sure you move all files and run this file in the\nC:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\master\\0\\ folder." << "\n";
		return 1;
	}

	if (!Tppout)
	{
		cerr << Tpp_out << " was not found. Make sure you move all files and run this file in the\nC:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\master\\0\\ folder." << "\n";
		return 1;
	}

	string Tppline;
	size_t Tpplen = Tpp_find.length();
	while (getline(Tppin, Tppline))
	{
		// Check if mod name is found. If found dont continue with replace.
		size_t Tpp_mod_pos = Tppline.find(Tpp_mod);
		if (Tpp_mod_pos != string::npos)
		{
			clear_screen();
			cerr << "Looks like " << Tpp_mod << " " << Tpp << " file patch was already made, skipping..." << "\n";
			pause();
			//Sleep(2000);
		}
		else
		{
			size_t Tpppos = Tppline.find(Tpp_find);
			if (Tpppos != string::npos)
				Tppline.replace(Tpppos, Tpplen, Tpp_replace);
		}

		Tppout << Tppline << '\n';
	}

	// Close files for deleting/renaming
	Tppin.close();
	Tppout.close();

	// Delete
	remove(Tpp.c_str());
	bool stream_r = !ifstream(Tpp.c_str());
	if (!stream_r)
	{
		perror("Error deleting");
		return 1;
	}

	// Rename
	int rc = rename(Tpp_out.c_str(), Tpp.c_str());
	if (rc)
	{
		perror("Error renaming");
		return 1;
	}
	return 0;
}

void mI_load_resource(int res, string out_file_name)
{
	HRSRC myResource = ::FindResource(NULL, MAKEINTRESOURCE(res), RT_RCDATA);
	unsigned int myResourceSize = ::SizeofResource(NULL, myResource);
	HGLOBAL myResourceData = ::LoadResource(NULL, myResource);
	void* pMyBinaryData = ::LockResource(myResourceData);

	std::ofstream f(out_file_name, std::ios::out | std::ios::binary);
	f.write((char*)pMyBinaryData, myResourceSize);
	f.close();
}

string mI_mod_name_safe(string mod_name)
{
	string mod_name_safe_slash;
	string mod_name_safe_dot;
	string mod_name_safe;

	remove_copy(mod_name.begin(), mod_name.end(), back_inserter(mod_name_safe_slash), '/');
	remove_copy(mod_name_safe_slash.begin(), mod_name_safe_slash.end(), back_inserter(mod_name_safe_dot), '\\');
	remove_copy(mod_name_safe_dot.begin(), mod_name_safe_dot.end(), back_inserter(mod_name_safe), '.');

	return mod_name_safe;
}

int main()
{
	// Stop console window from exiting when run from explorer
	CONSOLE_SCREEN_BUFFER_INFO csbi = { 0 };
	HANDLE hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (GetConsoleScreenBufferInfo(hStdOutput, &csbi))
	{
		// Check whether the cursor position is (0,0)
		if (csbi.dwCursorPosition.X == 0 &&
			csbi.dwCursorPosition.Y == 0)
		{
			// By using atexit, the pause() function will be called
			// automatically when the program exits.
			atexit(pause);
		}
	}

	SetConsoleTitle(L"TPP_ModInstall v1.0");
	clear_screen();

	// Extract Resources
	mI_load_resource(IDR_RCDATA1, "MGSV_QAR_Tool.exe");
	mI_load_resource(IDR_RCDATA2, "zlib1.dll");

	// Load ini and error check.
	CSimpleIniA ini(false, true, false);
	SI_Error rc = ini.LoadFile("TPP_ModInstall.ini");

	if (rc < 0)
	{
		cerr << "Unable to load config file TPP_ModInstall.ini" << "\n";
		return 1;
	}

	long Tpp_data_ini = ini.GetLongValue("config", "tpp_install_data", 0);

	ostringstream Tpp_data_stream;
	Tpp_data_stream << "0" << Tpp_data_ini << ".dat";
	string Tpp_data = Tpp_data_stream.str();

	if (Tpp_data.empty())
	{
		cerr << "tpp_install_data is empty in TPP_ModInstall.ini" << "\n";
		return 1;
	}

	bool Tpp_data_notfound = !ifstream(Tpp_data.c_str());
	if (Tpp_data_notfound)
	{
		cerr << Tpp_data << " was not found. Make sure you move all files and run this file in the\nC:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\master\\0\\ folder." << "\n";
		perror(nullptr);
		return 1;
	}

	// mod lua
	string Tpp_main_mod_name;
	vector<string> tpp_install_mod_names;
	vector<string> tpp_install_mod_lua;

	// Create dictionary.txt
	ofstream resource_dict("dictionary.txt");
	if (!resource_dict)
	{
		cerr << "Unable to create dictionary.txt" << "\n";
		perror(nullptr);
		return 1;
	}

	// get all values of tpp_install_mod_name
	CSimpleIniA::TNamesDepend values;
	ini.GetAllValues("config", "tpp_install_mod_lua", values);

	values.sort(CSimpleIniA::Entry::LoadOrder());

	CSimpleIniA::TNamesDepend::const_iterator i;
	int i_num = 0;
	for (i = values.begin(); i != values.end(); ++i)
	{
		string Tpp_mods_name = mI_mod_name_safe(i->pItem);

		if (i_num == 0)
			Tpp_main_mod_name = Tpp_mods_name;

		ostringstream Tpp_mods_lua_stream;
		Tpp_mods_lua_stream << Tpp_mods_name << ".lua";
		string Tpp_mods_lua = Tpp_mods_lua_stream.str();

		bool Tpp_mod_notfound = !ifstream(Tpp_mods_lua.c_str());
		if (Tpp_mod_notfound)
		{
			cerr << Tpp_mods_lua << " was not found. Make sure you move all files and run this file in the\nC:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\master\\0\\ folder." << "\n";
			perror(nullptr);
			return 1;
		}

		// Add mod name and lua to string array
		tpp_install_mod_names.push_back(Tpp_mods_name);
		tpp_install_mod_lua.push_back(Tpp_mods_lua);

		// Add mod name to dictionary.txt
		resource_dict << "/Assets/tpp/script/lib/" << Tpp_mods_name << "\n";

		// Clear stringstream
		Tpp_mods_lua_stream.str(string());
		Tpp_mods_lua_stream.clear();

		i_num++;
	}

	// Close dictionary.txt
	resource_dict << "/Assets/tpp/script/lib/Tpp\n/Assets/tpp/script/lib/TppMain";
	resource_dict.close();

	/*ostringstream Tpp_backup_stream;
	Tpp_backup_stream << "backup-" << Tpp_main_mod_name;
	string Tpp_backup = Tpp_backup_stream.str();

	size_t newsize_backup = strlen(Tpp_backup.c_str()) + 1;
	wchar_t * wcstring_backup = new wchar_t[newsize_backup];
	size_t convertedChars_backup = 0;
	mbstowcs_s(&convertedChars_backup, wcstring_backup, newsize_backup, Tpp_backup.c_str(), _TRUNCATE);

	cout << "Backing up " << Tpp_data << " ..." << "\n\n";

	CreateDirectory(wcstring_backup, NULL);

	Tpp_backup_stream << "\\" << Tpp_data;

	ifstream src_bk(Tpp_data, ios::binary);
	ofstream dst_bk(Tpp_backup_stream.str(), ios::binary);

	dst_bk << src_bk.rdbuf();

	bool Tpp_copied_bk_notfound = !ifstream(Tpp_backup_stream.str().c_str());
	if (Tpp_copied_bk_notfound)
	{
		cerr << Tpp_data << " could not be backed up." << "\n";
		perror(nullptr);
		pause();
	}*/

	// Extract data file.
	cout << "Extracting " << Tpp_data << "..." << "\n\n";
	Sleep(2000);

	STARTUPINFO info = { sizeof(info) };
	PROCESS_INFORMATION processInfo;

	ostringstream Tpp_cmd_stream;
	Tpp_cmd_stream << " " << Tpp_data << " -r";
	string Tpp_cmd = Tpp_cmd_stream.str();

	size_t newsize = strlen(Tpp_cmd.c_str()) + 1;
	wchar_t * wcstring = new wchar_t[newsize];
	size_t convertedChars = 0;
	mbstowcs_s(&convertedChars, wcstring, newsize, Tpp_cmd.c_str(), _TRUNCATE);

	if (CreateProcess(L"MGSV_QAR_Tool.exe", wcstring, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		perror("Cannot create process MGSV_QAR_Tool.exe");
		return 1;
	}

	cout << "\n";

	// Copy & patch tpp lib lua.
	cout << "Copying and patching files..." << "\n\n";
	Sleep(2000);

	// Check MGSV_QAR_Tool created an inf file and open it.
	ostringstream Tpp_inf_stream;
	Tpp_inf_stream << "0" << Tpp_data_ini << ".inf";
	string Tpp_inf = Tpp_inf_stream.str();

	ofstream Tpp_inf_out;
	Tpp_inf_out.open(Tpp_inf, ios::app);
	if (!Tpp_inf_out)
	{
		cerr << Tpp_inf << " was not found. Make sure you move all files and run this file in the\nC:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\master\\0\\ folder." << "\n";
		return 1;
	}

	for (vector<string>::const_iterator i = tpp_install_mod_names.begin(); i != tpp_install_mod_names.end(); ++i)
	{
		// Create lua file CityHash
		ostringstream Tpp_cityhash_stream;
		Tpp_cityhash_stream << "tpp/script/lib/" << *i;
		string Tpp_cityhash_str = Tpp_cityhash_stream.str();
		uint64_t seed0 = 0x9ae16a3b2f90404f;

		byte byte_array[8];
		size_t bufpos = 0;

		for (size_t i = Tpp_cityhash_str.size() - 1, j = 0; i >= 0 && j < 8; i--, j++)
			byte_array[j] = Tpp_cityhash_str[i];

		uint64_t seed1 =
			static_cast<uint64_t>(byte_array[0]) |
			static_cast<uint64_t>(byte_array[1]) << 8 |
			static_cast<uint64_t>(byte_array[2]) << 16 |
			static_cast<uint64_t>(byte_array[3]) << 24 |
			static_cast<uint64_t>(byte_array[4]) << 32 |
			static_cast<uint64_t>(byte_array[5]) << 40 |
			static_cast<uint64_t>(byte_array[6]) << 48 |
			static_cast<uint64_t>(byte_array[7]) << 56;

		uint64_t hash = CityHash64WithSeeds(Tpp_cityhash_str.c_str(), Tpp_cityhash_str.size(), seed0, seed1) & 0x3FFFFFFFFFFFF;
		hash = (static_cast<uint64_t>(796) << static_cast<uint64_t>(51)) | hash;

		ostringstream Tpp_inf_string_stream;
		Tpp_inf_string_stream << std::hex << hash << "|0" << Tpp_data_ini << "\\Assets\\tpp\\script\\lib\\" << *i << ".lua key=0 version=2 compressed=0\n";
		string Tpp_inf_string = Tpp_inf_string_stream.str();

		Tpp_inf_out << Tpp_inf_string;

		string Tpp = "\\Assets\\tpp\\script\\lib\\Tpp.lua";

		ostringstream Tpp_file_stream;
		Tpp_file_stream << "0" << Tpp_data_ini << Tpp;
		string Tpp_file_path = Tpp_file_stream.str();

		string Tpp_find = "\"/Assets/tpp/script/lib/TppMbFreeDemo.lua\"";
		ostringstream Tpp_replace_stream;
		Tpp_replace_stream << "\"/Assets/tpp/script/lib/TppMbFreeDemo.lua\",\"/Assets/tpp/script/lib/" << *i << ".lua\"";
		string Tpp_replace = Tpp_replace_stream.str();

		if (mI_search_and_replace(*i, Tpp_file_path, Tpp_find, Tpp_replace))
		{
			perror(nullptr);
			return 1;
		}

		string TppMain = "\\Assets\\tpp\\script\\lib\\TppMain.lua";

		ostringstream TppMain_file_stream;
		TppMain_file_stream << "0" << Tpp_data_ini << TppMain;
		string TppMain_file_path = TppMain_file_stream.str();

		string TppMain_find = "TppMission.UpdateForMissionLoad";
		ostringstream TppMain_replace_stream;
		TppMain_replace_stream << "TppMission.UpdateForMissionLoad," << *i << ".Update";
		string TppMain_replace = TppMain_replace_stream.str();

		if (mI_search_and_replace(*i, TppMain_file_path, TppMain_find, TppMain_replace))
		{
			perror(nullptr);
			return 1;
		}
	}

	for (vector<string>::const_iterator i = tpp_install_mod_lua.begin(); i != tpp_install_mod_lua.end(); ++i)
	{
		ostringstream Tpp_mod_copy_stream;
		Tpp_mod_copy_stream << "0" << Tpp_data_ini << "\\Assets\\tpp\\script\\lib\\" << *i;
		string Tpp_mod_copy = Tpp_mod_copy_stream.str();

		ifstream src(*i, ios::binary);
		ofstream dst(Tpp_mod_copy, ios::binary);

		dst << src.rdbuf();

		bool Tpp_mod_copied_notfound = !ifstream(Tpp_mod_copy.c_str());
		if (Tpp_mod_copied_notfound)
		{
			cerr << Tpp_mod_copy << " was not found. Make sure you move all files and run this file in the\nC:\\Program Files (x86)\\Steam\\steamapps\\common\\MGS_TPP\\master\\0\\ folder." << "\n";
			perror(nullptr);
			return 1;
		}

		Tpp_mod_copy_stream.str(string());
		Tpp_mod_copy_stream.clear();
	}

	// Repack
	cout << "Repacking " << Tpp_data << "..." << "\n\n";
	Sleep(2000);

	ostringstream Tpp_cmd2_stream;
	Tpp_cmd2_stream << " " << "0" << Tpp_data_ini << ".inf" << " -r";
	string Tpp_cmd2 = Tpp_cmd2_stream.str();

	size_t newsize2 = strlen(Tpp_cmd2.c_str()) + 1;
	wchar_t * wcstring2 = new wchar_t[newsize2];
	size_t convertedChars2 = 0;
	mbstowcs_s(&convertedChars2, wcstring2, newsize2, Tpp_cmd2.c_str(), _TRUNCATE);

	if (CreateProcess(L"MGSV_QAR_Tool.exe", wcstring2, NULL, NULL, TRUE, 0, NULL, NULL, &info, &processInfo))
	{
		WaitForSingleObject(processInfo.hProcess, INFINITE);
		CloseHandle(processInfo.hProcess);
		CloseHandle(processInfo.hThread);
	}
	else
	{
		cerr << "Cannot create process MGSV_QAR_Tool.exe" << "\n";
		perror(nullptr);
		return 1;
	}

	remove("MGSV_QAR_Tool.exe");
	remove("zlib1.dll");
	remove("dictionary.txt");

	clear_screen();

	// Finish.
	cout << "=======================\n";
	cout << Tpp_main_mod_name << " installed.\n";
	cout << "=======================\n";
	cout << "- Original " << Tpp_data << " has been backed up to backup-" << Tpp_main_mod_name << " folder\n";
	cout << "- To Uninstall mod, restore the " << Tpp_data << " from the backup folder\n";
	cout << "=======================\n";

	cout << "\n";

	return 0;
}


