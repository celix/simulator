/*********************************************************************************
*  Copyright (c) 2010-2011, Elliott Cooper-Balis
*                             Paul Rosenfeld
*                             Bruce Jacob
*                             University of Maryland 
*                             dramninjas [at] gmail [dot] com
*  All rights reserved.
*  
*  Redistribution and use in source and binary forms, with or without
*  modification, are permitted provided that the following conditions are met:
*  
*     * Redistributions of source code must retain the above copyright notice,
*        this list of conditions and the following disclaimer.
*  
*     * Redistributions in binary form must reproduce the above copyright notice,
*        this list of conditions and the following disclaimer in the documentation
*        and/or other materials provided with the distribution.
*  
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
*  ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
*  WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
*  DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
*  FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
*  DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
*  SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
*  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
*  OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*********************************************************************************/



//TraceBasedSim.cpp
//
//File to run a trace-based simulation
//

#include <getopt.h>
#include "Simulator.h"
#include "SimulatorIO.h"

using namespace DRAMSim;
using namespace std;

int main(int argc, char **argv)
{

	SimulatorIO *simIO = new SimulatorIO();

	//getopt stuff
	while (1)
	{
		static struct option long_options[] =
		{
			{"deviceini", required_argument, 0, 'd'},
			{"tracefile", required_argument, 0, 't'},
			{"systemini", required_argument, 0, 's'},

			{"pwd", required_argument, 0, 'p'},
			{"numcycles",  required_argument,	0, 'c'},
			{"option",  required_argument,	0, 'o'},
			{"quiet",  no_argument, 0, 'q'},
			{"help", no_argument, 0, 'h'},
			{"size", required_argument, 0, 'S'},
			{"visfile", required_argument, 0, 'v'},
			{0, 0, 0, 0}
		};

		int option_index=0; //for getopt
		int c = getopt_long (argc, argv, "t:s:c:d:o:p:S:v:qn", long_options, &option_index);
		if (c == -1)
		{
			break;
		}
		switch (c)
		{
		case 0: //TODO: figure out what the hell this does, cuz it never seems to get called
			if (long_options[option_index].flag != 0) //do nothing on a flag
			{
				printf("setting flag\n");
				break;
			}
			printf("option %s",long_options[option_index].name);
			if (optarg)
			{
				printf(" with arg %s", optarg);
			}
			printf("\n");
			break;
		case 'h':
			simIO->usage();
			exit(0);
			break;
		case 't':
			simIO->traceFilename = string(optarg);
			break;
		case 's':
			simIO->systemIniFilename = string(optarg);
			break;
		case 'd':
			simIO->deviceIniFilename = string(optarg);
			break;
		case 'c':
			simIO->cycleNum = atoi(optarg);
			break;
		case 'S':
			simIO->memorySize = atoi(optarg);
			break;
		case 'p':
			simIO->workingDirectory = string(optarg);
			break;
		case 'q':
			SHOW_SIM_OUTPUT = false;
			break;
		case 'n':
			simIO->useClockCycle = false;
			break;
		case 'o':
			simIO->paramOverrides = simIO->parseParamOverrides(string(optarg));
			break;
		case 'v':
			simIO->visFilename = string(optarg);
			break;
		case '?':
			simIO->usage();
			exit(-1);
			break;
		}
	}


	Simulator *simulator = new Simulator(simIO);
	simulator->setup();
	simulator->start();

	simulator->report();

}
