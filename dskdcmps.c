
//*******************************************************************
//
// program - dskdcmps.c
// purpose - test decompression of dsk files
//
//
// LZW decompression - no warranties expressed or implied
// Note that this code was mainly a test to see if it could
// be done, and to understand how dsk files were compressed
// and if in turn they could be decompressed without creating
// diskettes (the entire reason for dskxtrct). 
//
// Also note that there is some of confusion over the status of the
// patent for the LZW decompression algorithm. You use this code
// at your own risk. 
//           
//*******************************************************************

#define PgmTitle "dskdcmps"
#define PgmVersion "1.0 (08/01/2000)"
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <io.h>
#include <dos.h>
#include <direct.h>

#include "dskdcmps.h"

//*******************************************************************
char *right (char *target, char *source, int len) {
   int i, tpos, slen;
   if (target == NULL)
      return NULL;
   if (source == NULL)
      target[0] = '\0';
   else {
      slen = strlen(source);
      for (i = max(0, slen - len), tpos = 0; i < slen; i++)
         target [tpos++] = source [i];
      target[tpos] = '\0';
      }
   return target;
   }
//*******************************************************************
void trace(int flag) {
	/* flag: 1 = both */
	int slen;
	if (flag > Ctl.TraceLevel)
		return;
	slen = strlen(Ctl.msg);
	if (Ctl.msg[slen-1] == '\n')
		Ctl.msg[slen-1] = '\0';
	puts (Ctl.msg);
	if (Ctl.RptFile != NULL) {
	    slen = strlen(Ctl.msg);
		memcpy (Ctl.msg + slen, "\n\0", 2);
		fputs(Ctl.msg, Ctl.RptFile);
	}
}
//*******************************************************************
void PrintEntry (struct codet * ct, unsigned short tcode) {
	sprintf(Ctl.msg, "Entry code: %4x, usecount: %4x, clink: %4x, clast: %4x, cfirst: %4x\n", 
			tcode, ct->usecount[tcode], ct->charlink[tcode], ct->charlast[tcode],
			ct->charfirst[tcode]);
		trace(1);
	sprintf(Ctl.msg, "older: %4x, newer: %4x, used: %4d, size: %4d\n", 
			ct->older[tcode], ct->newer[tcode], ct->used[tcode], ct->size[tcode]); 
	trace(1);
}
//*******************************************************************
void ValidateLinkChains (struct codet * ct, unsigned short tcode) {
	unsigned short tnewer, tolder;
	tnewer = ct->newer[tcode];
	tolder = ct->older[tcode];
	if (tcode == ct->newest) {
		if (tnewer != 0)
			{sprintf(Ctl.msg, "Newer code not zero. tcode: %4x, newer: %4x, older: %4x\n", 
			tcode, tnewer, ct->older[tnewer]); trace(1);}
	}
	else {
		if (ct->older[tnewer] != tcode)
			{sprintf(Ctl.msg, "Older code not linked. tcode: %4x, newer: %4x, older: %4x\n", 
				tcode, tnewer, ct->older[tnewer]); trace(1);}
	}
	if (tcode == ct->oldest) {
		if (tolder != 0)
			{sprintf(Ctl.msg, "Older code not zero. tcode: %4x, older: %4x, newer: %4x\n", 
			tcode, tolder, ct->newer[tolder]); trace(1);}
	}
	else {
		if (ct->newer[tolder] != tcode)
			{sprintf(Ctl.msg, "Newer code not linked. tcode: %4x, older: %4x, newer: %4x\n", 
			tcode, tolder, ct->newer[tolder]); trace(1);}
	}
}
//*******************************************************************
void ValidateCharLink (struct codet * ct, unsigned short tcode) {
	unsigned short tnewer, tolder;
	tnewer = ct->newer[tcode];
	tolder = ct->older[tcode];
	if (tcode == ct->newest) {
		if (tnewer != 0)
			{sprintf(Ctl.msg, "Newer code not zero. tcode: %4x, newer: %4x, older: %4x\n", 
			tcode, tnewer, ct->older[tnewer]); trace(1);}
	}
	else {
		if (ct->older[tnewer] != tcode)
			{sprintf(Ctl.msg, "Older code not linked. tcode: %4x, newer: %4x, older: %4x\n", 
				tcode, tnewer, ct->older[tnewer]); trace(1);}
	}
	if (tcode == ct->oldest) {
		if (tolder != 0)
			{sprintf(Ctl.msg, "Older code not zero. tcode: %4x, older: %4x, newer: %4x\n", 
			tcode, tolder, ct->newer[tolder]); trace(1);}
	}
	else {
		if (ct->newer[tolder] != tcode)
			{sprintf(Ctl.msg, "Newer code not linked. tcode: %4x, older: %4x, newer: %4x\n", 
			tcode, tolder, ct->newer[tolder]); trace(1);}
	}
}
//*******************************************************************
void OutputStart(struct Cfile * inp, struct Cfile * out) {
	memcpy(out->Pos, inp->Pos, 41);
	--out->Pos[1];
	inp->Pos += 41;
	inp->Offset = inp->Pos - inp->Buf;
	out->Pos += 41;
	out->Offset = out->Pos - out->Buf;
	out->Filesize += 41;
}
//*******************************************************************
void OutputString(struct codet * ct, struct Cfile * out, 
		unsigned short tcode) {
	if (Ctl.Cmp->Filename != NULL) {
		Ctl.Cmp->Pos = Ctl.Cmp->Buf + (out->Pos - out->Buf);
		Ctl.Cmp->Offset = Ctl.Cmp->Pos - Ctl.Cmp->Buf;
		if (0 != memcmp(Ctl.Cmp->Pos, ct->code[tcode], ct->size[tcode])) {
			sprintf(Ctl.msg, "Storage mismatch, code: %4x, inp: %4x, out: %4x, cmp: %4x\n",
		   		tcode, Ctl.Inp->Offset, out->Offset, Ctl.Cmp->Offset);
			trace(1);
			Ctl.MaxInput = Ctl.Inp->Offset;
		}
	}
	memcpy(out->Pos, ct->code[tcode], ct->size[tcode]);
	out->Pos += ct->size[tcode];
	out->Offset = out->Pos - out->Buf;
	out->Filesize += ct->size[tcode];
}
//*******************************************************************
unsigned short GetNextcode (struct codet * ct, struct Cfile * inp) {
	unsigned short code, temp;
	if (ct->j) {
		temp = inp->Pos++[0] << 4;
		ct->hold = inp->Pos++[0];
		code = temp | (ct->hold >> 4);
	}
	else {
		temp = (ct->hold & '\x0F') << 8;
		code = temp | inp->Pos++[0];
		ct->hold = 0;
	}
	inp->Offset = inp->Pos - inp->Buf;
	ct->j = !ct->j;
	return (code);
}
//*******************************************************************
struct codet * DInit () {
	struct codet * ct;
	unsigned short code;
	ct = (struct codet *) malloc(sizeof(struct codet));
    memset(ct, '\0', sizeof(struct codet));
	for (code = 1; code <= 256; code++) {
		ct->newer[code] = 0;
		ct->older[code] = 0;
		ct->charlink[code] = 0;
		ct->charlast[code] = code;
		ct->charfirst[code] = code;
		ct->code[code] = (char *) malloc(1);
		ct->code[code][0] = code-1;
		ct->size[code] = 1;
		ct->used[code] = 0;
		ct->usecount[code] = 1;
	}
	for (code = 257; code <= 4095; code++) {
		ct->newer[code] = code + 1;
		ct->older[code] = code - 1;
		ct->charlink[code] = 0;
		ct->charlast[code] = 0;
		ct->charfirst[code] = 0;
		ct->code[code] = NULL;
		ct->size[code] = 0;
		ct->used[code] = 0;
		ct->usecount[code] = 0;
	}
	ct->newer[4095] = 0;
	ct->older[257] = 0;
	ct->oldest = 257;
	ct->newest = 4095;
	ct->j = 1==1;
	ct->oldcode = 0;
	ct->hold = 0;
	return (ct);
}
//*******************************************************************
void AddMRU (struct codet * ct, unsigned short tcode) {
	if (ct->usecount[tcode] != 0) {
		sprintf(Ctl.msg, "Usecount not zero in AddMRU, code: %4x\n", tcode); trace(1);
		PrintEntry(ct, tcode);}
	ct->newer[ct->newest] = tcode;
	ct->older[tcode] = ct->newest;
	ct->newer[tcode] = 0;
	ct->newest = tcode;
}
//*******************************************************************
void UnlinkCode (struct codet * ct, unsigned short tcode) {
	unsigned short tnewer, tolder;
	ValidateLinkChains(ct, ct->oldest);

	tnewer = ct->newer[tcode];
	tolder = ct->older[tcode];
	if (tcode == ct->newest)
		ct->newest = tolder;
	else
		ct->older[tnewer] = tolder;
	if (tcode == ct->oldest)
		ct->oldest = tnewer;
	else
		ct->newer[tolder] = tnewer;
	ct->older[tcode] = ct->newer[tcode] = 0;
}
//*******************************************************************
unsigned short GetLRU (struct codet * ct) {
	unsigned short tcode, xcode;
	ValidateLinkChains(ct, ct->oldest);
	tcode = ct->oldest;
	if (ct->usecount[tcode] != 0) {
		sprintf(Ctl.msg, "Usecount not zero in GetLRU, code: %4x\n", tcode); trace(1);
		PrintEntry(ct, tcode);}
	xcode = ct->charlink[tcode];
	if (tcode <= 100)
		tcode = tcode;
	UnlinkCode (ct, tcode);

	if (xcode != 0) {
		ct->usecount[xcode] --;
		if (ct->usecount[xcode] == 0) {
			AddMRU (ct, xcode);
		}
	}

	if (ct->code[tcode] != NULL)
		free(ct->code[tcode]);
	ct->used[tcode] ++;
	return (tcode);
}
//*******************************************************************
void ReserveEntry (struct codet * ct, unsigned short tcode) {
	if (ct->usecount[tcode] > 0)
		ct->usecount[tcode] ++;
	else {
		UnlinkCode(ct, tcode);
		ct->usecount[tcode] = 1;
	}
}
//*******************************************************************
void BuildEntry (struct codet * ct, unsigned short newcode) {
	unsigned short lruentry, tcode;
	int codesize, test;
	char work [MAXSTRLEN], work2[MAXSTRLEN], work3[MAXSTRLEN];
	char * codestr;
	lruentry = GetLRU(ct);
	codesize = ct->size[ct->oldcode] + 1;
	codestr = (char *) malloc(codesize);
	memcpy(codestr, ct->code[ct->oldcode], codesize - 1);
	if (newcode != lruentry)
		tcode = newcode;
	else {
		tcode = ct->oldcode;
	}
	memcpy(codestr + codesize - 1, ct->code[tcode], 1);
	ct->code[lruentry] = codestr;
	ct->size[lruentry] = codesize;
	ct->charlink[lruentry] = ct->oldcode;
	ct->charfirst[lruentry] = ct->charfirst[ct->charlink[lruentry]];
	ct->charlast[lruentry] = tcode;
	ReserveEntry(ct, ct->oldcode);
	AddMRU (ct, lruentry);
	strcpy(work, "");
	for (test = 0; test < codesize; test++) {
		sprintf(work2, "%2x\0", codestr[test]);
		right(work3, work2, 2);
		strcat(work, work3);
	}
	sprintf(Ctl.msg, "offset: %4x, newcode: %4x. nused: %4x, lru: %4x, lused: %4x, size: %4d, str: %s\n", 
		Ctl.Inp->Offset, newcode, ct->used[newcode], lruentry, ct->used[lruentry], codesize, work); 
	trace(1);

	if (newcode == lruentry)
			test = 1;
}
//*******************************************************************
void Decompress (struct Cfile * inp, struct Cfile * out) {
	struct codet * ct;
	unsigned short newcode;
	ct = DInit();
	newcode = 1;
	OutputStart(inp, out);
	if (Ctl.MaxInput == 0)
		Ctl.MaxInput = inp->End - inp->Buf;
 
	while (inp->Pos < inp->End && newcode != 0 && inp->Offset < Ctl.MaxInput) {
		newcode = GetNextcode(ct, inp);
		if (newcode > 0) {
			if (ct->oldcode > 0)
				BuildEntry(ct, newcode);
			OutputString(ct, out, newcode);
		}
		ct->oldcode = newcode;
	}
}
//*******************************************************************
int FileWrite (struct Cfile * fptr) {
	int ret, size;
	sprintf(Ctl.msg, "file write: %s, file size %d\n", fptr->Filename, fptr->Filesize); 
	trace(0);
	ret = ((size = write(fptr->Fid, fptr->Buf, fptr->Filesize)) > 0);
	fptr->End = fptr->Buf;
	return (ret);
}
//*******************************************************************
int FileRead (struct Cfile * fptr) {
	int ret, size;
	fptr->Pos = fptr->Buf;
	ret = ((size = read(fptr->Fid, fptr->Buf, fptr->AllocSize)) > 0);
	fptr->End = fptr->Buf + fptr->Filesize;
	sprintf(Ctl.msg, "file read: %s, file size %d\n", fptr->Filename, fptr->Filesize); 
	trace(0);
	return (ret);
}
//*******************************************************************
void FClose (struct Cfile * fileid) {
	if (fileid != NULL) {
		if (fileid->Fid > 0)
			close (fileid->Fid);
		if (fileid->Buf != NULL)
			free (fileid->Buf);
		free (fileid);
	}
}
//*******************************************************************
void FOpenOut (struct Cfile * fptr) {
	if (-1 == (fptr->Fid = open(fptr->Filename, _O_WRONLY | _O_BINARY | O_CREAT, S_IWRITE))) {
		sprintf(Ctl.msg, "Unable to open input file: '%s'\n", fptr->Filename); trace(0);
		exit(34);
	}
	else {
		fptr->Filesize = 0;
		fptr->AllocSize = DSKALLOCSIZE;
		fptr->Buf = (char *) malloc(fptr->AllocSize);
		fptr->Pos = fptr->Buf;
		fptr->End = fptr->Buf + fptr->AllocSize;
		fptr->Offset = 0;
	}
    return;
}
//*******************************************************************
void FOpenInp (struct Cfile * fptr) {
	int opencond = FALSE;
	struct stat StatBuf;
	if (access(fptr->Filename, 0) != 0) {
		sprintf(Ctl.msg, "File not found: '%s'\n", fptr->Filename); trace(0);
	}
	else 
		if (-1 == (fptr->Fid = open(fptr->Filename, _O_RDONLY | _O_BINARY, S_IREAD))) 
			{sprintf(Ctl.msg, "Unable to open input file: '%s'\n", fptr->Filename); trace(0);}
		else
			if (fstat(fptr->Fid, &StatBuf) != 0)
				{sprintf(Ctl.msg, "Unable to get input file size: '%s'\n", fptr->Filename);
				trace(0);}
			else 
				opencond = TRUE;
	if (opencond) {
		fptr->Filesize = StatBuf.st_size;
		fptr->AllocSize = fptr->Filesize;
		fptr->Buf = (char *) malloc(fptr->AllocSize);
		fptr->Pos = fptr->Buf;
		fptr->End = fptr->Pos;
		fptr->Offset = 0;
	}
	else exit(33);
    return;
}
//*******************************************************************
struct Cfile * FInit () {
	struct Cfile * fptr;
	fptr = malloc (sizeof (struct Cfile));
    memset(fptr, '\0', sizeof(struct Cfile));
	fptr->Buf = NULL;
	fptr->Filename = NULL;
	fptr->Filesize = 0;
	fptr->AllocSize = 0;
	fptr->Pos = NULL;
	fptr->End = NULL;
	fptr->Offset = 0;
	return (fptr);
}
//*******************************************************************
void PrintPgmTitle() {
   sprintf(Ctl.msg, "%s - %s\n", PgmTitle, PgmVersion); trace(0);
   sprintf(Ctl.msg, "\n"); trace(0);
   }
//*******************************************************************
void SyntaxMessage ()
    {printf("%s V%s - \n", PgmTitle, PgmVersion);
     printf("       - decompress compressed dsk files\n");
     printf("Syntax: /i:<input> /o:<output> /r:<report> /c:<compare>\n");
     printf("    /i: - input compressed dsk file\n");
     printf("    /o: - output decompressed dsk file\n");
     printf("    /r: - optional output/trace file, screen is default\n");
     printf("    /c: - optional decompressed file to compare to output,\n");
     printf("          used during testing to verify accuracy\n");
     exit(1);     }
//*******************************************************************
int main(int argc, char *argv[]) {
	int argi;
    memset(&Ctl, '\0', sizeof(Ctl));
	Ctl.MaxInput = 0;
	Ctl.TraceLevel = 0;
	Ctl.RptName = NULL;
	Ctl.Inp = FInit('i');
	Ctl.Out = FInit('o');
	Ctl.Cmp = FInit('i');
      /* -------------  parameter handling begin -------------*/
	if (argc < 2) { 
		SyntaxMessage();
		exit (16);
	}
	
	for (argi = 1; argi < argc; argi++) {
		if (strlen(argv[argi]) > 3) {
			if (strnicmp(argv[argi], "/i:", 3) == 0) {
				Ctl.Inp->Filename = argv [argi] + 3;
				continue;
			}
			if (strnicmp(argv[argi], "/o:", 3) == 0) {
				Ctl.Out->Filename = argv [argi] + 3;
				continue;
			}
			if (strnicmp(argv[argi], "/c:", 3) == 0) {
				Ctl.Cmp->Filename = argv [argi] + 3;
				continue;
			}
			if (strnicmp(argv[argi], "/r:", 3) == 0) {
				Ctl.RptName = argv [argi] + 3;
				continue;
			}
		}
	}
	if (Ctl.Inp->Filename == NULL || Ctl.Out->Filename == NULL ) {
		printf("Input and Output file parameters are required\n");
		exit (99);
	}
      /* -------------  parameter handling end ---------------*/

	FOpenInp(Ctl.Inp);
	if (Ctl.Cmp->Filename != NULL)
	    FOpenInp(Ctl.Cmp);
	FOpenOut(Ctl.Out);
	if (Ctl.RptName != NULL) {
		if (NULL == (Ctl.RptFile = fopen(Ctl.RptName, "w"))) {
			fprintf(stderr, "Unable to open report file: '%s'", Ctl.RptName);
			fprintf(stderr, "Error: %s", strerror(errno));
			exit(16);
		}
	}
	else
		fprintf(stderr, "No report file given, output to screen\n");

	sprintf(Ctl.msg, "Begin loading input\n"); trace(0);
	FileRead(Ctl.Inp);
	if (Ctl.Cmp->Filename != NULL)
		FileRead(Ctl.Cmp);
	sprintf(Ctl.msg, "Begin decompressing\n"); trace(0);
	Decompress(Ctl.Inp, Ctl.Out);
	sprintf(Ctl.msg, "Begin cleanup and termination\n"); trace(0);
	FileWrite(Ctl.Out);
	if (Ctl.RptFile != NULL)
		fclose(Ctl.RptFile);
	sprintf(Ctl.msg, "\n"); trace(0);
	sprintf(Ctl.msg, "Input  filesize: %d\n", Ctl.Inp->Filesize); trace(0);
	sprintf(Ctl.msg, "Output filesize: %d\n", Ctl.Out->Filesize); trace(0);
	sprintf(Ctl.msg, "\n"); trace(0);

	FClose(Ctl.Out);
	FClose(Ctl.Inp);
	FClose(Ctl.Cmp);

    return(0);
    }