#include <psp2/kernel/iofilemgr.h>
#include <cstdio>
#include <cstdlib>
#include "fs.h"
#include "game.h"

typedef struct {
   char   signature[4];
   int      version;
   int      offset[8];
} HEADER;

char *filename[2] = {
   "param.sfo",
   "icon0.png"
};

int maxbuffer = 1024;

namespace EBOOT {

    void Extract(char* eboot_path, char* rom_folder)
    {
        void *infile;
        void *outfile;
        HEADER header;
        int loop0;
        int total_size;

        infile = FS::OpenRead(eboot_path);
        FS::Read(infile, &header, sizeof(HEADER));

        char buffer[maxbuffer];
        for (int i = 0; i < 2; i++)
        {
            int size;

            // Get the size of param.sfo
            size = header.offset[i+1] - header.offset[i];
            FS::Seek(infile, header.offset[i]);

            char output_file[64];
            sprintf(output_file, "ux0:data/SMLA00001/data/%s", rom_folder);
            FS::MkDirs(output_file);
            sprintf(output_file, "ux0:data/SMLA00001/data/%s/%s", rom_folder, filename[i]);
            outfile = FS::Create(output_file);

            do {
                int readsize;
                
                // Make sure we don't exceed the maximum buffer size
                if (size > maxbuffer)
                {
                    readsize = maxbuffer;
                } else {
                    readsize = size;
                }
                size -= readsize;
                
                // Read in the data from the PBP
                FS::Read(infile, buffer, readsize);

                // Write the contents of the buffer to the output file
                FS::Write(outfile, buffer, readsize);
            } while (size);

            FS::Close(outfile);
        }

        FS::Close(infile);
    }
}
