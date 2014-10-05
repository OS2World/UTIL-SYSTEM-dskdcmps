// dskdcmps.h

#define uchar unsigned char
#define MAXSTRLEN 4096
#define BOOL int
#define TRUE 1
#define FALSE 0
#define MAXTABLE 4096
struct codet {
	unsigned short hold;
	int j;
	unsigned short oldcode, oldest, newest;
	unsigned short older[MAXTABLE], newer[MAXTABLE];
	unsigned short charlink[MAXTABLE], charlast[MAXTABLE], charfirst[MAXTABLE];
	int size[MAXTABLE], used[MAXTABLE], usecount[MAXTABLE];
	char * code[MAXTABLE];


};

#define CFILEBUFLEN 4096
#define DSKALLOCSIZE 2*1024*1024
struct Cfile {
	int Fid;
	char * Filename;
	uchar * Buf;           // abs addr of beginning of buffer
	uchar * Pos;           // abs addr of current position in buffer being worked on
	uchar * End;           // abs addr of byte just after end of buffer 
	int    AllocSize;      // allocated dsk buffer size (in:filesize, out:2 meg)
	int    Offset;         // relative offset into buffer
	int    Filesize;       // data size read or data size built so far for output
};

struct {
    char * RptName;
    FILE * RptFile;
	struct Cfile * Inp;
	struct Cfile * Out;
	struct Cfile * Cmp;
	char msg[MAXSTRLEN];
	int MaxInput, TraceLevel;
} Ctl;


// end dskdcmps.h
