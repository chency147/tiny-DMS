#include <stdio.h>
#include <iostream>
#include "SuperBlock.h"
using namespace std;

void initFileSystem(const char* filename);
void commandListener(const char* filename);
int choose();
void init_help();
int main(int argc, char** argv)
{
	initFileSystem(SIMULATED_DISK);//
	if (choose() == 0){
        return 0;
	}
	init_help();
	commandListener(SIMULATED_DISK);//
	return 0;
}
