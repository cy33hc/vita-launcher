
#include <vector>
#include <string>
#include "fs.h"


int main(int, char **)
{
	std::vector<std::string> files = FS::ListFiles("/home/cyee/projects/vita-ftp-client/");
	for (int i=0; i<files.size(); i++)
	{
		printf("%s\n", files[i].c_str());
	}
	return 0;
}
