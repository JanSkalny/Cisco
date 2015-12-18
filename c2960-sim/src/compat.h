#pragma once

#include <Windows.h>

#define _strdup strdup 

void sleep(int n) {
	Sleep(1000*n);
}