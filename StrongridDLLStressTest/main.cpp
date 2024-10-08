/*
*  main.cpp
*
*  Copyright (C) 2017 Luigi Vanfretti
*
*  This file is part of StrongridDLL.
*
*  StrongridDLL is free software: you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation, either version 3 of the License, or
*  (at your option) any later version.
*
*  StrongridDLL is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with StrongridDLL.  If not, see <http://www.gnu.org/licenses/>.
*
*/

#include <cctype>
#include <cstring>      // memset, strlen
#include <ctime>        // std::localtime, std::time, std::time_t, std::tm
#include <fstream>
#include <iomanip>
#include <iostream>
#include <map>
#include <sstream>
#include <thread>       // std::this_thread, std::thread
#include <vector>

#include "../StrongridDLL/Strongrid.h"
#include "Common.h"

using namespace std;
using namespace stresstest;

static const int TIMEOUT_MS = 30000;


static bool WRITE_OUTPUT_TO_FILE = true;
volatile bool SHUTDOWN_FLAG = false;

void AssertIsZero( int val )
{
	if( val != 0 ) throw Exception("Function did not return 0");
}

class PmuState
{
public:
	PmuState( pmuConfig_Ver3* pmuCfg, int pmuIndex )
	{
		m_pmuConfiguration = 0;
		m_pmuConfigurationVer3 = pmuCfg;
		m_pmuIndex = pmuIndex;
		m_data = 0;

		// Create and initialize realData object
		m_data = new noArraysPmuDataFrame();
		phasorValueReal = new float[m_pmuConfigurationVer3->numberOfPhasors];
		phasorValueImaginary = new float[m_pmuConfigurationVer3->numberOfPhasors];
		analogValueArr = new float[m_pmuConfigurationVer3->numberOfAnalog];
		digitalValueArr = new uint8_t[m_pmuConfigurationVer3->numberOfDigital];
		PhasorArrayLength = m_pmuConfiguration->numberOfPhasors;
		AnalogArrayLength = m_pmuConfiguration->numberOfAnalog;
		DigitalArrayLength = m_pmuConfiguration->numberOfDigital;
	}

	PmuState( pmuConfig* pmuCfg, int pmuIndex )
	{
		m_pmuConfiguration = pmuCfg;
		m_pmuConfigurationVer3 = 0;
		m_pmuIndex = pmuIndex;
		m_data = 0;

		// Create and initialize realData object
		m_data = new noArraysPmuDataFrame();

		// Initialize arrays
		phasorValueReal = new float[m_pmuConfiguration->numberOfPhasors];
		phasorValueImaginary = new float[m_pmuConfiguration->numberOfPhasors];
		analogValueArr = new float[m_pmuConfiguration->numberOfAnalog];
		digitalValueArr = new uint8_t[m_pmuConfiguration->numberOfDigital];
		PhasorArrayLength = m_pmuConfiguration->numberOfPhasors;
		AnalogArrayLength = m_pmuConfiguration->numberOfAnalog;
		DigitalArrayLength = m_pmuConfiguration->numberOfDigital;
	}

	~PmuState()
	{
		if( m_pmuConfiguration != 0 ) {
			delete [] m_pmuConfiguration->stationname;
			delete m_pmuConfiguration;
		}
		if( m_pmuConfigurationVer3 != 0 ) {
			delete [] m_pmuConfigurationVer3->stationname;
			delete m_pmuConfigurationVer3;
		}

		delete [] phasorValueReal;
		delete [] phasorValueImaginary;
		delete [] analogValueArr;
		delete [] digitalValueArr;
		delete m_data;
	}
/*
	const pmuConfig_Ver3* PmuCfgVer3() { return m_pmuConfigurationVer3; }
	const pmuConfig* PmuCfgVer2() { return m_pmuConfiguration; }*/
	const noArraysPmuDataFrame* PmuData() { return m_data; }
	noArraysPmuDataFrame* PmuDataRef() { return m_data; }

	void ObfuscateRealDataFrame()
	{
		//// obfuscate phasor values
		//for( int i = 0; i < m_pmuConfiguration->numberOfPhasors; ++i ) {
		//	m_data->phasorValueReal[i] = 77;
		//	m_data->phasorValueImaginary[i] = 77;
		//}

		//// Obfuscate analog data
		//for( int i = 0; i < m_pmuConfiguration->numberOfAnalog; ++i )
		//	m_data->analogValueArr[i] = 99;

		//// Obfuscate digital data
		//for( int i = 0; i < m_pmuConfiguration->numberOfDigital; ++i )
		//	m_data->digitalValueArr[i] = 88;
	}

	int PmuIndex() const { return m_pmuIndex; }


	float* phasorValueReal;
	float* phasorValueImaginary;
	float* analogValueArr;
	int PhasorArrayLength;
	int AnalogArrayLength;
	int DigitalArrayLength;
	uint8_t* digitalValueArr;

private:
	pmuConfig_Ver3* m_pmuConfigurationVer3;
	pmuConfig* m_pmuConfiguration;
	noArraysPmuDataFrame* m_data;
	int m_pmuIndex;
};

std::tm current_time()
{
	const std::time_t unix_time = std::time(nullptr);
	return *std::localtime(&unix_time);
}

std::string getLogTs()
{
	std::tm sysTime = current_time();
	ostringstream ostr;
	ostr << sysTime.tm_year << "." << sysTime.tm_mon << "." << sysTime.tm_mday << " "
			<< setfill('0') << setw(2) << sysTime.tm_hour  << ":"
			<< setfill('0') << setw(2) << sysTime.tm_min << ":"
			<< setfill('0') << setw(2) << sysTime.tm_sec << "\t";

	return ostr.str();
}

void ReadFrameLoopProc(int pseudoPdcId, ostream& strout, const std::vector<PmuState*>& pmuConfigurationMap  )
{
	int pseudoPdcIdArray[1024];

	// Read frame loop
	//for( int numIterations = 0; numIterations < 50 * 10 && SHUTDOWN_FLAG == false; ++numIterations )
	while( SHUTDOWN_FLAG == false )
	{
		strout << "=============================================================\nREADING DATAFRAME\n=============================================================\n";

		// TMP DEBUG - test threadpooling base function
		int pdcWithData = 0;
		memset(pseudoPdcIdArray,0,sizeof(int)*1024);
		AssertIsZero( pollPdcWithDataWaiting(1024, pseudoPdcIdArray, &pdcWithData, 30000) );

		// Read next frame
		AssertIsZero(::readNextFrame(1500, pseudoPdcId));

		// Get PDC data
		pdcDataFrame pdcData;
		AssertIsZero(::getPdcRealData(&pdcData, pseudoPdcId));

		// Print timestamp
		ParsedTimestamp ts = pdcData.Timestamp;
		strout << getLogTs() << "Dataframe Ts: "
			<< setfill('0') << setw(4) << ts.Year << "."
			<< setfill('0') << setw(2) << ts.Month << "."
			<< setfill('0') << setw(2) << ts.Day << " "
			<< setfill('0') << setw(2) << ts.Hour << ":"
			<< setfill('0') << setw(2) << ts.Minute << ":"
			<< setfill('0') << fixed << setw(2) << setprecision(3) << (ts.Second + (float)ts.Ms / 1000.0f);

		// Print timestamp quality data
		strout << ", ClockIsReliable=" << pdcData.TimeQuality.ClockIsReliable
			<< ", ClockErrSec="  <<  pdcData.TimeQuality.ClockErrorSec
			<< ", LeapSecPending=" << pdcData.TimeQuality.LeapSecPending
			<< ", LeapSecCorr=" << pdcData.TimeQuality.LeapSecCorrection
			<< std::endl;

		for( std::vector<PmuState*>::const_iterator iter = pmuConfigurationMap.begin(); iter != pmuConfigurationMap.end(); ++iter )
		{
			// Get a quick ref to the pmustate object for easy use
			PmuState* pmu = *iter;

			// FOR TEST ONLY - RANDOMIZE CONTENT OF PMU DATAFRAME
			pmu->ObfuscateRealDataFrame();

			// Print data - values
			PmuStatus pmuSts;
			AssertIsZero(::getPmuRealDataLabview(pmu->PmuDataRef(), &pmuSts,
				pmu->PhasorArrayLength, pmu->phasorValueReal, pmu->phasorValueImaginary,
				pmu->AnalogArrayLength, pmu->analogValueArr,
				pmu->DigitalArrayLength, pmu->digitalValueArr,
				pseudoPdcId, pmu->PmuIndex()));

			// Print STAT
			strout << getLogTs() << "Statusbits - \n\t"
				<< "DataErrorCode=" << (int)pmuSts.dataErrorCode << ", "
				<< "PmuSyncFlag=" << pmuSts.pmuSyncFlag << ", "
				<< "PmuDataSortFlag=" << pmuSts.pmuDataSortingFlag << ", "
				<< "TriggerFlag=" << pmuSts.pmuTriggerFlag << ", "
				<< "ConfCngFlag=" << pmuSts.configChangeFlag << ",\n\t"
				<< "DataModified=" << pmuSts.dataModifiedFlag << ","
				<< "TimeQualityCode=" << (int)pmuSts.timeQualityCode << ", "
				<< "UnlockTimeCode=" << (int)pmuSts.unlockTimeCode << ", "
				<< "TriggerReasonCode=" << (int)pmuSts.triggerReasonCode
				<< std::endl;

			// Print all phasors
			strout << getLogTs() << "Phasor values: (#" << pmu->PhasorArrayLength << "): ";
			for( int i= 0; i < pmu->PhasorArrayLength; ++i )
				strout << pmu->phasorValueReal[i] << "|" << pmu->phasorValueImaginary[i] << ",";
			strout << " :END\n";

			// Print all analog values
			strout << getLogTs() << "Analog values (#" << pmu->AnalogArrayLength << "): ";
			for( int i = 0; i < pmu->AnalogArrayLength; ++i )
				strout << pmu->analogValueArr[i] << ",";
			strout << " :END\n";

			// Print all digital bits
			strout << getLogTs() << "Digital values: (#" << pmu->DigitalArrayLength << "): ";
			for( int i = 0; i < pmu->DigitalArrayLength; ++i )
				strout << (int)pmu->digitalValueArr[i] << ",";
			strout << " :END\n\n";

			strout.flush();
		}
	}
}

void pdcThreadProc_Ver3(PdcConfig* pdccfgraw)
{
	PdcConfig pdc = *pdccfgraw;
	strongrid_library_init();

	while( SHUTDOWN_FLAG == false )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		int pseudoPdcId = -1;

		ostringstream ostr;
		ostream strout(cout.rdbuf());
		filebuf   objFileBuf;

		try {
			// Connect to the pdc
			bool isConnected = false;
			while ( isConnected == false )
			{
				strout << getLogTs() << "Connecting to PDC, ID=" << pdc.PdcId << ", IP=" << pdc.IP << ", Port=" << pdc.Port << endl;
				int retval = ::connectPdc((char*)pdc.IP.c_str(), pdc.Port, atoi(pdc.PdcId.c_str()), &pseudoPdcId );
				isConnected = retval == 0;
				if( isConnected == false ) {
		 			strout << getLogTs() << "StrongridIEEEC37118Dll::connectPdc returned " << retval << ". Waiting 10s.";
					std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				}
			}
			strout << getLogTs() << "Connection established" << endl;

			// change output buffer to 'file' if
			if( WRITE_OUTPUT_TO_FILE )
			{
				// Get timestamp
				std::tm sysTime = current_time();

				ostr.clear(); ostr << "PDC_" << pdc.PdcId << "!" << pseudoPdcId << "_"
					<< sysTime.tm_year << "." << sysTime.tm_mon << "." << sysTime.tm_mday << "_"
					<< setfill('0') << setw(2) << sysTime.tm_hour  << "_"
					<< setfill('0') << setw(2) << sysTime.tm_min << "_"
					<< setfill('0') << setw(2) << sysTime.tm_sec <<  ".log";
				ostr.width(2);
				strout << getLogTs() << "Switching to file based output: '" << ostr.str() << "' for PDCID=" << pdc.PdcId << " @ " << pdc.IP << ":" << pdc.Port << std::endl;
				objFileBuf.open (ostr.str(), ios :: out);
				strout.rdbuf(&objFileBuf);
			}

			// Assert on the pseudopdcid - must be >= 0
			if( pseudoPdcId < 0 ) throw Exception("PseudoPdcId < 0");


			// Read header
			char headerBuffer[1024];
			AssertIsZero(::readHeaderData(30000, pseudoPdcId));
			AssertIsZero(::getHeaderMsg(headerBuffer,1024, pseudoPdcId ));
			strout << getLogTs() << "Header message:\n----------------------------------------\n" << headerBuffer << "\n----------------------------------------\n";

			// Read configuration
			strout << getLogTs() << "Reading configuration...";
			int retval = ::readConfiguration_Ver3(TIMEOUT_MS, pseudoPdcId);
			if( retval != 0 ) {
				ostr.clear(); ostr << "StrongridIEEEC37118Dll::connectPdc returned " << retval << ".";
				throw Exception(ostr.str());
			}
			strout << "Configuration read successfully" << endl;

			// Print all configuration parameters: Common pdc
			pdcConfiguration pdcCommonCfg;
			AssertIsZero(::getPdcConfig_Ver3(&pdcCommonCfg, pseudoPdcId ));
			strout << getLogTs() << "getMainConfig - "
				<< "\n\tFrame/Sec=" << pdcCommonCfg.FramesPerSecond
				<< endl << endl;

			// Map pseudoPdcId->pmuConfig
			std::vector<PmuState*> pmuList;

			// Get PMU configuration for each PMU in the PDC
			for( int i = 0; i < pdcCommonCfg.numberOfPMUs; ++i )
			{

				// Extract pmu configuration for the PMU at index 'i'
				pmuConfig_Ver3* pmuConf = new pmuConfig_Ver3(); pmuConf->stationname = new char[256];

				AssertIsZero(::getPmuConfiguration_Ver3(pmuConf, pseudoPdcId, i ));

				// Print common configuration params
				strout << getLogTs() << "getPmuConfiguration_Ver3/pmuConfig - "
					<< "\n\tpmuId=" << pmuConf->pmuid
					<< "\n\tstationname=" << pmuConf->stationname
					<< "\n\tnomFreq=" << pmuConf->nominalFrequency
					<< "\n\tnumberOfPhasors=" << pmuConf->numberOfPhasors
					<< "\n\tnumberOfAnalog=" << pmuConf->numberOfAnalog
					<< "\n\tnumberOfDigital=" << pmuConf->numberOfDigital
					<< "\n\tPosition LAT/LON/ELEV=" << pmuConf->POS_LAT << "|" << pmuConf->POS_LON << "|" << pmuConf->POS_ELEV
					<< "\n\tPhasorMsmGroupDelay=" << pmuConf->PhasorMeasurementGroupDelayMs
					<< "\n\tPhasorMsmWindow=" << pmuConf->PhasorMeasurementWindow
					<< endl << endl;

				char tmpNameBuffer[256];

				// Print PHASOR scalar information
				for( int iPhasor = 0; iPhasor < pmuConf->numberOfPhasors; ++iPhasor )
				{
					phasorConfig_Ver3 phasorCfg; phasorCfg.name = tmpNameBuffer;
					AssertIsZero(::getPhasorConfig_Ver3(&phasorCfg, pseudoPdcId, i, iPhasor ));
					strout << getLogTs() << "getPhasorConfig_Ver3/phasorConfig_Ver3 - "
						<< "\n\tName=" << phasorCfg.name
						<< "\n\tType=" << (int)phasorCfg.type
						<< "\n\tFormat=" << (int)phasorCfg.format
						<< "\n\tDataIsScaled=" << phasorCfg.dataIsScaled
						<< "\n\tScalar_Magnitude=" << phasorCfg.scaling_magnitude
						<< "\n\tScalar_Offset=" << phasorCfg.scaling_angleOffset
						<< std::endl << std::endl;
				}

				// Print ANALOG scalar information
				for( int iAnalog = 0; iAnalog < pmuConf->numberOfAnalog; ++iAnalog )
				{
					analogConfig_Ver3 analogCfg; analogCfg.name = tmpNameBuffer;
					AssertIsZero(::getAnalogConfig_Ver3(&analogCfg, pseudoPdcId, i, iAnalog ));
					strout << getLogTs() << "getPhasorConfig_Ver3/analogConfig_Ver3 - "
						<< "\n\tName=" << analogCfg.name
						<< "\n\tDataIsScaled=" << analogCfg.dataIsScaled
						<< "\n\tScalar_Magnitude=" << analogCfg.scaling_magnitude
						<< "\n\tScalar_Offset=" << analogCfg.scaling_offset
						<< std::endl << std::endl;
				}

				// Print DIGITAL unit information
				for( int iDig = 0; iDig < pmuConf->numberOfDigital; ++iDig )
				{
					digitalConfig digCfg; digCfg.name = tmpNameBuffer;
					AssertIsZero(::getDigitalConfig_Ver3(&digCfg, pseudoPdcId, i, iDig ));
					if( strlen(digCfg.name) == 0 ) continue;
					strout << getLogTs() << "getDigitalConfig/digitalConfig - "
						<< "\n\tName=" << digCfg.name
						<< "\n\tIdx=" << iDig
						<< "\n\tNormalBit=" << digCfg.normalBit
						<< "\n\tValidBit=" << digCfg.isValidBit
						<< std::endl << std::endl;
				}

				pmuList.push_back(new PmuState(pmuConf, i));
			}

			strout << getLogTs() << "\nStarting datastream..." << endl;
			AssertIsZero(::startDataStream(pseudoPdcId));

			// Ender shared-readframe loop
			ReadFrameLoopProc(pseudoPdcId, strout, pmuList );

			// Clean up
			for( std::vector<PmuState*>::iterator iter = pmuList.begin(); iter != pmuList.end(); ++iter )
				delete (*iter);
			pmuList.clear();

			// Disconnect from pdc
			AssertIsZero(::disconnectPdc(pseudoPdcId));
			strout << getLogTs() << "Disconnected from PDC" << endl;
			strout.flush();
		}
		catch( Exception e )
		{
			printf("An error has ocurred: %s\n", e.ErrorMessage().c_str() );
		}
		catch( ... )
		{
			printf("An unknown error has ocurred");
		}
	}

	strongrid_library_cleanup();
}

void pdcThreadProc(PdcConfig* pdccfgraw)
{
	PdcConfig pdc = *pdccfgraw;
	strongrid_library_init();

	while( SHUTDOWN_FLAG == false )
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
		int pseudoPdcId = -1;

		ostringstream ostr;
		ostream strout(cout.rdbuf());
		filebuf   objFileBuf;

		try {
			// Connect to the pdc
			bool isConnected = false;
			while ( isConnected == false )
			{
				strout << getLogTs() << "Connecting to PDC, ID=" << pdc.PdcId << ", IP=" << pdc.IP << ", Port=" << pdc.Port << endl;
				int retval = ::connectPdc((char*)pdc.IP.c_str(), pdc.Port, atoi(pdc.PdcId.c_str()), &pseudoPdcId );
				isConnected = retval == 0;
				if( isConnected == false ) {
		 			strout << getLogTs() << "StrongridIEEEC37118Dll::connectPdc returned " << retval << ". Waiting 10s.";
					std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				}
			}
			strout << getLogTs() << "Connection established" << endl;

			// change output buffer to 'file' if
			if( WRITE_OUTPUT_TO_FILE )
			{
				// Get timestamp
				std::tm sysTime = current_time();

				ostr.clear(); ostr << "PDC_" << pdc.PdcId << "!" << pseudoPdcId << "_"
					<< sysTime.tm_year << "." << sysTime.tm_mon << "." << sysTime.tm_mday << "_"
					<< setfill('0') << setw(2) << sysTime.tm_hour  << "_"
					<< setfill('0') << setw(2) << sysTime.tm_min << "_"
					<< setfill('0') << setw(2) << sysTime.tm_sec <<  ".log";
				ostr.width(2);
				strout << getLogTs() << "Switching to file based output: '" << ostr.str() << "' for PDCID=" << pdc.PdcId << " @ " << pdc.IP << ":" << pdc.Port << std::endl;
				objFileBuf.open (ostr.str(), ios :: out);
				strout.rdbuf(&objFileBuf);
			}

			// Assert on the pseudopdcid - must be >= 0
			if( pseudoPdcId < 0 ) throw Exception("PseudoPdcId < 0");

			// Read header
			char headerBuffer[1024];
			::stopDataStream(pseudoPdcId);
			if( readHeaderData(10000, pseudoPdcId) == 0 && getHeaderMsg(headerBuffer,1024, pseudoPdcId ) == 0 )
				strout << getLogTs() << "Header message:\n----------------------------------------\n" << headerBuffer << "\n----------------------------------------\n";
			else
				strout << getLogTs() << "UNABLE TO READ HEADER MESSAGE!" << endl;

			// Read configuration
			strout << getLogTs() << "Reading configuration...";
			int retval = ::readConfiguration(TIMEOUT_MS, pseudoPdcId);
			if( retval != 0 ) {
				ostr.clear(); ostr << "StrongridIEEEC37118Dll::connectPdc returned " << retval << ".";
				throw Exception(ostr.str());
			}
			strout << "Configuration read successfully" << endl;

			// Print all configuration parameters: Common pdc
			pdcConfiguration pdcCommonCfg;
			AssertIsZero(::getPdcConfig(&pdcCommonCfg, pseudoPdcId ));
			strout << getLogTs() << "getMainConfig - "
				<< "\n\tFrame/Sec=" << pdcCommonCfg.FramesPerSecond
				<< endl << endl;

			// Map pseudoPdcId->pmuConfig
			std::vector<PmuState*> pmuList;

			// Get PMU configuration for each PMU in the PDC
			for( int i = 0; i < pdcCommonCfg.numberOfPMUs; ++i )
			{

				// Extract pmu configuration for the PMU at index 'i'
				pmuConfig* pmuConf = new pmuConfig(); pmuConf->stationname = new char[256];

				AssertIsZero(::getPmuConfiguration(pmuConf, pseudoPdcId, i ));

				// Print common configuration params
				strout << getLogTs() << "getPmuConfiguration/pmuConfig - "
					<< "\n\tpmuId=" << pmuConf->pmuid
					<< "\n\tstationname=" << pmuConf->stationname
					<< "\n\tnomFreq=" << pmuConf->nominalFrequency
					<< "\n\tnumberOfPhasors=" << pmuConf->numberOfPhasors
					<< "\n\tnumberOfAnalog=" << pmuConf->numberOfAnalog
					<< "\n\tnumberOfDigital=" << pmuConf->numberOfDigital
					<< endl << endl;

				char tmpNameBuffer[256];

				// Print PHASOR scalar information
				for( int iPhasor = 0; iPhasor < pmuConf->numberOfPhasors; ++iPhasor )
				{
					phasorConfig phasorCfg; phasorCfg.name = tmpNameBuffer;
					AssertIsZero(::getPhasorConfig(&phasorCfg, pseudoPdcId, i, iPhasor ));
					strout << getLogTs() << "getPhasorConfig/phasorConfig - "
						<< "\n\tName=" << phasorCfg.name
						<< "\n\tType=" << (int)phasorCfg.type
						<< "\n\tFormat=" << (int)phasorCfg.format
						<< "\n\tDataIsScaled=" << phasorCfg.dataIsScaled
						<< "\n\tScalar=" << phasorCfg.scalar
						<< std::endl << std::endl;
				}

				// Print ANALOG scalar information
				for( int iAnalog = 0; iAnalog < pmuConf->numberOfAnalog; ++iAnalog )
				{
					analogConfig analogCfg; analogCfg.name = tmpNameBuffer;
					AssertIsZero(::getAnalogConfig(&analogCfg, pseudoPdcId, i, iAnalog ));
					strout << getLogTs() << "getPhasorConfig/analogConfig - "
						<< "\n\tName=" << analogCfg.name
						<< "\n\tDataIsScaled=" << analogCfg.dataIsScaled
						<< "\n\tUserdefined_Scalar=" << analogCfg.userdefined_scalar
						<< std::endl << std::endl;
				}

				// Print DIGITAL unit information
				for( int iDig = 0; iDig < pmuConf->numberOfDigital; ++iDig )
				{
					digitalConfig digCfg; digCfg.name = tmpNameBuffer;
					AssertIsZero(::getDigitalConfig(&digCfg, pseudoPdcId, i, iDig ));
					if( strlen(digCfg.name) == 0 ) continue;
					strout << getLogTs() << "getDigitalConfig/digitalConfig - "
						<< "\n\tName=" << digCfg.name
						<< "\n\tIdx=" << iDig
						<< "\n\tNormalBit=" << digCfg.normalBit
						<< "\n\tValidBit=" << digCfg.isValidBit
						<< std::endl << std::endl;
				}

				pmuList.push_back( new PmuState(pmuConf, i) );
			}

			strout << getLogTs() << "\nStarting datastream..." << endl;
			AssertIsZero(::startDataStream(pseudoPdcId));

			// Ender shared-readframe loop
			ReadFrameLoopProc(pseudoPdcId, strout, pmuList );

			// Clean up
			for( std::vector<PmuState*>::iterator iter = pmuList.begin(); iter != pmuList.end(); ++iter )
				delete (*iter);
			pmuList.clear();

			// Disconnect from pdc
			AssertIsZero(::disconnectPdc(pseudoPdcId));
			strout << getLogTs() << "Disconnected from PDC" << endl;
			strout.flush();
		}
		catch( Exception e )
		{
			strout << getLogTs() << "An error has ocurred: " << e.ErrorMessage() << endl;
		}
		catch( ... )
		{
			printf("An unknown error has ocurred");
		}
	}

	strongrid_library_cleanup();
}

int main(int argc, char *argv[])
{
		string IP   =   "130.237.53.177";
		int Port    =   10000;
		string PdcId =  "1000";
		int Version =   1;

	try {
		PdcConfig config(IP, Port, PdcId, Version);

		// Print PDC in test
		cout << "PDC in test:" << endl;
        cout << "   - PDC mounted for test: IP=" << config.IP << ", PORT=" << config.Port << ", PDCID=" << config.PdcId << endl;
		cout << endl << endl << "Press 'q' and then 'Enter' to exit." << endl << endl;

		// Connect to PDC..
		cout << endl << endl << "Spawning PDC worker thread:\n";
		cout << endl << endl << "Connect to PDC:\n";

		std::thread worker;
		if (config.Version == 1 )
			worker = std::thread(pdcThreadProc, &config);
		else if (config.Version == 2)
			worker = std::thread(pdcThreadProc_Ver3, &config);
		else
			throw Exception("Invalid config - Version must be either 1 or 2");


		// Close program
		while( cin.get() != 'q');
		cout << "\nShutting down..";
		SHUTDOWN_FLAG = true; // signal shutdown to PDC worker threads

		worker.detach();
		std::this_thread::sleep_for(std::chrono::milliseconds(2000));
	}
	catch( Exception e )
	{
		cout << "An error has ocurred: " << e.ErrorMessage() << endl;
		cin.get();
	}
	catch( ... )
	{
		cout << "an unknown error has occurred. Shutting down.";
		cin.get();
	}
}
