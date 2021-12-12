#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

#define TRUE 1
#define FALSE 0
#define UCHAR unsigned char
#define MAX_PATH 256
#define WCHAR char

#ifndef WIN32
#define strtok_s strtok_r
#endif


typedef unsigned long       DWORD;
typedef int                 BOOL;
typedef unsigned char       BYTE;
typedef unsigned short      WORD;

#define MAXHOSTS 16

#define TCHAR char

void QueueMsg(char * Msg, int Len);
int SocketSend(char * Buffer, int len);

void SetPortMonLine(int i, char * Text, int visible, int enabled);

TCHAR Host[MAXHOSTS + 1][100] = {""};
int Port[MAXHOSTS + 1] = {0};
TCHAR UserName[MAXHOSTS + 1][80] = { "" };
TCHAR Password[MAXHOSTS + 1][80] = { "" };
TCHAR MonParams[MAXHOSTS + 1][80] = { "" };
int ListenPort = 8015;


// Keyboard Scrollback Buffer

int MonData = 0;
int SlowTimer = 0;
int CurrentHost = 0;

int portmask = 4;
int mtxparam = 1;
int mcomparam = 1;
int monUI = 0;

BOOL Bells = TRUE;
BOOL StripLF = FALSE;
BOOL LogMonitor = FALSE;
BOOL LogOutput = FALSE;
BOOL SendDisconnected = TRUE;
BOOL MonitorNODES = FALSE;
BOOL MonitorColour = TRUE;
BOOL ChatMode = TRUE;
int MonPorts = 1;
BOOL ListenOn = FALSE;

time_t LastWrite = 0xffffffff;
int AlertInterval = 300;
BOOL AlertBeep = TRUE;
int AlertFreq = 600;
int AlertDuration = 250;
TCHAR AlertFileName[256] = { 0 };

BOOL UseKeywords = TRUE;

char KeyWordsName[MAX_PATH] = "Keywords.sys";
char ** KeyWords = NULL;
int NumberofKeyWords = 0;



// YAPP stuff

#define SOH 1
#define STX 2
#define ETX 3
#define EOT 4
#define ENQ 5
#define ACK 6
#define DLE	0x10
#define NAK 0x15
#define CAN 0x18

#define YAPPTX	32768					// Sending YAPP file

int MaxRXSize = 100000;
char BaseDir[256] = "";

unsigned char InputBuffer[1024];

unsigned char InputMode;			// Line by Line or Binary or YAPP
UCHAR InputMode;				// Normal or YAPP
char  YAPPPath[MAX_PATH] = "";	// Path for saving YAPP Files

int paclen = 128;

int InputLen;				// Data we have already = Offset of end of an incomplete packet;

unsigned char * MailBuffer;			// Yapp Message being received
int MailBufferSize;
int YAPPLen;				// Bytes sent/received of YAPP Message
long YAPPDate;				// Date for received file - if set enables YAPPC
char ARQFilename[200];		// Filename from YAPP Header

unsigned char SavedData[8192];		// Max receive is 4096 is should never get more that 8k
int SaveLen = 0;

void YAPPSendData();


char * strlop(char * buf, char delim)
{
	// Terminate buf at delim, and return rest of string

	char * ptr = strchr(buf, delim);

	if (ptr == NULL) return NULL;

	*(ptr)++ = 0;

	return ptr;
}

void EncodeSettingsLine(int n, char * String)
{
	sprintf(String, "%s|%d|%s|%s|%s", Host[n], Port[n], UserName[n], Password[n], MonParams[n]);
	return;
}

void DecodeSettingsLine(int n, char * String)
{
	char * Param = strdup(String);
	char * Rest;
	char * Save = Param;			// for Free

	Rest = strlop(Param, '|');

	if (Rest == NULL)
		return;

	strcpy(Host[n], Param);
	Param = Rest;

	Rest = strlop(Param, '|');
	Port[n] = atoi(Param);
	Param = Rest;

	Rest = strlop(Param, '|');
	strcpy(UserName[n], Param);
	Param = Rest;

	Rest = strlop(Param, '|');
	strcpy(Password[n], Param);

	strcpy(MonParams[n], Rest);


	free(Save);
	return;
}


BOOL CheckKeyWord(char * Word, char * Msg)
{
	char * ptr1 = Msg, *ptr2;
	int len = strlen(Word);

	while (*ptr1)					// Stop at end
	{
		ptr2 = strstr(ptr1, Word);

		if (ptr2 == NULL)
			return FALSE;				// OK

		// Only bad if it ia not part of a longer word

		if ((ptr2 == Msg) || !(isalpha(*(ptr2 - 1))))	// No alpha before
			if (!(isalpha(*(ptr2 + len))))			// No alpha after
				return TRUE;					// Bad word

		// Keep searching

		ptr1 = ptr2 + len;
	}

	return FALSE;					// OK
}

BOOL CheckKeyWords(char * Msg, int len)
{
	char dupMsg[2048];
	int i;

	if (UseKeywords == 0 || NumberofKeyWords == 0)
		return FALSE;

	memcpy(dupMsg, Msg, len);
	dupMsg[len] = 0;
	//_strlwr(dupMsg);

	for (i = 1; i <= NumberofKeyWords; i++)
	{
		if (CheckKeyWord(KeyWords[i], dupMsg))
		{
//			Beep(660, 250);
			return TRUE;			// Alert
		}
	}

	return FALSE;					// OK

}




void ProcessReceivedData(unsigned char * Buffer, int len)
{
	int MonLen = 0, err = 0, newlen;
	unsigned char * ptr;
	unsigned char * Buffptr;
	unsigned char * FEptr = 0;

	if (InputMode == 'Y')			// Yapp
	{
		ProcessYAPPMessage(Buffer, len);
		return;
	}

	//	mbstowcs(Buffer, BufferB, len);

		// Look for MON delimiters (FF/FE)

	Buffptr = Buffer;

	if (MonData)
	{
		// Already in MON State

		FEptr = memchr(Buffptr, 0xfe, len);

		if (!FEptr)
		{
			// no FE - so send all to monitor

			WritetoMonWindow(Buffer, len);
			return;
		}

		MonData = FALSE;

		MonLen = FEptr - Buffptr;		// Mon Data, Excluding the FE

		WritetoMonWindow(Buffptr, MonLen);

		Buffptr = ++FEptr;				// Char following FE

		if (++MonLen < len)
		{
			len -= MonLen;
			goto MonLoop;				// See if next in MON or Data
		}

		// Nothing Left

		return;
	}

MonLoop:

	if (ptr = memchr(Buffptr, 0xff, len))
	{
		// Buffer contains Mon Data

		if (ptr > Buffptr)
		{
			// Some Normal Data before the FF

			int NormLen = ptr - Buffptr;				// Before the FF

			if (NormLen == 1 && Buffptr[0] == 0)
			{
				// Keepalive
			}

			else
			{
				CheckKeyWords(Buffptr, NormLen);

				WritetoOutputWindow(Buffptr, NormLen);
			}

			len -= NormLen;
			Buffptr = ptr;
			goto MonLoop;
		}

		if (ptr[1] == 0xff)
		{
			// Port Definition String

			int NumberofPorts = atoi(&ptr[2]);
			char *p, *Context;
			int i = 1;
			TCHAR msg[80];
			TCHAR ID[80];
			int portnum;

			// Remove old menu
			
			for (i = 0; i < 32; i++)
			{
				SetPortMonLine(i, "", 0, 0);
			}

			p = strtok_s(&ptr[2], "|", &Context);

			while (NumberofPorts--)
			{
				p = strtok_s(NULL, "|", &Context);
				if (p == NULL)
					break;

				portnum = atoi(p);

				sprintf(msg, "Port %s", p);

				if (portmask & (1 << (portnum - 1)))
					SetPortMonLine(portnum, msg, 1, 1);
				else
					SetPortMonLine(portnum, msg, 1, 0);
			}
			return;
		}


		MonData = 1;

		FEptr = memchr(Buffptr, 0xfe, len);

		if (FEptr)
		{
			MonData = 0;

			MonLen = FEptr + 1 - Buffptr;				// MonLen includes FF and FE

			WritetoMonWindow(Buffptr + 1, MonLen - 2);

			len -= MonLen;
			Buffptr += MonLen;							// Char Following FE

			if (len <= 0)
			{
				return;
			}
			goto MonLoop;
		}
		else
		{
			// No FE, so rest of buffer is MON Data

			WritetoMonWindow(Buffptr + 1, MonLen - 1);
			return;
		}
	}

	// No FF, so must be session data

	if (InputMode == 'Y')			// Yapp
	{
		ProcessYAPPMessage(Buffer, len);
		return;
	}


	if (len == 1 && Buffptr[0] == 0)
		return;							// Keepalive

	// Could be a YAPP Header


	if (len == 2 && Buffptr[0] == ENQ && Buffptr[1] == 1)		// YAPP Send_Init
	{
		unsigned char YAPPRR[2];
		char Mess[64];

		// Turn off monitoring

		len = sprintf(Mess, "\\\\\\\\%x %x %x %x %x %x %x %x\r", 0, 0, 0, 0, 0, 0, 0, 0);
		SendMsg(Mess, len);
		InputMode = 'Y';

		YAPPRR[0] = ACK;
		YAPPRR[1] = 1;

		SocketFlush();			// To give Monitor Msg time to be sent
		Sleep(1000);
		QueueMsg(YAPPRR, 2);

		return;
	}

	CheckKeyWords(Buffptr, len);

	WritetoOutputWindow(Buffptr, len);

	SlowTimer = 0;
	return;
}

int SendMsg(TCHAR * Buffer, int len)
{
	return SocketSend(Buffer, len);
}

void SendTraceOptions()
{
	char Buffer[80];
	int Len = sprintf(Buffer, "\\\\\\\\%x %x %x %x %x %x %x %x\r", portmask, mtxparam, mcomparam, MonitorNODES, MonitorColour, monUI, 1, 1);
	strcpy(&MonParams[CurrentHost][0], &Buffer[4]);
	SaveSettings();
	SocketFlush();
	SocketSend(Buffer, Len);
	SocketFlush();
}





void YAPPSendData();

void QueueMsg(char * Msg, int len)
{
	int Sent = SendMsg(Msg, len);


	if (Sent != len)
		Sent = 0;
}

int InnerProcessYAPPMessage(UCHAR * Msg, int Len);

BOOL ProcessYAPPMessage(UCHAR * Msg, int Len)
{
	// may have saved data

	memcpy(&SavedData[SaveLen], Msg, Len);

	SaveLen += Len;

	while (SaveLen && InputMode == 'Y')
	{
		int Used = InnerProcessYAPPMessage(SavedData, SaveLen);

		if (Used == 0)
			return 0;			// Waiting for more

		SaveLen -= Used;

		if (SaveLen)
			memmove(SavedData, &SavedData[Used], SaveLen);
	}
	return 0;
}

int InnerProcessYAPPMessage(UCHAR * Msg, int Len)
{
	int pktLen = Msg[1];
	char Reply[2] = { ACK };
	int NameLen, SizeLen, OptLen;
	char * ptr;
	int FileSize;
	WCHAR MsgFile[MAX_PATH];
	FILE * hFile;
	char Mess[255];
	int len;
	char BufferW[2000];
	struct stat STAT;



	switch (Msg[0])
	{
	case ENQ: // YAPP Send_Init

		// Shouldn't occur in session. Reset state and process

		if (MailBuffer)
		{
			free(MailBuffer);
			MailBufferSize = 0;
			MailBuffer = 0;
		}

		Mess[0] = ACK;
		Mess[1] = 1;

		InputMode = 'Y';
		QueueMsg(Mess, 2);

		// Turn off monitoring

		Sleep(1000);				// To give YAPP Msg tome to be sent

		len = sprintf(Mess, "\\\\\\\\%x %x %x %x %x %x %x %x\r", 0, 0, 0, 0, 0, 0, 0, 0);
		SendMsg(Mess, len);


		return Len;

	case SOH:

		// HD Send_Hdr     SOH  len  (Filename)  NUL  (File Size in ASCII)  NUL (Opt) 

		// YAPPC has date/time in dos format

		NameLen = strlen(&Msg[2]);
		strcpy(ARQFilename, &Msg[2]);

		ptr = &Msg[3 + NameLen];
		SizeLen = strlen(ptr);
		FileSize = atoi(ptr);

		OptLen = pktLen - (NameLen + SizeLen + 2);

		YAPPDate = 0;

		if (OptLen >= 8)		// We have a Date/Time for YAPPC
		{
			ptr = ptr + SizeLen + 1;
			YAPPDate = strtol(ptr, NULL, 16);
		}

		// Check Size

		if (FileSize > MaxRXSize)
		{
			Mess[0] = NAK;
			Mess[1] = sprintf(&Mess[2], "File %s size %d larger than limit %d\r", ARQFilename, FileSize, MaxRXSize);
			Sleep(1000);				// To give YAPP Msg tome to be sent
			QueueMsg(Mess, Mess[1] + 2);

			len = sprintf(BufferW, "YAPP File %s size %d larger than limit %d\r", ARQFilename, FileSize, MaxRXSize);
			WritetoOutputWindow(BufferW, len);

			InputMode = 0;
			SendTraceOptions();

			return Len;
		}

		// Make sure file does not exist

		// Path is Wide String, ARQFN normal

		sprintf(MsgFile, "%s/%s", YAPPPath, ARQFilename);

		if (stat(MsgFile, &STAT) == 0)
		{
			FileSize = STAT.st_size;

			Mess[0] = NAK;
			Mess[1] = sprintf(&Mess[2], "%s", "File Already Exists");
			Sleep(1000);				// To give YAPP Msg time to be sent
			QueueMsg(Mess, Mess[1] + 2);
			len = sprintf(BufferW, "YAPP File Receive Failed - %s already exists\r", MsgFile);
			WritetoOutputWindow(BufferW, len);

			InputMode = 0;
			SendTraceOptions();

			return Len;
		}


		MailBufferSize = FileSize;
		MailBuffer = malloc(FileSize);
		YAPPLen = 0;

		if (YAPPDate)			// If present use YAPPC
			Reply[1] = ACK;			//Receive_TPK
		else
			Reply[1] = 2;			//Rcv_File

		QueueMsg(Reply, 2);

		len = sprintf(BufferW, "YAPP Receving File %s size %d\r", ARQFilename, FileSize);
		WritetoOutputWindow(BufferW, len);

		return Len;

	case STX:

		// Data Packet

		// Check we have it all

		if (YAPPDate)			// If present use YAPPC so have checksum
		{
			if (pktLen > (Len - 3))		// -2 for header and checksum
				return 0;				// Wait for rest
		}
		else
		{
			if (pktLen > (Len - 2))		// -2 for header
				return 0;				// Wait for rest
		}

		// Save data and remove from buffer

		// if YAPPC check checksum

		if (YAPPDate)
		{
			UCHAR Sum = 0;
			int i;
			UCHAR * uptr = &Msg[2];

			i = pktLen;

			while (i--)
				Sum += *(uptr++);

			if (Sum != *uptr)
			{
				// Checksum Error

				Mess[0] = CAN;
				Mess[1] = sprintf(&Mess[2], "YAPPC Checksum Error");
				QueueMsg(Mess, Mess[1] + 2);

				len = sprintf(BufferW, "YAPPC Checksum Error on file %s\r", MsgFile);
				WritetoOutputWindow(BufferW, len);

				InputMode = 0;
				SendTraceOptions();
				return Len;
			}
		}

		if ((YAPPLen)+pktLen > MailBufferSize)
		{
			// Too Big ??

			Mess[0] = CAN;
			Mess[1] = sprintf(&Mess[2], "YAPP Too much data received");
			QueueMsg(Mess, Mess[1] + 2);

			len = sprintf(BufferW, "YAPP Too much data received on file %s\r", MsgFile);
			WritetoOutputWindow(BufferW, len);

			InputMode = 0;
			SendTraceOptions();
			return Len;
		}


		memcpy(&MailBuffer[YAPPLen], &Msg[2], pktLen);
		YAPPLen += pktLen;

		if (YAPPDate)
			++pktLen;				// Add Checksum

		return pktLen + 2;

	case ETX:

		// End Data

		if (YAPPLen == MailBufferSize)
		{
			// All received

			int ret;
			DWORD Written = 0;
			char FN[MAX_PATH];

			sprintf(MsgFile, "%s/%s", YAPPPath, ARQFilename);

			hFile = fopen(MsgFile, "wb");

			if (hFile)
			{
				Written = fwrite(MailBuffer, 1, YAPPLen, hFile);
				fclose(hFile);

				if (YAPPDate)
				{
//					struct tm TM;
//					struct timeval times[2];
					/*
										The MS-DOS date. The date is a packed value with the following format.

										cant use DosDateTimeToFileTime on Linux

										Bits	Description
										0-4	Day of the month (1–31)
										5-8	Month (1 = January, 2 = February, and so on)
										9-15	Year offset from 1980 (add 1980 to get actual year)
										wFatTime
										The MS-DOS time. The time is a packed value with the following format.
										Bits	Description
										0-4	Second divided by 2
										5-10	Minute (0–59)
										11-15	Hour (0–23 on a 24-hour clock)
					*/
					/*
				
					memset(&TM, 0, sizeof(TM));

					TM.tm_sec = (YAPPDate & 0x1f) << 1;
					TM.tm_min = ((YAPPDate >> 5) & 0x3f);
					TM.tm_hour = ((YAPPDate >> 11) & 0x1f);

					TM.tm_mday = ((YAPPDate >> 16) & 0x1f);
					TM.tm_mon = ((YAPPDate >> 21) & 0xf) - 1;
					TM.tm_year = ((YAPPDate >> 25) & 0x7f) + 80;

					Debugprintf("%d %d %d %d %d %d", TM.tm_year, TM.tm_mon, TM.tm_mday, TM.tm_hour, TM.tm_min, TM.tm_sec);

					times[0].tv_sec = times[1].tv_sec = mktime(&TM);
					times[0].tv_usec = times[1].tv_usec = 0;
						*/
				}
			}


			free(MailBuffer);
			MailBufferSize = 0;
			MailBuffer = 0;

			if (Written != YAPPLen)
			{
				Mess[0] = CAN;
				Mess[1] = sprintf(&Mess[2], "Failed to save YAPP File");
				QueueMsg(Mess, Mess[1] + 2);

				len = sprintf(BufferW, "Failed to save YAPP File %s\r", MsgFile);
				WritetoOutputWindow(BufferW, len);

				InputMode = 0;
				SendTraceOptions();
			}
		}

		Reply[1] = 3;		//Ack_EOF
		QueueMsg(Reply, 2);

		len = sprintf(BufferW, "Reception of file %s complete\r", MsgFile);
		WritetoOutputWindow(BufferW, len);



		return Len;

	case EOT:

		// End Session

		Reply[1] = 4;		// Ack_EOT
		QueueMsg(Reply, 2);
		SocketFlush();
		InputMode = 0;

		SendTraceOptions();
		return Len;

	case CAN:

		// Abort

		Mess[0] = ACK;
		Mess[1] = 5;			// CAN Ack
		QueueMsg(Mess, 2);

		if (MailBuffer)
		{
			free(MailBuffer);
			MailBufferSize = 0;
			MailBuffer = 0;
		}

		// May have an error message

		len = Msg[1];

		if (len)
		{
			len = sprintf(BufferW, "YAPP Transfer cancelled - %s\r", &Msg[2]);
		}
		else
			len = sprintf(Mess, "YAPP Transfer cancelled\r");

		InputMode = 0;
		SendTraceOptions();

		return Len;

	case ACK:

		switch (Msg[1])
		{
			char * ptr;

		case 1:					// Rcv_Rdy

			// HD Send_Hdr     SOH  len  (Filename)  NUL  (File Size in ASCII)  NUL (Opt)

			// Remote only needs filename so remove path

			ptr = ARQFilename;

			while (strchr(ptr, '/'))
				ptr = strchr(ptr, '/') + 1;

			len = strlen(ptr) + 3;

			strcpy(&Mess[2], ptr);
			len += sprintf(&Mess[len], "%d", MailBufferSize);
			len++;					// include null
//			len += sprintf(&Mess[len], "%8X", YAPPDate);
//			len++;					// include null
			Mess[0] = SOH;
			Mess[1] = len - 2;

			QueueMsg(Mess, len);

			return Len;

		case 2:

			YAPPDate = 0;				// Switch to Normal (No Checksum) Mode

		case 6:							// Send using YAPPC

			//	Start sending message

			YAPPSendData();
			return Len;

		case 3:

			// ACK EOF - Send EOT

			Mess[0] = EOT;
			Mess[1] = 1;
			QueueMsg(Mess, 2);

			return Len;

		case 4:

			// ACK EOT

			InputMode = 0;
			SendTraceOptions();

			len = sprintf(BufferW, "File transfer complete\r");
			WritetoOutputWindow(BufferW, len);
	

			return Len;

		default:
			return Len;

		}

	case NAK:

		// Either Reject or Restart

		// RE Resume       NAK  len  R  NULL  (File size in ASCII)  NULL

		if (Len > 2 && Msg[2] == 'R' && Msg[3] == 0)
		{
			int posn = atoi(&Msg[4]);

			YAPPLen += posn;
			MailBufferSize -= posn;

			YAPPSendData();
			return Len;

		}

		// May have an error message

		len = Msg[1];

		if (len)
		{
			char ws[256];

			Msg[len + 2] = 0;

			strcpy(ws, &Msg[2]);

			len = sprintf(BufferW, "File rejected - %s\r", ws);
		}
		else
			len = sprintf(BufferW, "File rejected\r");

		WritetoOutputWindow(BufferW, len);
	

		InputMode = 0;
		SendTraceOptions();

		return Len;

	}

	len = sprintf(BufferW, "Unexpected message during YAPP Transfer. Transfer canncelled\r");
	WritetoOutputWindow(BufferW, len);
	
	InputMode = 0;
	SendTraceOptions();

	return Len;

}

void YAPPSendFile(WCHAR * FN)
{
	int FileSize = 0;
	char MsgFile[MAX_PATH];
	FILE * hFile;
	struct stat STAT;
	char Buffer[2000];
	int Len;

	strcpy(MsgFile, FN);

	if (MsgFile == NULL)
	{
		Len = sprintf(Buffer, "Filename missing\r");
		WritetoOutputWindow(Buffer, Len);
	
		SendTraceOptions();

		return;
	}

	if (stat(MsgFile, &STAT) != -1)
	{
		FileSize = STAT.st_size;

		hFile = fopen(MsgFile, "rb");

		if (hFile)
		{
			char Mess[255];
			time_t UnixTime = STAT.st_mtime;

//			FILETIME ft;
			long long ll;
//			SYSTEMTIME st;
			WORD FatDate;
			WORD FatTime;
			struct tm TM;

			strcpy(ARQFilename, MsgFile);
			MailBuffer = malloc(FileSize);
			MailBufferSize = FileSize;
			YAPPLen = 0;
			fread(MailBuffer, 1, FileSize, hFile);

			// Get Date and Time for YAPPC Mode

/*					The MS-DOS date. The date is a packed value with the following format.

					cant use DosDateTimeToFileTime on Linux

					Bits	Description
					0-4	Day of the month (1–31)
					5-8	Month (1 = January, 2 = February, and so on)
					9-15	Year offset from 1980 (add 1980 to get actual year)
					wFatTime
					The MS-DOS time. The time is a packed value with the following format.
					Bits	Description
					0-4	Second divided by 2
					5-10	Minute (0–59)
					11-15	Hour (0–23 on a 24-hour clock)

					memset(&TM, 0, sizeof(TM));

					TM.tm_sec = (YAPPDate & 0x1f) << 1;
					TM.tm_min = ((YAPPDate >> 5) & 0x3f);
					TM.tm_hour =  ((YAPPDate >> 11) & 0x1f);

					TM.tm_mday =  ((YAPPDate >> 16) & 0x1f);
					TM.tm_mon =  ((YAPPDate >> 21) & 0xf) - 1;
					TM.tm_year = ((YAPPDate >> 25) & 0x7f) + 80;


// Note that LONGLONG is a 64-bit value

			ll = Int32x32To64(UnixTime, 10000000) + 116444736000000000;
			ft.dwLowDateTime = (DWORD)ll;
			ll >>= 32;
			ft.dwHighDateTime = (DWORD)ll;

			FileTimeToSystemTime(&ft, &st);
			FileTimeToDosDateTime(&ft, &FatDate, &FatTime);

			YAPPDate = (FatDate << 16) + FatTime;

			memset(&TM, 0, sizeof(TM));

			TM.tm_sec = (YAPPDate & 0x1f) << 1;
			TM.tm_min = ((YAPPDate >> 5) & 0x3f);
			TM.tm_hour = ((YAPPDate >> 11) & 0x1f);

			TM.tm_mday = ((YAPPDate >> 16) & 0x1f);
			TM.tm_mon = ((YAPPDate >> 21) & 0xf) - 1;
			TM.tm_year = ((YAPPDate >> 25) & 0x7f) + 80;
*/
			fclose(hFile);

			Mess[0] = ENQ;
			Mess[1] = 1;

			QueueMsg(Mess, 2);
			InputMode = 'Y';

			return;
		}
	}

	Len = sprintf(Buffer, "File %s not found\r", FN);
	WritetoOutputWindow(Buffer, Len);

}

void YAPPSendData()
{
	char Mess[258];

	while (1)
	{
		int Left = MailBufferSize;

		if (Left == 0)
		{
			// Finished - send End Data

			Mess[0] = ETX;
			Mess[1] = 1;

			QueueMsg(Mess, 2);

			break;
		}

		if (Left > paclen - 3)		// two bytes header and possible checksum
			Left = paclen - 3;

		memcpy(&Mess[2], &MailBuffer[YAPPLen], Left);

		YAPPLen += Left;
		MailBufferSize -= Left;

		// if YAPPC add checksum

		if (YAPPDate)
		{
			UCHAR Sum = 0;
			int i;
			UCHAR * uptr = (UCHAR *)&Mess[2];

			i = Left;

			while (i--)
				Sum += *(uptr++);

			*(uptr) = Sum;

			Mess[0] = STX;
			Mess[1] = Left;

			QueueMsg(Mess, Left + 3);
		}
		else
		{
			Mess[0] = STX;
			Mess[1] = Left;

			QueueMsg(Mess, Left + 2);
		}
	}
}

