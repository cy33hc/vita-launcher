
#include <vector>
#include <string>
#include <algorithm>
#include "fs.h"
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
	printf("%s\n", argv[1]);
	std::vector<std::string> files = FS::ListFiles(argv[1]);
	for (int i=0; i<files.size(); i++)
	{
		std::string temp = std::string(files[i]);
		std::transform(temp.begin(), temp.end(), temp.begin(),
                            [](unsigned char c){ return std::tolower(c); });
		printf("%s\n", files[i].c_str());
		if (ends_with(temp, "eboot.pbp"))
		{
			printf("Extracting param.sfo and icon0.png from %s\n", files[i].c_str());
			std::string directory;
			const size_t last_slash_idx = files[i].rfind('/');
			if (std::string::npos != last_slash_idx)
			{
    			directory = files[i].substr(0, last_slash_idx);
				int ret = EBOOT::Extract(files[i].c_str(), directory.c_str());
				if (ret < 0)
				{
					printf("%s - INVALID EBOOT\n", files[i].c_str());
					return -1;
				}
			}
		}
	}
	return 0;
}
