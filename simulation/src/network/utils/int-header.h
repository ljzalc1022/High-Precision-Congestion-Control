#ifndef INT_HEADER_H
#define INT_HEADER_H

#include "ns3/buffer.h"
#include <stdint.h>
#include <cstdio>

namespace ns3 {

class IntHop{
public:
	static const uint32_t timeWidth = 24;
	static const uint32_t bytesWidth = 20;
	static const uint32_t qlenWidth = 16;
	static const uint32_t bigflowWidth = 1;
	static const uint64_t lineRateValues[8];
	union{
		struct {
			uint64_t lineRate: 64-timeWidth-bytesWidth-qlenWidth-bigflowWidth,
					 time: timeWidth,
					 bytes: bytesWidth,
					 qlen: qlenWidth,
					 bigflow: bigflowWidth;
		};
		uint32_t buf[2];
	};

	static const uint32_t byteUnit = 128;
	static const uint32_t qlenUnit = 80;
	static uint32_t multi;

	uint64_t GetLineRate(){
		return lineRateValues[lineRate];
	}
	uint64_t GetBytes(){
		return (uint64_t)bytes * byteUnit * multi;
	}
	uint32_t GetQlen(){
		return (uint32_t)qlen * qlenUnit * multi;
	}
	uint64_t GetTime(){
		return time;
	}
	void Set(uint64_t _time, uint64_t _bytes, uint32_t _qlen, uint64_t _rate){
		time = _time;
		bytes = _bytes / (byteUnit * multi);
		qlen = _qlen / (qlenUnit * multi);
		switch (_rate){
			case 25000000000lu:
				lineRate=0;break;
			case 50000000000lu:
				lineRate=1;break;
			case 100000000000lu:
				lineRate=2;break;
			case 200000000000lu:
				lineRate=3;break;
			case 400000000000lu:
				lineRate=4;break;
			case 2000000000lu:
				lineRate=5;break;
			case 10000000000lu:
				lineRate=6;break;
			default:
				printf("Error: IntHeader unknown rate: %lu\n", _rate);
				break;
		}
	}
	uint64_t GetBytesDelta(IntHop &b){
		if (bytes >= b.bytes)
			return (bytes - b.bytes) * byteUnit * multi;
		else
			return (bytes + (1<<bytesWidth) - b.bytes) * byteUnit * multi;
	}
	uint64_t GetTimeDelta(IntHop &b){
		if (time >= b.time)
			return time - b.time;
		else
			return time + (1<<timeWidth) - b.time;
	}

public:
	static const int rateWidth = 48;
	union {
		struct {
			uint64_t targetRate: rateWidth,
					 card0: 64 - rateWidth;
		};
		struct {
			uint64_t heavyBytes: bytesWidth,
					 card1: 64 - bytesWidth; // number of flows on the link
		};
		uint32_t myBuf[2];
	};

	uint64_t GetHBytesDelta(IntHop &b) {
		if (heavyBytes >= b.heavyBytes)
			return (heavyBytes - b.heavyBytes) * byteUnit;
		else 
			return (heavyBytes + (1<<bytesWidth) - b.heavyBytes) * byteUnit;
	}
	void SetMyCC(uint64_t _targetRate, uint64_t _heavyBytes, uint32_t _card) {
	//	targetRate = _targetRate;
	//	card0 = _card;
		heavyBytes = _heavyBytes / byteUnit;
		card1 = _card;
	}
};

// class Portion {
// public:
// 	union {
// 		float p;
// 		uint32_t buf;
// 	};
// };

class IntHeader{
public:
	static const uint32_t maxHop = 5;
	enum Mode{
		NORMAL = 0,
		TS = 1,
		PINT = 2,
		NONE
	};
	static Mode mode;
	static int pint_bytes;

	// Note: the structure of IntHeader must have no internal padding, 
	// because we will directly transform the part of packet buffer to IntHeader*
	union{
		struct {
			IntHop hop[maxHop];
			uint16_t nhop;
			// uint16_t bigflowMark;
			// uint32_t portion;
			uint16_t magic_option; // 1 + 5 * 3 = 16
			// Portion portion[maxHop];
		};
		uint64_t ts;
		union {
			uint16_t power;
			struct{
				uint8_t power_lo8, power_hi8;
			};
		}pint;
	};

	IntHeader();
	static uint32_t GetStaticSize();
	void PushHop(uint64_t time, uint64_t bytes, uint32_t qlen, uint64_t rate, 
				 bool isBigflow, uint64_t heavyBytes, uint64_t targetRate, uint32_t card, uint16_t p);
	void Serialize (Buffer::Iterator start) const;
	uint32_t Deserialize (Buffer::Iterator start);
	uint64_t GetTs(void);
	uint16_t GetPower(void);
	void SetPower(uint16_t);
};

}

#endif /* INT_HEADER_H */
