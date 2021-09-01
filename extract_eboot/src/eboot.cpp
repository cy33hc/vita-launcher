#include <stdio.h>
#include "fs.h"

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

    int Extract(const char* eboot_path, const char* rom_folder)
    {
        FILE *infile;
        FILE *outfile;
        HEADER header;

        infile = fopen(eboot_path, "rb");
        fread(&header, sizeof(HEADER), 1, infile);

        if (header.signature[0] != 0x00 ||
            header.signature[1] != 0x50 ||
            header.signature[2] != 0x42 ||
            header.signature[3] != 0x50 ||
            header.offset[0] != 0x28)
        {
            // Invalid eboot
            return -1;
        }

        char buffer[maxbuffer];
        for (int i = 0; i < 2; i++)
        {
            int size;

            // Get the size of param.sfo
            int offset1 = header.offset[i];
            int offset2 = header.offset[i+1];
            size = offset2 - offset1;

            int ret = fseek(infile, offset1, SEEK_SET);

            char output_file[512];
            FS::MkDirs(rom_folder);
            sprintf(output_file, "%s/%s", rom_folder, filename[i]);
            outfile = fopen(output_file, "wb");

            do {
                int readsize;
                int bytes_read;
                
                // Make sure we don't exceed the maximum buffer size
                if (size > maxbuffer)
                {
                    readsize = maxbuffer;
                } else {
                    readsize = size;
                }
                size -= readsize;
                
                // Read in the data from the PBP
                bytes_read = fread(buffer, 1, readsize, infile);
                if (bytes_read != readsize)
                {
                    printf("Bytes read is different %lld, %lld\n", bytes_read, readsize);
                }

                // Write the contents of the buffer to the output file
                fwrite(buffer, 1, readsize, outfile);
            } while (size);

            fflush(outfile);
            fclose(outfile);
            printf("Extracted %s\n", output_file);
        }

        fclose(infile);

        return 0;
    }
}
