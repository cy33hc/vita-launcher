
#include <vector>
#include <string>
#include <algorithm>
#include "fs.h"
#include "iso.h"
#include "cso.h"
#include "eboot.h"

bool ends_with(const std::string &str, const std::string &suffix)
{
    return str.size() >= suffix.size() &&
           str.compare(str.size() - suffix.size(), suffix.size(), suffix) == 0;
}

int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: extract_ebbot <path_for_roms_folder>\n");
		return 1;
	}
	char sfo_path[512];
	char icon_path[512];
	std::string rom_path;
	std::vector<std::string> files = FS::ListFiles(argv[1]);
	for (int i=0; i<files.size(); i++)
	{
		rom_path = std::string(argv[1]) + "/" + files[i];
		std::string temp = std::string(rom_path);
		std::transform(temp.begin(), temp.end(), temp.begin(),
                            [](unsigned char c){ return std::tolower(c); });
		if (ends_with(temp, "eboot.pbp"))
		{
			printf("==========================================\n");
			printf("Extracting param.sfo and icon0.png from %s\n", rom_path.c_str());
			std::string directory;
			const size_t last_slash_idx = rom_path.rfind('/');
			if (std::string::npos != last_slash_idx)
			{
    			directory = rom_path.substr(0, last_slash_idx);
				int ret = EBOOT::Extract(rom_path.c_str(), directory.c_str());
				if (ret < 0)
				{
					printf("%s - INVALID EBOOT\n", rom_path.c_str());
					return -1;
				}
			}
		}
		else if (ends_with(temp, ".iso") || ends_with(temp, ".cso"))
		{
			const size_t last_slash_idx = rom_path.rfind('/');
			const size_t last_dot_idx = rom_path.rfind(".");
			std::string directory = rom_path.substr(0, last_slash_idx);
			std::string file_prefix= rom_path.substr(0, last_dot_idx);

			FS::MkDirs(directory);
			sprintf(sfo_path, "%s.sfo", file_prefix.c_str());
			sprintf(icon_path, "%s.png", file_prefix.c_str());
			printf("==========================================\n");
			printf("Extracting sfo and icon from %s\n", rom_path.c_str());
			if (ISO::isISO(rom_path))
			{
				ISO *iso = new ISO(rom_path);
				iso->Extract(sfo_path, icon_path);
				delete iso;
				printf("Extracted %s\nExtracted %s\n", sfo_path, icon_path);
			}
			else if (CSO::isCSO(rom_path))
			{
				CSO *cso = new CSO(rom_path);
				cso->Extract(sfo_path, icon_path);
				delete cso;
				printf("Extracted %s\nExtracted %s\n", sfo_path, icon_path);
			}
			else
			{
				printf("ERROR--ERROR--ERROR--ERROR--ERROR--ERROR\n");
				printf("%s is INVALID ISO or CSO\n", rom_path);
				printf("ERROR--ERROR--ERROR--ERROR--ERROR--ERROR\n");
			}
		}

	}
	printf("Finish extracting\n");
	return 0;
}
