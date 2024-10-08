/*
*  Strongrid.h
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

#ifndef STRONGRIDIEEEC37118DLL_H
#define STRONGRIDIEEEC37118DLL_H


#include <cstdint>
typedef unsigned char      BOOL8_t;

#define STRONGRIDIEEEC37118DLL_API extern "C"

typedef struct
{
	int32_t LeapSecCorrection; // +1 for 1 second added, and -1 for subtract. 0 means no change
	BOOL8_t LeapSecPending; // Early warning of pending leap second
	BOOL8_t ClockIsReliable;
	float ClockErrorSec;
} TimeStatus;

typedef struct
{
	int16_t Year;
	int16_t Month;
	int16_t Day;
	int16_t Hour;
	int16_t Minute;
	int16_t Second;
	int16_t Ms;
} ParsedTimestamp;

typedef struct
{
	TimeStatus TimeQuality;
	ParsedTimestamp Timestamp;
	double SecondOfCentury;

	float FramesPerSecond;
	int numberOfPMUs;

}pdcConfiguration;


typedef struct
{
	int32_t pmuid;
	char* stationname;
	float nominalFrequency;
	int32_t numberOfPhasors;
	int32_t numberOfAnalog;
	int32_t numberOfDigital;

}pmuConfig;

typedef struct
{
	int32_t pmuid;
	float nominalFrequency;
	int32_t numberOfPhasors;
	int32_t numberOfAnalog;
	int32_t numberOfDigital;

}noArraysPmuConfig;

typedef struct
{
	int32_t pmuid;
	char* stationname;
	float nominalFrequency;
	int32_t numberOfPhasors;
	int32_t numberOfAnalog;
	int32_t numberOfDigital;

	float POS_LAT;
	float POS_LON;
	float POS_ELEV;
	uint8_t SVC_CLASS;
	int32_t PhasorMeasurementWindow;
	int32_t PhasorMeasurementGroupDelayMs;
	char globalPmuId[16];

}pmuConfig_Ver3;


typedef struct PmuStatus
{
	// Data error:
	//  - 0 = good measurement data - no errors
	//  - 1 = PMU error. No information about data
	//  - 2 = PMU in test mode
	//  - 3 = PMU error (no not use values)
	uint8_t dataErrorCode;

	// PMU in sync with a UTC tracable time source
	BOOL8_t pmuSyncFlag;

	// Data sorting - true=sort by timestamp / false=sort by arrival
	BOOL8_t pmuDataSortingFlag;

	// PMU trigger detected
	BOOL8_t pmuTriggerFlag;

	// Configuration change detected
	BOOL8_t configChangeFlag;

	// Data modified - true=modified by post processing / false=not
	BOOL8_t dataModifiedFlag;

	// Time quality codes
	// - 0 = Not used (indicates code from previous version of profile)
	// - 1 = Estimated maximum time error < 100 ns
	// - 2 = Estimated maximum time error < 1 us
	// - 3 = Estimated maximum time error < 10 us
	// - 4 = Estimated maximum time error < 100 us
	// - 5 = Estimated maximum time error < 1 ms
	// - 6 = Estimated maximum time error < 10 ms
	// - 7 = Estiamted maximum time error > 10 ms or time error unknown
	uint8_t timeQualityCode;

	// Unlocked time:
	//  - 0 = sync locked or unlocked < 10 s (best quality)
	//  - 1 = 10 s <= unlocked time < 100 s
	//  - 2 = 100s < unlock time <= 1000s
	//  - 3 = unlocked time > 1000s
	uint8_t unlockTimeCode;

	// Trigger reason
	//  - 0 = manual
	//  - 1 = magnitude low
	//  - 2 = magnitude high
	//  - 3 = Phase angle diff
	//  - 4 = Frequency high or low
	//  - 5 = df/dt High
	//  - 6 = <reserved>
	//  - 7 = Digital
	uint8_t triggerReasonCode;
};

typedef struct
{
	TimeStatus TimeQuality;
	ParsedTimestamp Timestamp;
	double SecondOfCentury;
	int NumPmuInDataFrame;

}pdcDataFrame ;

typedef struct
{
	float			frequency;
	float			deltaFrequency;

	uint16_t		PhasorArrayLength;
	float*			phasorValueReal;
	float*			phasorValueImaginary;

	uint16_t		AnalogArrayLength;
	float*			analogValueArr;

	uint16_t		DigitalArrayLength;
	BOOL8_t*		digitalValueArr;

}pmuDataFrame ;

typedef struct
{
	float			frequency;
	float			deltaFrequency;

}noArraysPmuDataFrame;

typedef struct
{
	char*		    name;  // INPUT ARRAY MUST BE >= 256 in length
	uint8_t			type; // 0 == voltage, 1 == current
	uint8_t			format; // 0 = rectangular, 1 = polar | NOTE: currently this version is always in 'rectangular'
	BOOL8_t			dataIsScaled;	// True if scaled - false if not
	float			scalar;

}phasorConfig;

typedef struct
{
	uint8_t			type; // 0 == voltage, 1 == current
	uint8_t			format; // 0 = rectangular, 1 = polar | NOTE: currently this version is always in 'rectangular'
	BOOL8_t			dataIsScaled;	// True if scaled - false if not
	float			scalar;

}noArraysPhasorConfig;

typedef struct
{
	char*		    name;  // INPUT ARRAY MUST BE >= 256 in length
	uint8_t			type; // 0 == voltage, 1 == current
	uint8_t			format; // 0 = rectangular, 1 = polar | NOTE: currently this version is always in 'rectangular'
	BOOL8_t			dataIsScaled;	// True if scaled - false if not
	float			scaling_magnitude;
	float			scaling_angleOffset;

}phasorConfig_Ver3;

typedef struct
{
	char*		    name;  // INPUT ARRAY MUST BE >= 256 in length

	// 0 = Single point on wave
	// 1 = RMS of analog input
	// 2 = peak of analog input
	// 5-64 = reserved
	int Type;

	// Scaling information
	BOOL8_t			dataIsScaled;  // True if already scaled - false if not
	float			userdefined_scalar;

}analogConfig;

typedef struct
{
	// 0 = Single point on wave
	// 1 = RMS of analog input
	// 2 = peak of analog input
	// 5-64 = reserved
	int Type;

	// Scaling information
	BOOL8_t			dataIsScaled;  // True if already scaled - false if not
	float			userdefined_scalar;

}noArraysAnalogConfig;

typedef struct
{
	char*		    name;  // INPUT ARRAY MUST BE >= 256 in length

	BOOL8_t			dataIsScaled;  // True if already scaled - false if not
	float			scaling_magnitude;
	float			scaling_offset;

}analogConfig_Ver3;

typedef struct
{
	char*		name;	   // INPUT ARRAY MUST BE >= 256 in length
	BOOL8_t		normalBit; // "Normal" state of bit
	BOOL8_t		isValidBit;  // Bit is valid
}digitalConfig;

typedef struct
{
	BOOL8_t		normalBit; // "Normal" state of bit
	BOOL8_t		isValidBit;  // Bit is valid
}noArraysDigitalConfig;

STRONGRIDIEEEC37118DLL_API void strongrid_library_init();

STRONGRIDIEEEC37118DLL_API int connectPdc( char *ipAddress,  int port, int32_t pdcId,  int32_t* pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int disconnectPdc( int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int readHeaderData( int32_t timeoutMs, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int readConfiguration( int32_t timeoutMs, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int readConfiguration_Ver3( int32_t timeoutMs, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int startDataStream( int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int stopDataStream( int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int readNextFrame( int32_t timeoutMs, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int strongrid_library_cleanup();

// Fills in "outPseudoPdcIdArr" with pseudoPdcId's which have data waiting.
//  pseudoPdcIdArrayLength = The length of the input array
//  outPseudoPdcIdArr = the input array where the pseudo PDC id's are placed
//  outNumPdcWithData = the total number of PDC's with data waiting
STRONGRIDIEEEC37118DLL_API int pollPdcWithDataWaiting( int pseudoPdcIdArrayLength, int32_t* outPseudoPdcIdArr, int32_t* outNumPdcWithData, int pollTimeoutMs );

STRONGRIDIEEEC37118DLL_API int getPdcConfig(pdcConfiguration* pdcCfg, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int getPdcConfig_Ver3(pdcConfiguration* pdcCfg, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int getPmuConfiguration(pmuConfig* pmuconf, int32_t pseudoPdcId, int32_t pmuIndex);

STRONGRIDIEEEC37118DLL_API int getPmuConfiguration_Ver3(pmuConfig_Ver3* pmuconf, int32_t pseudoPdcId, int32_t pmuIndex);

STRONGRIDIEEEC37118DLL_API int getPhasorConfig(phasorConfig *phasorCfg, int32_t pseudoPdcId, int32_t pmuIndex, int32_t phasorIndex);

STRONGRIDIEEEC37118DLL_API int getPhasorConfig_Ver3(phasorConfig_Ver3 *phasorCfg, int32_t pseudoPdcId, int32_t pmuIndex, int32_t phasorIndex);

STRONGRIDIEEEC37118DLL_API int getAnalogConfig(analogConfig *analogCfg, int32_t pseudoPdcId, int32_t pmuIndex, int32_t analogIndex);

STRONGRIDIEEEC37118DLL_API int getAnalogConfig_Ver3(analogConfig_Ver3 *analogCfg, int32_t pseudoPdcId, int32_t pmuIndex, int32_t analogIndex);

STRONGRIDIEEEC37118DLL_API int getDigitalConfig(digitalConfig* digitalCfg, int32_t pseudoPdcId, int32_t pmuIndex, int32_t digitalIndex );

STRONGRIDIEEEC37118DLL_API int getDigitalConfig_Ver3(digitalConfig* digitalCfg, int32_t pseudoPdcId, int32_t pmuIndex, int32_t digitalIndex );

STRONGRIDIEEEC37118DLL_API int getPdcRealData(pdcDataFrame* rd, int32_t pseudoPdcId);

STRONGRIDIEEEC37118DLL_API int getPmuRealData(pmuDataFrame* rd, PmuStatus* status, int32_t pseudoPdcId, int32_t pmuIndex);

STRONGRIDIEEEC37118DLL_API int getHeaderMsg( char* msg, int maxMsgLength, int32_t pseudoPdcId);

// --------------------- LABVIEW COMPATABILITY FUNCTIONS: All char* arrays must be 256 bytes, or longer  --------------------------

STRONGRIDIEEEC37118DLL_API int getPmuRealDataLabview(noArraysPmuDataFrame* rd, PmuStatus* status,
															uint16_t PhasorArrayLength, float* phasorValueReal, float* phasorValueImaginary,
															uint16_t AnalogArrayLength, float* analogValueArr,
															uint16_t DigitalArrayLength, uint8_t* digitalValueArr,
															int32_t pseudoPdcId, int32_t pmuIndex);

STRONGRIDIEEEC37118DLL_API int getPmuConfigurationLabview(noArraysPmuConfig* pmuconf, char* stationName, int32_t pseudoPdcId, int32_t pmuIndex);

STRONGRIDIEEEC37118DLL_API int getPhasorConfigLabview(noArraysPhasorConfig *phasorCfg, char* name, int32_t pseudoPdcId, int32_t pmuIndex, int32_t phasorIndex);

STRONGRIDIEEEC37118DLL_API int getAnalogConfigLabview(noArraysAnalogConfig *analogCfg, char* name, int32_t pseudoPdcId, int32_t pmuIndex, int32_t analogIndex);

STRONGRIDIEEEC37118DLL_API int getDigitalConfigLabview(noArraysDigitalConfig* digitalCfg, char* name, int32_t pseudoPdcId, int32_t pmuIndex, int32_t digitalIndex );


#endif
