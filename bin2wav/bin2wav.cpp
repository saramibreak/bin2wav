// bin2wav.cpp : コンソール アプリケーションのエントリ ポイントを定義します。
//
/*
 * This code is released under the Microsoft Public License (MS-PL). See License.txt, below.
 */

#include "stdafx.h"

// 00:00:00 <= MSF <= 89:59:74
// 90:00:00 <= MSF <= 99:59:74 is TODO
int MSFtoLBA(
	unsigned char byMinute,
	unsigned char bySecond,
	unsigned char byFrame
	)
{
	return (byMinute * 60 + bySecond) * 75 + byFrame;
}

bool getSkipSyntax(_TCHAR* str)
{
	const _TCHAR* p1 = _tcsstr(str, _T("CATALOG"));
	const _TCHAR* p2 = _tcsstr(str, _T("CDTEXTFILE"));
	const _TCHAR* p3 = _tcsstr(str, _T("FLAGS"));
	const _TCHAR* p4 = _tcsstr(str, _T("ISRC"));
	const _TCHAR* p5 = _tcsstr(str, _T("PERFORMER"));
	const _TCHAR* p6 = _tcsstr(str, _T("POSTGAP"));
	const _TCHAR* p7 = _tcsstr(str, _T("PREGAP"));
	const _TCHAR* p8 = _tcsstr(str, _T("REM"));
	const _TCHAR* p9 = _tcsstr(str, _T("SONGWRITER"));
	const _TCHAR* p10 = _tcsstr(str, _T("TITLE"));
	if (p1 || p2 || p3 || p4 || p5 || 
		p6 || p7 || p8 || p9 || p10) {
		return true;
	}
	return false;
}

bool getFileName(_TCHAR* str, _TCHAR* fname)
{
	const _TCHAR* p3 = _tcsstr(str, _T("\""));
	if (!p3) {
		return false;
	}
	const _TCHAR* p4 = _tcsstr(p3 + 1, _T("\""));
	if (!p4) {
		return false;
	}
	size_t flen = (p4 - p3) / sizeof(_TCHAR) - 1;
	_tcsncpy(fname, p3 + 1, flen);
	return true;
}

bool getFileSyntax(_TCHAR* str, _TCHAR* fname)
{
	const _TCHAR* p1 = _tcsstr(str, _T("FILE"));
	const _TCHAR* p2 = _tcsstr(str, _T("BINARY"));
	if (!p1 || !p2) {
		return false;
	}
	return getFileName(str, fname);
}

bool getTrackAudioSyntax(_TCHAR* str, unsigned char* track)
{
	const _TCHAR* pTrack = _tcsstr(str, _T("TRACK"));
	if (!pTrack) {
		return false;
	}
	const _TCHAR* pAudio = _tcsstr(str, _T("AUDIO"));
	if (!pAudio) {
		return false;
	}
	// get track num
	char tmp[3] = { 0 };
	tmp[0] = *(pTrack + 6);
	tmp[1] = *(pTrack + 7);
	char* endptr = NULL;
	*track = (unsigned char)strtol(tmp, &endptr, 10);

	return true;
}

bool getIndexSyntax(_TCHAR* str, int* lba, int* idx)
{
	const _TCHAR* pIndex = _tcsstr(str, _T("INDEX"));
	if (!pIndex) {
		return false;
	}
	// get MSF
	char i[3] = { 0 };
	i[0] = *(pIndex + 6);
	i[1] = *(pIndex + 7);
	char* endptr = NULL;
	*idx = (unsigned char)strtol(i, &endptr, 10);

	// get MSF
	char m[3] = { 0 };
	m[0] = *(pIndex + 9);
	m[1] = *(pIndex + 10);
	unsigned char min = (unsigned char)strtol(m, &endptr, 10);

	char s[3] = { 0 };
	s[0] = *(pIndex + 12);
	s[1] = *(pIndex + 13);
	unsigned char sec = (unsigned char)strtol(s, &endptr, 10);

	char f[3] = { 0 };
	f[0] = *(pIndex + 15);
	f[1] = *(pIndex + 16);
	unsigned char frm = (unsigned char)strtol(f, &endptr, 10);
	
	*lba = MSFtoLBA(min, sec, frm);
	return true;
}

long GetFileSize(
	long lOffset,
	FILE *fp
	)
{
	long FileSize = 0;
	if (fp != NULL) {
		fseek(fp, 0, SEEK_END);
		FileSize = ftell(fp);
		fseek(fp, lOffset, SEEK_SET);
	}
	return FileSize;
}

bool CreateWavFile(
	_TCHAR* drive,
	_TCHAR* dir,
	_TCHAR* fname,
	long fsize,
	int i,
	FILE* fpBin
	)
{
	_TCHAR fnameWithoutExt[_MAX_FNAME] = { 0 };
	_TCHAR outpath[_MAX_PATH] = { 0 };
	_TCHAR szPlus[12] = { 0 };
	if (i < 100) {
		_stprintf(szPlus, _T(" (Index %02d)"), i);
	}
	_tcsncpy(fnameWithoutExt, fname, _tcslen(fname) - 4);
	if (i < 100) {
		_tcscat(fnameWithoutExt, szPlus);
	}
	_tmakepath(outpath, drive, dir, fnameWithoutExt, _T(".wav"));
	FILE* fpBinW = NULL;
	if (NULL == (fpBinW = _tfopen(outpath, _T("wb")))) {
		_tprintf(_T("Failed to file open. [%s]\n"), fname);
		return false;
	}
	fputs("RIFF", fpBinW);
	long wavsize = fsize + 36;
	fputc(wavsize & 0xff, fpBinW);
	fputc((wavsize >> 8) & 0xff, fpBinW);
	fputc((wavsize >> 16) & 0xff, fpBinW);
	fputc((wavsize >> 24) & 0xff, fpBinW);
	fputs("WAVE", fpBinW);
	fputs("fmt ", fpBinW);
	// fmt chunk num
	fputc(0x10, fpBinW);
	fputc(0, fpBinW);
	fputc(0, fpBinW);
	fputc(0, fpBinW);
	// fmt id
	fputc(1, fpBinW);
	fputc(0, fpBinW);
	// channel num
	fputc(2, fpBinW);
	fputc(0, fpBinW);
	// sampling rate
	fputc(0x44, fpBinW);
	fputc(0xac, fpBinW);
	fputc(0, fpBinW);
	fputc(0, fpBinW);
	// data speed
	fputc(0x10, fpBinW);
	fputc(0xb1, fpBinW);
	fputc(2, fpBinW);
	fputc(0, fpBinW);
	// block size
	fputc(4, fpBinW);
	fputc(0, fpBinW);
	// bit per sample
	fputc(0x10, fpBinW);
	fputc(0, fpBinW);
	fputs("data", fpBinW);
	// data size
	fputc(fsize & 0xff, fpBinW);
	fputc((fsize >> 8) & 0xff, fpBinW);
	fputc((fsize >> 16) & 0xff, fpBinW);
	fputc((fsize >> 24) & 0xff, fpBinW);
	char buf[2352] = { 0 };
	size_t readsize = 0;
	for (;;) {
		readsize += fread(buf, sizeof(char), sizeof(buf), fpBin);
		fwrite(buf, sizeof(char), sizeof(buf), fpBinW);
		if (readsize == (size_t)fsize) {
			break;
		}
	}
	fclose(fpBinW);
	return true;
}

int handleCueFile(FILE* fp, _TCHAR* drive, _TCHAR* dir)
{
	_TCHAR str[256] = { 0 };
	_TCHAR fname[_MAX_FNAME] = { 0 };
	_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp);
	while (!feof(fp)) {
		for (;;) {
			if (getSkipSyntax(str)) {
				if (!_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp)) {
					fclose(fp);
					return EXIT_FAILURE;
				}
			}
			else {
				break;
			}
		}
		if (!getFileSyntax(str, fname)) {
			break;
		}
		if (!_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp)) {
			break;
		}
		for (;;) {
			if (getSkipSyntax(str)) {
				if (!_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp)) {
					fclose(fp);
					return EXIT_FAILURE;
				}
			}
			else {
				break;
			}
		}
		unsigned char track = 0;
		if (!getTrackAudioSyntax(str, &track)) {
			// read next FILE syntax
			bool bNext = false;
			for (;;) {
				if (!_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp)) {
					bNext = true;
					break;
				}
				if (getFileSyntax(str, fname)) {
					bNext = true;
					break;
				}
			}
			if (bNext) {
				continue;
			}
		}
		_TCHAR fullpath[_MAX_PATH] = { 0 };
		_tmakepath(fullpath, drive, dir, fname, NULL);
		FILE* fpBin = NULL;
		if (NULL == (fpBin = _tfopen(fullpath, _T("rb")))) {
			_tprintf(_T("Failed to open. [%s]\n"), fname);
			break;
		}
		long fsize = GetFileSize(0, fpBin);
		int lba[101] = { 0 };
		int idx[101] = { 0 };
		bool endIndex = false;
		int i = 0;
		for (; i < 100; i++) {
			if (!_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp)) {
				break;
			}
			for (;;) {
				if (getSkipSyntax(str)) {
					if (!_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp)) {
						break;
					}
				}
				else {
					break;
				}
			}
			if (!getIndexSyntax(str, &lba[i], &idx[i])) {
				endIndex = true;
				break;
			}
		}
		_tprintf(_T("Creating wav from %s\n"), fname);
		for (int k = 0; k < i; k++) {
			int sectorSize = (lba[k + 1] - lba[k]) * 2352;
			int createSize = 0;
			if (sectorSize > 0) {
				createSize = sectorSize;
			}
			else {
				createSize = fsize + sectorSize;
			}
			CreateWavFile(drive, dir, fname, createSize, idx[k], fpBin);
		}
		fclose(fpBin);
	}
	_tprintf(_T("\n"));
	return EXIT_SUCCESS;
}

int handleGdiFile(FILE* fp, _TCHAR* drive, _TCHAR* dir)
{
	_TCHAR str[256] = { 0 };
	_TCHAR fname[_MAX_FNAME] = { 0 };
	char* endptr = NULL;
	_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp);
	int nTrackNum = strtol(str, &endptr, 10);
	if (*endptr == '\n') {
		// skip track 1, for this is data track
		_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp);
		// skip track 2, for this is warning message
		_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp);
		// skip track 3, for this is data track
		_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp);
		for (int i = 4; i <= nTrackNum; i++) {
			_fgetts(str, sizeof(str) / sizeof(_TCHAR), fp);
			char ctl = str[9];
			if (10 <= nTrackNum) {
				ctl = str[10];
			}
			// '0' is audio
			if (ctl == '0') {
				if (getFileName(str, fname)) {
					_TCHAR fullpath[_MAX_PATH] = { 0 };
					_tmakepath(fullpath, drive, dir, fname, NULL);
					FILE* fpBin = NULL;
					if (NULL == (fpBin = _tfopen(fullpath, _T("rb")))) {
						_tprintf(_T("Failed to open. [%s]\n"), fname);
						break;
					}
					_tprintf(_T("Creating wav from %s\n"), fname);
					CreateWavFile(drive, dir, fname, GetFileSize(0, fpBin), 100, fpBin);
					fclose(fpBin);
				}
			}
		}
	}
	return EXIT_SUCCESS;
}

typedef enum _EXT_TYPE {
	unknown,
	cue,
	gdi
} EXT_TYPE;

EXT_TYPE checkArg(int argc, _TCHAR* pszFullPath, _TCHAR* drive, _TCHAR* dir)
{
	EXT_TYPE bRet = unknown;
	_TCHAR ext[_MAX_EXT] = { 0 };
	if (argc == 2) {
		_tsplitpath(pszFullPath, drive, dir, NULL, ext);
		if (!_tcscmp(ext, _T(".cue"))) {
			bRet = cue;
		}
		else if (!_tcscmp(ext, _T(".gdi"))) {
			bRet = gdi;
		}
	}
	return bRet;
}

int _tmain(int argc, _TCHAR* argv[])
{
	_tprintf(_T("AppVersion\n"));
	_tprintf(_T("\t%s %s\n"), _T(__DATE__), _T(__TIME__));
	_TCHAR drive[_MAX_DRIVE] = { 0 };
	_TCHAR dir[_MAX_DIR] = { 0 };
	EXT_TYPE bType = checkArg(argc, argv[1], drive, dir);
	if (bType == unknown) {
		_tprintf(_T("Usage\n"));
		_tprintf(_T("\tbin2wav <cue file> or <gdi file>\n"));
		return EXIT_FAILURE;
	}
	FILE* fp = NULL;
	if (NULL == (fp = _tfopen(argv[1], _T("r")))) {
		_tprintf(_T("Failed to open [%s]\n"), argv[1]);
		return EXIT_FAILURE;
	}
	int nRet = EXIT_SUCCESS;
	if (bType == cue) {
		nRet = handleCueFile(fp, drive, dir);
	}
	else if (bType == gdi) {
		nRet = handleGdiFile(fp, drive, dir);
	}
	fclose(fp);
	return nRet;
}

