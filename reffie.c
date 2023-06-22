#define _GNU_SOURCE
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>



void docopy(int in, int out, int pct, long step) {

	//multiple of block size 4096, 4*8 = 32kb
	off64_t buffersz = 4096*step;
	char * buffer = malloc(buffersz);


	struct stat fileinfo = {0};
    	fstat(in, &fileinfo);


	ssize_t maxr = (fileinfo.st_size)/buffersz;
	ssize_t finalread = (fileinfo.st_size)%buffersz;


	if (pct < 0) { pct = 0; }
	if (pct > 100) { pct = 100; }

	printf("%d pct reflink\n",pct);

	for(long i=0; i < maxr;i++) {
		off64_t offset = i*buffersz;

		if ((i%100) < pct) {
			copy_file_range(in,&offset,out,&offset,buffersz,0);
		} else {
			//some very not checked code
			lseek(in,offset,SEEK_SET);
			lseek(out,offset,SEEK_SET);
			int numRead = read(in, buffer, buffersz);
			write(out, buffer, numRead);
		}
	}
	if (finalread) {
		off64_t offset = maxr*buffersz;
		copy_file_range(in,&offset,out,&offset,finalread,0);
	}

	free(buffer);

}

int main(int argc, char *argv[]) {
	if (argc > 4) {
		char * in = NULL;
		char * out = NULL;
		int pct = 50;
		long step = 8;

		FILE *fin, *fout;

		for(int i=1;i<argc;i++) {
			if(strcmp(argv[i],"-i") == 0 && i+1<argc) {
				in = argv[i+1];
				i++;
			} else if(strcmp(argv[i],"-o") == 0 && i+1<argc) {
				out = argv[i+1];
				i++;
			} else if(strcmp(argv[i],"-p") == 0 && i+1<argc) {
				pct = atoi(argv[i+1]);
				i++;
			} else if(strcmp(argv[i],"-s") == 0 && i+1<argc) {
				char *r;
				step = strtol(argv[i+1],&r,10);
				i++;
			} else {
				printf("Unkown %s",argv[i]);
			}
		}	
		
		if (in != NULL && out != NULL) {
			int fin,fout;

			fin = open(in,O_RDONLY);
			if (fin == -1) {
				printf("Couldn't open %s",in);
				return -1;
			}

			int openFlags = O_CREAT | O_WRONLY | O_TRUNC;
    			mode_t filePerms = S_IRUSR | S_IXUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH | S_IWOTH;
			
			fout = open(out, openFlags, filePerms);
			if (fout == -1) {
				printf("Couldn't open %s",out);
				return -1;
			}
			
			
			docopy(fin,fout,pct,step);

			close(fin);
			close(fout);

			return 0;
		} else {
			printf("-i in or -o out is null\n");
		}




	} else {
		printf("Min 4 args");
	}
	return -1;
}
