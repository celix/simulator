#ifndef SIMULATORIO_H_
#define SIMULATORIO_H_

#include "IniReader.h"
#include "Transaction.h"

namespace DRAMSim
{

	using namespace std;

	class SimulatorIO
	{
	public:
		SimulatorIO( string sys = "system.ini",
								string dev = "",
								string trc = "",
								string vis = "",
								string wd = "",
								string out ="results/",
								IniReader::OverrideMap *po = NULL,
								unsigned ms = 2048,
								unsigned cn = 0,
								bool cc = true):
								systemIniFilename(sys),
								deviceIniFilename(dev),
								traceFilename(trc),
								visFilename(vis),
								workingDirectory(wd),
								outputFilePath(out),
								paramOverrides(po),
								memorySize(ms),
								cycleNum(cn),
								useClockCycle(cc){};
		~SimulatorIO();

		void loadInputParams();
		void initOutputFiles();

		Transaction* nextTrans();

		IniReader::OverrideMap* parseParamOverrides(const string &kv_str);
		string FilenameWithNumberSuffix(const string &filename, const string &extension, unsigned maxNumber = 100);
		void mkdirIfNotExist(string path);
		bool fileExists(string &path);
		void usage();



		string systemIniFilename;
		string deviceIniFilename;
		string traceFilename;
		string visFilename;

		string workingDirectory;
		string outputFilePath;

		ifstream traceFile;
		static ofstream verifyFile; //used in Rank.cpp and MemoryController.cpp if VERIFICATION_OUTPUT is set
		static ofstream visFile; 	//mostly used in MemoryController
		static ofstream logFile;

		TraceType traceType;
		IniReader::OverrideMap *paramOverrides;

		unsigned memorySize;
		uint64_t cycleNum;
		bool useClockCycle;
	};


}


#endif /* SIMULATORIO_H_ */
