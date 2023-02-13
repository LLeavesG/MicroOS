#pragma once
#pragma warning(disable:4996)
#include "misc.h"

char datestr[16];
char timestr[16];
char mss[4];


void system_log(const char* msg) {
    struct tm* now;
    struct timeb tb;

    ftime(&tb);
    now = localtime(&tb.time);
    sprintf(datestr, "%04d-%02d-%02d", now->tm_year + 1900, now->tm_mon + 1, now->tm_mday);
    sprintf(timestr, "%02d:%02d:%02d", now->tm_hour, now->tm_min, now->tm_sec);
    sprintf(mss, "%03d", tb.millitm);
    printf("%s %s.%s %s", datestr, timestr, mss, msg);
}