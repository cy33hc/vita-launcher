#include <cstdio>
#include <cstdlib>
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

size_t maxbuffer = 1024;

namespace EBOOT {

    void swapbytes(void *inp, size_t len)
    {
        unsigned int i;
        unsigned char *in=(unsigned char *)inp,tmp;

        for(i=0;i<len/2;i++) {
            tmp=*(in+i);
            *(in+i)=*(in+len-i-1);
            *(in+len-i-1)=tmp;
        }
    }

    int Extract(const char* eboot_path, const char* rom_folder)
    {
        int infile;
        int outfile;
        HEADER header;

        infile = FS::OpenRead(eboot_path);
        FS::Read(infile, &header, sizeof(HEADER));

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
            size_t size;

            // Get the size of param.sfo
            off_t offset1 = header.offset[i];
            off_t offset2 = header.offset[i+1];
            printf("offset1=%ld\n", offset1);
            printf("offset2=%ld\n", offset2);

            size = offset2 - offset1;
            printf("size=%lld\n", size);

            off_t ret = FS::Seek(infile, offset1);
            printf("Seek %ld\n", ret);

            char output_file[512];
            FS::MkDirs(rom_folder);
            sprintf(output_file, "%s/%s", rom_folder, filename[i]);
            printf("%s\n", output_file);
            outfile = FS::Create(output_file);

            do {
                size_t readsize;
                size_t bytes_read;
                
                // Make sure we don't exceed the maximum buffer size
                if (size > maxbuffer)
                {
                    readsize = maxbuffer;
                } else {
                    readsize = size;
                }
                size -= readsize;
                
                // Read in the data from the PBP
                bytes_read = FS::Read(infile, buffer, readsize);
                if (bytes_read != readsize)
                {
                    printf("Bytes read is different %lld, %lld\n", bytes_read, readsize);
                }

                // Write the contents of the buffer to the output file
                FS::Write(outfile, buffer, readsize);
            } while (size);

            FS::Close(outfile);
        }

        FS::Close(infile);

        return 0;
    }
}
