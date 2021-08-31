
#include <vector>
#include <string>
#include "fs.h"


int main(int argc, char** argv)
{
	if (argc < 2)
	{
		printf("Usage: extract_ebbot <path_for_roms_folder>");
		return 1;
	}
	printf("%s\n", argv[1]);
	std::vector<std::string> files = FS::ListFiles(argv[1]);
	for (int i=0; i<files.size(); i++)
	{
		printf("%s\n", files[i].c_str());
	}
	return 0;
}
