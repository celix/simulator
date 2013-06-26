#include "SystemConfiguration.h"
#include "SimulatorIO.h"
#include "Transaction.h"
#include "IniReader.h"
#include "DataPacket.h"

#include <sys/stat.h>
#include <sys/types.h>

#include <errno.h>
#include <sstream> //stringstream
#include <stdlib.h> // getenv()


namespace DRAMSim
{
	using namespace std;

	ofstream SimulatorIO::verifyFile;
	ofstream SimulatorIO::visFile;
	ofstream SimulatorIO::logFile;

	SimulatorIO::~SimulatorIO()
	{
		// flush our streams and close them up
		if (VIS_FILE_OUTPUT)
		{
			visFile.flush();
			visFile.close();
		}
	#ifdef LOG_OUTPUT
		logFile.flush();
		logFile.close();
	#endif

		traceFile.close();
	}

	void SimulatorIO::loadInputParams()
	{

		PRINT("+++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");
		// no default value for the default model name
		if (deviceIniFilename.length() == 0)
		{
			ERROR("Please provide a device ini file");
			usage();
			exit(-1);
		}
		else if(traceFilename.length() == 0)
		{
			ERROR("Please provide a trace file");
			usage();
			exit(-1);
		}

		//memory size check
		if (!isPowerOfTwo(memorySize))
		{
			ERROR("Please specify a power of 2 memory size");
			abort();
		}

		//setting relative working directory path
		if (workingDirectory.length() > 0)
		{
			//ignore the workingDirectory argument if the argument is an absolute path
			if (systemIniFilename[0] != '/')
			{
				systemIniFilename = workingDirectory + "/" + systemIniFilename;
			}

			if (deviceIniFilename[0] != '/')
			{
				deviceIniFilename = workingDirectory + "/" + deviceIniFilename;
			}

			if (traceFilename[0] != '/')
			{
				traceFilename = workingDirectory + "/" +traceFilename;
			}
		}


		// get the trace filename
		string temp = traceFilename.substr(traceFilename.find_last_of("/")+1);

		//get the prefix of the trace name
		temp = temp.substr(0,temp.find_first_of("_"));
		if (temp=="mase")
		{
			traceType = mase;
		}
		else if (temp=="k6")
		{
			traceType = k6;
		}
		else if (temp=="k7")
		{
			traceType = k7;
		}
		else if (temp=="pin")
		{
			traceType = pin;
		}
		else if (temp=="DGpin")
		{
			traceType = DGpin;
		}
		else
		{
			ERROR("== Unknown Tracefile Type : "<<temp);
			exit(0);
		}


		DEBUG("== Loading device model file '"<<deviceIniFilename<<"' == ");
		IniReader::ReadIniFile(deviceIniFilename, IniReader::DEV_INI);
		DEBUG("== Loading system model file '"<<systemIniFilename<<"' == ");
		IniReader::ReadIniFile(systemIniFilename, IniReader::SYS_INI);

		// If we have any overrides, set them now before creating all of the memory objects
		if (paramOverrides != NULL)
			IniReader::OverrideKeys(paramOverrides);

		IniReader::InitEnumsFromStrings();
		if (!IniReader::CheckIfAllSet())
		{
			exit(-1);
		}

		if (NUM_CHANS == 0)
		{
			ERROR("Zero channels");
			abort();
		}

		// don't need this anymore
		delete paramOverrides;

	}



	/**
	 * This function creates up to 3 output files:
	 * 	- The .log file if LOG_OUTPUT is set
	 * 	- the .vis file where csv data for each epoch will go
	 * 	- the .tmp file if verification output is enabled
	 * The results directory is setup to be in PWD/TRACEFILENAME.[SIM_DESC]/DRAM_PARTNAME/PARAMS.vis
	 * The environment variable SIM_DESC is also appended to output files/directories
	 *
	 * TODO: verification info needs to be generated per channel so it has to be
	 * moved back to MemorySystem
	 **/
	void SimulatorIO::initOutputFiles()
	{
		string simDesc;
		char *SIM_DESC = getenv("SIM_DESC");
		if (SIM_DESC)
		{
			simDesc = string(SIM_DESC);
		}

		size_t lastSlash;
		size_t dLength = deviceIniFilename.length();
		size_t tLength = traceFilename.length();
		string deviceName, traceName;

		//create output folders
		// chop off the .ini if it's there
		if (deviceIniFilename.substr(dLength-4) == ".ini")
		{
			deviceName = deviceIniFilename.substr(0,dLength-4);
			dLength -= 4;
		}
		if (traceFilename.substr(tLength-4) == ".trc")
		{
			traceName = traceFilename.substr(0,tLength-4);
			tLength -= 4;
		}
		// chop off everything past the last / (i.e. leave filename only)
		if ((lastSlash = deviceName.find_last_of("/")) != string::npos)
		{
			deviceName = deviceName.substr(lastSlash+1,dLength-lastSlash-1);
		}
		// working backwards, chop off the next piece of the directory
		if ((lastSlash = traceName.find_last_of("/")) != string::npos)
		{
			traceName = traceName.substr(lastSlash+1,tLength-lastSlash-1);
		}
		if (workingDirectory.length() > 0)
		{
			outputFilePath = workingDirectory + "/" + outputFilePath;
		}

		// create the directories if they don't exist
		mkdirIfNotExist(outputFilePath);
		outputFilePath = outputFilePath + traceName + "/";
		mkdirIfNotExist(outputFilePath);
		outputFilePath = outputFilePath + deviceName + "/";
		mkdirIfNotExist(outputFilePath);

		// create a properly named verification output file if need be and open it
		// as the stream 'verifyOut'
		if (VERIFICATION_OUTPUT)
		{
			string baseFilename = deviceIniFilename.substr(deviceIniFilename.find_last_of("/")+1);
			string verifyFilename =  "sim_out_"+baseFilename;
			if (SIM_DESC != NULL)
			{
				verifyFilename += "."+simDesc;
			}
			verifyFilename =outputFilePath + verifyFilename + ".tmp";
			verifyFile.open(verifyFilename.c_str());
			if (!verifyFile)
			{
				ERROR("Cannot open "<< verifyFilename);
				abort();
			}
		}

		// This sets up the vis file output along with the creating the result
		// directory structure if it doesn't exist
		if (VIS_FILE_OUTPUT)
		{

			if (visFilename.empty())
			{
				// finally, figure out the visFilename
				string sched = "BtR";
				string queue = "pRank";
				if (schedulingPolicy == RankThenBankRoundRobin)
				{
					sched = "RtB";
				}
				if (queuingStructure == PerRankPerBank)
				{
					queue = "pRankpBank";
				}

				stringstream tmpOut;
				tmpOut 	<< (TOTAL_STORAGE>>10) 		<< "GB."
						<< NUM_CHANS 				<< "Ch."
						<< NUM_RANKS 				<< "R."
						<< ADDRESS_MAPPING_SCHEME 	<< "."
						<< ROW_BUFFER_POLICY 		<< "."
						<< TRANS_QUEUE_DEPTH		<< "TQ."
						<< CMD_QUEUE_DEPTH 			<< "CQ."
						<< sched					<< "."
						<< queue;
				visFilename = tmpOut.str();

			}

			if (SIM_DESC != NULL)
			{
				visFilename += simDesc;
			}

			visFilename =  FilenameWithNumberSuffix( outputFilePath + visFilename, ".vis");



			visFile.open(visFilename.c_str());
			if (!visFile)
			{
				ERROR("Cannot open '"<<visFilename<<"'");
				exit(-1);
			}
			//write out the ini config values for the visualizer tool
			IniReader::WriteValuesOut(visFile);

			if (visFilename != "")
				DEBUG("== creating vis file to " <<visFilename << " ==");
		}
		else
		{
			DEBUG("vis file output disabled");
		}

	#ifdef LOG_OUTPUT
		string dramsimLogFilename("dramsim");
		if (SIM_DESC != NULL)
		{
			dramsimLogFilename += "."+simDesc;
		}

		dramsimLogFilename = FilenameWithNumberSuffix(outputFilePath + dramsimLogFilename, ".log");

		logFile.open(dramsimLogFilename.c_str(), ios_base::out | ios_base::trunc );

		if (!logFile)
		{
			ERROR("Cannot open "<< dramsimLogFilename);
			exit(-1);
		}
	#endif


		DEBUG("== Loading trace file '"<<traceFilename<<"' == ");
		traceFile.open(traceFilename.c_str());

		if (!traceFile.is_open())
		{
			cout << "== Error - Could not open trace file"<<endl;
			exit(0);
		}

		PRINT("++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++++");

	}


	Transaction* SimulatorIO::nextTrans()
	{
		string line="";
		int skipLine = 0;
		static int lineNumber = 1;

		do
		{
			if (skipLine > 0)
			{
				DEBUG("WARNING: Skipping line "<<lineNumber-1<< " ('" << line << "') in tracefile");
			}

			// return NULL when EOF
			if (!getline(traceFile, line))
			{
				return NULL;
			}

			skipLine++;
			lineNumber++;
		} while (line.length() == 0);

		uint64_t addr;
		uint64_t clockCycle=0;
		Transaction::TransactionType transType = Transaction::DATA_READ; //by default

		size_t previousIndex=0;
		size_t spaceIndex=0;
		byte *dataBuffer = NULL;
		DataPacket *dataPacket = NULL;
		string addressStr="", cmdStr="", dataStr="", ccStr="";
		size_t subrankLen = LEN_DEF;

		switch (traceType)
		{
		case k6:
		{
			spaceIndex = line.find_first_of(" ", 0);

			addressStr = line.substr(0, spaceIndex);
			previousIndex = spaceIndex;

			spaceIndex = line.find_first_not_of(" ", previousIndex);
			cmdStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
			previousIndex = line.find_first_of(" ", spaceIndex);

			spaceIndex = line.find_first_not_of(" ", previousIndex);
			ccStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);

			if (cmdStr.compare("P_MEM_WR")==0 ||
					cmdStr.compare("BOFF")==0)
			{
				transType = Transaction::DATA_WRITE;
			}
			else if (cmdStr.compare("P_FETCH")==0 ||
					 cmdStr.compare("P_MEM_RD")==0 ||
					 cmdStr.compare("P_LOCK_RD")==0 ||
					 cmdStr.compare("P_LOCK_WR")==0)
			{
				transType = Transaction::DATA_READ;
			}
			else
			{
				ERROR("== Unknown Command : "<<cmdStr);
				exit(0);
			}

			istringstream a(addressStr.substr(2));//gets rid of 0x
			a>>hex>>addr;

			//if this is set to false, clockCycle will remain at 0, and every line read from the trace
			//  will be allowed to be issued
			if (useClockCycle)
			{
				istringstream b(ccStr);
				b>>clockCycle;
			}
			break;
		}
		case k7:
		case pin:
		case DGpin:
		{
			spaceIndex = line.find_first_of(" ", 0);

			addressStr = line.substr(0, spaceIndex);
			previousIndex = spaceIndex;

			spaceIndex = line.find_first_not_of(" ", previousIndex);
			cmdStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
			previousIndex = line.find_first_of(" ", spaceIndex);

			spaceIndex = line.find_first_not_of(" ", previousIndex);
			ccStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
			previousIndex = line.find_first_of(" ", spaceIndex);

			if (previousIndex != string::npos)
			{
				spaceIndex = line.find_first_not_of(" ", previousIndex);
				istringstream tmp(line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex));
				tmp>>subrankLen;
				previousIndex = line.find_first_of(" ", spaceIndex);
			}

			if (previousIndex != string::npos)
			{
				spaceIndex = line.find_first_not_of(" ", previousIndex);
				dataStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
			}

			if (cmdStr.compare("P_MEM_WR")==0 || cmdStr.compare("BOFF")==0)
			{
				transType = Transaction::DATA_WRITE;
			}
			else if (cmdStr.compare("P_FETCH")==0 ||
					 cmdStr.compare("P_MEM_RD")==0 ||
					 cmdStr.compare("P_LOCK_RD")==0 ||
					 cmdStr.compare("P_LOCK_WR")==0)
			{
				transType = Transaction::DATA_READ;
			}
			else
			{
				ERROR("== Unknown Command : "<<cmdStr);
				exit(0);
			}

			istringstream a(addressStr.substr(2));//gets rid of 0x
			a>>hex>>addr;

			//parse data
			//if we are running in a no storage mode, don't allocate space, just return NULL
#ifdef DATA_STORAGE
			if (dataStr.size() > 0 && transType == Transaction::DATA_WRITE)
			{
				// two hex characters = 1 byte
				if (dataStr.size() % 2 != 0)
				{
					ERROR("Could you please give me the data in whole bytes?");
					exit(-1);
				}

				size_t stringBytes = dataStr.size()/2;
				// if we have more bytes than the size of a transaction, there's a problem
				if (stringBytes > TRANS_DATA_BYTES)
				{
					ERROR("Can't put "<<stringBytes<<" bytes into a single transaction");
					exit(-1);
				}

		#ifdef DATA_STORAGE_SSA
				size_t transBytes = SUBARRAY_DATA_BYTES*subrankLen;
				stringBytes = transBytes;
		#else
				size_t transBytes = TRANS_DATA_BYTES;
		#endif

				unsigned chr;
				dataBuffer = (byte *)calloc(sizeof(byte),transBytes);
				for (size_t i=0; i < transBytes; i++)
				{
					if (i < stringBytes)
					{
						string piece = dataStr.substr(i*2,2);
						//PRINT("now on piece ("<<i<<") "<<piece);
						istringstream iss(piece);
						// because of the way isstringstream works I can't directly insert
						// into a char, I have to go through an unsigned and then cast
						iss >> hex >> chr;
						dataBuffer[i] = (byte) chr;
					}
					else
					{
						dataBuffer[i] = 0x00;
					}
				}

				dataPacket = new DataPacket(dataBuffer, stringBytes, addr);

				//PRINT("ds="<<dataStr <<", bytes="<<stringBytes<<"\ndp="<<*dataPacket);
			}
#endif

			//if this is set to false, clockCycle will remain at 0, and every line read from the trace
			//  will be allowed to be issued
			if (useClockCycle)
			{
				istringstream b(ccStr);
				b>>clockCycle;
			}

			break;
		}
		case mase:
		{
			spaceIndex = line.find_first_of(" ", 0);

			addressStr = line.substr(0, spaceIndex);
			previousIndex = spaceIndex;

			spaceIndex = line.find_first_not_of(" ", previousIndex);
			cmdStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);
			previousIndex = line.find_first_of(" ", spaceIndex);

			spaceIndex = line.find_first_not_of(" ", previousIndex);
			ccStr = line.substr(spaceIndex, line.find_first_of(" ", spaceIndex) - spaceIndex);

			if (cmdStr.compare("IFETCH")==0||
					cmdStr.compare("READ")==0)
			{
				transType = Transaction::DATA_READ;
			}
			else if (cmdStr.compare("WRITE")==0)
			{
				transType = Transaction::DATA_WRITE;
			}
			else
			{
				ERROR("== Unknown command in tracefile : "<<cmdStr);
			}

			istringstream a(addressStr.substr(2));//gets rid of 0x
			a>>hex>>addr;

			//if this is set to false, clockCycle will remain at 0, and every line read from the trace
			//  will be allowed to be issued
			if (useClockCycle)
			{
				istringstream b(ccStr);
				b>>clockCycle;
			}

			break;
		}
		} // end of SWITCH

		return new Transaction(transType, addr, dataPacket, subrankLen, clockCycle);
	}


	/**
	 * Override options can be specified on the command line as -o key1=value1,key2=value2
	 * this method should parse the key-value pairs and put them into a map
	 **/
	IniReader::OverrideMap* SimulatorIO::parseParamOverrides(const string &kv_str)
	{
		IniReader::OverrideMap *kv_map = new IniReader::OverrideMap();
		size_t start = 0, comma=0, equal_sign=0;
		// split the commas if they are there
		while (1)
		{
			equal_sign = kv_str.find('=', start);
			if (equal_sign == string::npos)
			{
				break;
			}

			comma = kv_str.find(',', equal_sign);
			if (comma == string::npos)
			{
				comma = kv_str.length();
			}

			string key = kv_str.substr(start, equal_sign-start);
			string value = kv_str.substr(equal_sign+1, comma-equal_sign-1);

			(*kv_map)[key] = value;
			start = comma+1;
		}
		return kv_map;
	}

	void SimulatorIO::mkdirIfNotExist(string path)
	{
		struct stat stat_buf;
		// check if the directory exists
		if (stat(path.c_str(), &stat_buf) != 0) // nonzero return value on error, check errno
		{
			if (errno == ENOENT)
			{
	//			DEBUG("\t directory doesn't exist, trying to create ...");

				// set permissions dwxr-xr-x on the results directories
				mode_t mode = (S_IXOTH | S_IXGRP | S_IXUSR | S_IROTH | S_IRGRP | S_IRUSR | S_IWUSR) ;
				if (mkdir(path.c_str(), mode) != 0)
				{
					perror("Error Has occurred while trying to make directory: ");
					cerr << path << endl;
					abort();
				}
			}
			else
			{
				perror("Something else when wrong: ");
				abort();
			}
		}
		else // directory already exists
		{
			if (!S_ISDIR(stat_buf.st_mode))
			{
				ERROR(path << "is not a directory");
				abort();
			}
		}
	}


	string SimulatorIO::FilenameWithNumberSuffix(const string &filename, const string &extension, unsigned maxNumber)
	{
		string currentFilename = filename+extension;
		if (!fileExists(currentFilename))
		{
			return currentFilename;
		}

		// otherwise, add the suffixes and test them out until we find one that works
		stringstream tmpNum;
		tmpNum<<"."<<1;
		for (unsigned i=1; i<maxNumber; i++)
		{
			currentFilename = filename+tmpNum.str()+extension;
			if (fileExists(currentFilename))
			{
				currentFilename = filename;
				tmpNum.seekp(0L, ios::beg);
				tmpNum << "." << i;
			}
			else
			{
				return currentFilename;
			}
		}
		// if we can't find one, just give up and return whatever is the current filename
		ERROR("Warning: Couldn't find a suitable suffix for "<<filename);
		return currentFilename;
	}


	bool SimulatorIO::fileExists(string &path)
	{
		struct stat stat_buf;
		if (stat(path.c_str(), &stat_buf) != 0)
		{
			if (errno == ENOENT)
			{
				return false;
			}
			ERROR("Warning: some other kind of error happened with stat(), should probably check that");
		}
		return true;
	}

	void SimulatorIO::usage()
	{
		cout << "DRAMSim2 Usage: " << endl;
		cout << "DRAMSim -t tracefile -s system.ini -d ini/device.ini [-c #] [-p pwd] [-q] [-S 2048] [-n] [-o OPTION_A=1234,tRC=14,tFAW=19]" <<endl;
		cout << "\t-t, --tracefile=FILENAME \tspecify a tracefile to run  "<<endl;
		cout << "\t-s, --systemini=FILENAME \tspecify an ini file that describes the memory system parameters  "<<endl;
		cout << "\t-d, --deviceini=FILENAME \tspecify an ini file that describes the device-level parameters"<<endl;
		cout << "\t-c, --numcycles=# \t\tspecify number of cycles to run the simulation for [default=30] "<<endl;
		cout << "\t-q, --quiet \t\t\tflag to suppress simulation output (except final stats) [default=no]"<<endl;
		cout << "\t-o, --option=OPTION_A=234,tFAW=14\t\t\toverwrite any ini file option from the command line"<<endl;
		cout << "\t-p, --pwd=DIRECTORY\t\tSet the working directory (i.e. usually DRAMSim directory where ini/ and results/ are)"<<endl;
		cout << "\t-S, --size=# \t\t\tSize of the memory system in megabytes [default=2048M]"<<endl;
		cout << "\t-n, --notiming \t\t\tDo not use the clock cycle information in the trace file"<<endl;
		cout << "\t-v, --visfile \t\t\tVis output filename"<<endl;
	}
}



