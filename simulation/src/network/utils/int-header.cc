#include "int-header.h"

namespace ns3 {

// const uint64_t IntHop::lineRateValues[8] = {25000000000lu,50000000000lu,100000000000lu,200000000000lu,400000000000lu,0,0,0};
const uint64_t IntHop::lineRateValues[8] = {25000000000lu,50000000000lu,100000000000lu,200000000000lu,400000000000lu, 2000000000lu, 10000000000lu, 0};
uint32_t IntHop::multi = 1;

IntHeader::Mode IntHeader::mode = NONE;
int IntHeader::pint_bytes = 2;

IntHeader::IntHeader() : 
	// nhop(0), bigflowMark(0), portion{0} 
	nhop(0)
	, magic_option(0)
{
	for (uint32_t i = 0; i < maxHop; i++)
		hop[i] = {0};
	// for (uint32_t i = 0; i < maxHop; i++)
	// 	portion[i].p = {0};
}

uint32_t IntHeader::GetStaticSize(){
	if (mode == NORMAL){
		// return sizeof(hop) + sizeof(nhop) + sizeof(bigflowMark) + sizeof(portion);
		return sizeof(hop) + sizeof(nhop) + sizeof (magic_option);
		// return sizeof(hop) + sizeof(nhop);
	}else if (mode == TS){
		return sizeof(ts);
	}else if (mode == PINT){
		return sizeof(pint);
	}else {
		return 0;
	}
}

void IntHeader::PushHop(uint64_t time, uint64_t bytes, uint32_t qlen, uint64_t rate, 
						bool isBigflow, uint64_t Rb, uint64_t R, uint32_t card, 
						uint16_t p) {
	// only do this in INT mode
	if (mode == NORMAL){
		uint32_t idx = nhop % maxHop;
		hop[idx].Set(time, bytes, qlen, rate);
		hop[idx].bigflow = isBigflow ? 1 : 0;
		hop[idx].Rb = Rb;
		hop[idx].R = R;
		hop[idx].card = card;
		nhop++;
		magic_option |= (isBigflow ? 1 : 0);
		magic_option |= ((p % 8) << (3 * idx + 1));
	}
}

void IntHeader::Serialize (Buffer::Iterator start) const{
	Buffer::Iterator i = start;
	if (mode == NORMAL){
		for (uint32_t j = 0; j < maxHop; j++){
			i.WriteU32(hop[j].buf[0]);
			i.WriteU32(hop[j].buf[1]);
			i.WriteU64(hop[j].Rb);
			i.WriteU64(hop[j].R);
			i.WriteU64(hop[j].card);
		}
		i.WriteU16(nhop);
		// i.WriteU16(bigflowMark);
		i.WriteU16(magic_option);
		// for (uint32_t j = 0; j < maxHop; j++){
		// 	i.WriteU32(portion[j].buf);
		// }
		// i.WriteU32(portion);
	}else if (mode == TS){
		i.WriteU64(ts);
	}else if (mode == PINT){
		if (pint_bytes == 1)
			i.WriteU8(pint.power_lo8);
		else if (pint_bytes == 2)
			i.WriteU16(pint.power);
	}
}

uint32_t IntHeader::Deserialize (Buffer::Iterator start){
	Buffer::Iterator i = start;
	if (mode == NORMAL){
		for (uint32_t j = 0; j < maxHop; j++){
			hop[j].buf[0] = i.ReadU32();
			hop[j].buf[1] = i.ReadU32();
			hop[j].Rb = i.ReadU64();
			hop[j].R = i.ReadU64();
			hop[j].card = i.ReadU64();
		}
		nhop = i.ReadU16();
		magic_option = i.ReadU16();
		// bigflowMark = i.ReadU16();
		// for (uint32_t j = 0; j < maxHop; j++){
		// 	portion[j].buf = i.ReadU32();
		// };
		// portion = i.ReadU32();
	}else if (mode == TS){
		ts = i.ReadU64();
	}else if (mode == PINT){
		if (pint_bytes == 1)
			pint.power_lo8 = i.ReadU8();
		else if (pint_bytes == 2)
			pint.power = i.ReadU16();
	}
	return GetStaticSize();
}

uint64_t IntHeader::GetTs(void){
	if (mode == TS)
		return ts;
	return 0;
}

uint16_t IntHeader::GetPower(void){
	if (mode == PINT)
		return pint_bytes == 1 ? pint.power_lo8 : pint.power;
	return 0;
}
void IntHeader::SetPower(uint16_t power){
	if (mode == PINT){
		if (pint_bytes == 1)
			pint.power_lo8 = power;
		else
			pint.power = power;
	}
}

}
