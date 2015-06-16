#include "errstr.h"

// Based on http://3dbrew.org/wiki/Error_codes
// (\d+)\s(.+)
// case \1\:\n\t*levelStr = "\2";\n\tbreak;
void errstr(uint32_t errCode, const char** descStr, const char** summaryStr, const char** moduleStr, const char** levelStr)
{
	if(descStr == NULL || summaryStr == NULL || moduleStr == NULL || levelStr == NULL)
		return;

	uint32_t desc = errCode & 0x3FF;
	uint32_t module = (errCode >> 10) & 0xFF;
	uint32_t summary = (errCode >> 21) & 0x3F;
	uint32_t level = (errCode >> 27) & 0x1F;

	switch(desc)
	{
	case 0:
		*descStr = "Success";
		break;
	case 2:
		*descStr = "Invalid memory permissions (kernel)";
		break;
	case 4:
		*descStr = "Invalid ticket version (AM)";
		break;
	case 5:
		*descStr = "String too big? This error is returned when service name length is greater than 8. (srv)";
		break;
	case 6:
		*descStr = "Access denied? This error is returned when you request a service that you don't have access to. (srv)";
		break;
	case 7:
		*descStr = "String too small? This error is returned when service name contains an unexpected null byte. (srv)";
		break;
	case 8:
		*descStr = "Camera already in use/busy (qtm).";
		break;
	case 10:
		*descStr = "Not enough memory (os)";
		break;
	case 26:
		*descStr = "Session closed by remote (os)";
		break;
	case 37:
		*descStr = "Invalid NCCH? (AM)";
		break;
	case 39:
		*descStr = "Invalid title version (AM)";
		break;
	case 43:
		*descStr = "Database doesn't exist/failed to open (AM)";
		break;
	case 44:
		*descStr = "Trying to uninstall system-app (AM)";
		break;
	case 101:
		*descStr = "Archive not mounted/mount-point not found (fs)";
		break;
	case 105:
		*descStr = "Request timed out (http)";
		break;
	case 120:
		*descStr = "Title/object not found? (fs)";
		break;
	case 141:
		*descStr = "Gamecard not inserted? (fs)";
		break;
	case 230:
		*descStr = "Invalid open-flags / permissions? (fs)";
		break;
	case 271:
		*descStr = "Invalid configuration (mvd).";
		break;
	case 391:
		*descStr = "NCCH hash-check failed? (fs)";
		break;
	case 392:
		*descStr = "RSA/AES-MAC verification failed? (fs)";
		break;
	case 395:
		*descStr = "RomFS/Savedata hash-check failed? (fs)";
		break;
	case 630:
		*descStr = "Command not allowed / missing permissions? (fs)";
		break;
	case 702:
		*descStr = "Invalid path? (fs)";
		break;
	case 761:
		*descStr = "Incorrect read-size for ExeFS? (fs)";
		break;
	case 1000:
		*descStr = "Invalid section";
		break;
	case 1001:
		*descStr = "Too large";
		break;
	case 1002:
		*descStr = "Not authorized";
		break;
	case 1003:
		*descStr = "Already done";
		break;
	case 1004:
		*descStr = "Invalid size";
		break;
	case 1005:
		*descStr = "Invalid enum value";
		break;
	case 1006:
		*descStr = "Invalid combination";
		break;
	case 1007:
		*descStr = "No data";
		break;
	case 1008:
		*descStr = "Busy";
		break;
	case 1009:
		*descStr = "Misaligned address";
		break;
	case 1010:
		*descStr = "Misaligned size";
		break;
	case 1011:
		*descStr = "Out of memory";
		break;
	case 1012:
		*descStr = "Not implemented";
		break;
	case 1013:
		*descStr = "Invalid address";
		break;
	case 1014:
		*descStr = "Invalid pointer";
		break;
	case 1015:
		*descStr = "Invalid handle";
		break;
	case 1016:
		*descStr = "Not initialized";
		break;
	case 1017:
		*descStr = "Already initialized";
		break;
	case 1018:
		*descStr = "Not found";
		break;
	case 1019:
		*descStr = "Cancel requested";
		break;
	case 1020:
		*descStr = "Already exists";
		break;
	case 1021:
		*descStr = "Out of range";
		break;
	case 1022:
		*descStr = "Timeout";
		break;
	case 1023:
		*descStr = "Invalid result value";
		break;
	}

	switch(summary)
	{
	case 0:
		*summaryStr = "Success";
		break;
	case 1:
		*summaryStr = "Nothing happened";
		break;
	case 2:
		*summaryStr = "Would block";
		break;
	case 3:
		*summaryStr = "Out of resource";
		break;
	case 4:
		*summaryStr = "Not found";
		break;
	case 5:
		*summaryStr = "Invalid state";
		break;
	case 6:
		*summaryStr = "Not supported";
		break;
	case 7:
		*summaryStr = "Invalid argument";
		break;
	case 8:
		*summaryStr = "Wrong argument";
		break;
	case 9:
		*summaryStr = "Canceled";
		break;
	case 10:
		*summaryStr = "Status changed";
		break;
	case 11:
		*summaryStr = "Internal";
		break;
	case 63:
	default:
		*summaryStr = "Invalid result value";
		break;
	}

	switch(module)
	{
	case 0:
		*moduleStr = "Common";
		break;
	case 1:
		*moduleStr = "Kernel";
		break;
	case 2:
		*moduleStr = "Util";
		break;
	case 3:
		*moduleStr = "File server";
		break;
	case 4:
		*moduleStr = "Loader server";
		break;
	case 5:
		*moduleStr = "TCB";
		break;
	case 6:
		*moduleStr = "OS";
		break;
	case 7:
		*moduleStr = "DBG";
		break;
	case 8:
		*moduleStr = "DMNT";
		break;
	case 9:
		*moduleStr = "PDN";
		break;
	case 10:
		*moduleStr = "GX";
		break;
	case 11:
		*moduleStr = "I2C";
		break;
	case 12:
		*moduleStr = "GPIO";
		break;
	case 13:
		*moduleStr = "DD";
		break;
	case 14:
		*moduleStr = "CODEC";
		break;
	case 15:
		*moduleStr = "SPI";
		break;
	case 16:
		*moduleStr = "PXI";
		break;
	case 17:
		*moduleStr = "FS";
		break;
	case 18:
		*moduleStr = "DI";
		break;
	case 19:
		*moduleStr = "HID";
		break;
	case 20:
		*moduleStr = "CAM";
		break;
	case 21:
		*moduleStr = "PI";
		break;
	case 22:
		*moduleStr = "PM";
		break;
	case 23:
		*moduleStr = "PM_LOW";
		break;
	case 24:
		*moduleStr = "FSI";
		break;
	case 25:
		*moduleStr = "SRV";
		break;
	case 26:
		*moduleStr = "NDM";
		break;
	case 27:
		*moduleStr = "NWM";
		break;
	case 28:
		*moduleStr = "SOC";
		break;
	case 29:
		*moduleStr = "LDR";
		break;
	case 30:
		*moduleStr = "ACC";
		break;
	case 31:
		*moduleStr = "RomFS";
		break;
	case 32:
		*moduleStr = "AM";
		break;
	case 33:
		*moduleStr = "HIO";
		break;
	case 34:
		*moduleStr = "Updater";
		break;
	case 35:
		*moduleStr = "MIC";
		break;
	case 36:
		*moduleStr = "FND";
		break;
	case 37:
		*moduleStr = "MP";
		break;
	case 38:
		*moduleStr = "MPWL";
		break;
	case 39:
		*moduleStr = "AC";
		break;
	case 40:
		*moduleStr = "HTTP";
		break;
	case 41:
		*moduleStr = "DSP";
		break;
	case 42:
		*moduleStr = "SND";
		break;
	case 43:
		*moduleStr = "DLP";
		break;
	case 44:
		*moduleStr = "HIO_LOW";
		break;
	case 45:
		*moduleStr = "CSND";
		break;
	case 46:
		*moduleStr = "SSL";
		break;
	case 47:
		*moduleStr = "AM_LOW";
		break;
	case 48:
		*moduleStr = "NEX";
		break;
	case 49:
		*moduleStr = "Friends";
		break;
	case 50:
		*moduleStr = "RDT";
		break;
	case 51:
		*moduleStr = "Applet";
		break;
	case 52:
		*moduleStr = "NIM";
		break;
	case 53:
		*moduleStr = "PTM";
		break;
	case 54:
		*moduleStr = "MIDI";
		break;
	case 55:
		*moduleStr = "MC";
		break;
	case 56:
		*moduleStr = "SWC";
		break;
	case 57:
		*moduleStr = "FatFS";
		break;
	case 58:
		*moduleStr = "NGC";
		break;
	case 59:
		*moduleStr = "CARD";
		break;
	case 60:
		*moduleStr = "CARDNOR";
		break;
	case 61:
		*moduleStr = "SDMC";
		break;
	case 62:
		*moduleStr = "BOSS";
		break;
	case 63:
		*moduleStr = "DBM";
		break;
	case 64:
		*moduleStr = "Config";
		break;
	case 65:
		*moduleStr = "PS";
		break;
	case 66:
		*moduleStr = "CEC";
		break;
	case 67:
		*moduleStr = "IR";
		break;
	case 68:
		*moduleStr = "UDS";
		break;
	case 69:
		*moduleStr = "PL";
		break;
	case 70:
		*moduleStr = "CUP";
		break;
	case 71:
		*moduleStr = "Gyroscope";
		break;
	case 72:
		*moduleStr = "MCU";
		break;
	case 73:
		*moduleStr = "NS";
		break;
	case 74:
		*moduleStr = "News";
		break;
	case 75:
		*moduleStr = "RO";
		break;
	case 76:
		*moduleStr = "GD";
		break;
	case 77:
		*moduleStr = "Card SPI";
		break;
	case 78:
		*moduleStr = "EC";
		break;
	case 79:
		*moduleStr = "RO";
		break;
	case 80:
		*moduleStr = "Web Browser";
		break;
	case 81:
		*moduleStr = "Test";
		break;
	case 82:
		*moduleStr = "ENC";
		break;
	case 83:
		*moduleStr = "PIA";
		break;
	case 92:
		*moduleStr = "MVD";
		break;
	case 96:
		*moduleStr = "QTM";
		break;
	case 254:
		*moduleStr = "Application";
		break;
	case 255:
	default:
		*moduleStr = "Invalid result value";
		break;
	}

	switch(level)
	{
	case 0:
		*levelStr = "Success";
		break;
	case 1:
		*levelStr = "Info";
		break;
	case 25:
		*levelStr = "Status";
		break;
	case 26:
		*levelStr = "Temporary";
		break;
	case 27:
		*levelStr = "Permanent";
		break;
	case 28:
		*levelStr = "Usage";
		break;
	case 29:
		*levelStr = "Reinitialize";
		break;
	case 30:
		*levelStr = "Reset";
		break;
	case 31:
		*levelStr = "Fatal";
		break;
	}
}
