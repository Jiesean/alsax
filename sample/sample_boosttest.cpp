
#define BOOST_TEST_MODULE test_pcmalsa
#include <boost/test/included/unit_test.hpp>
using namespace boost;

#include "sai_micbasex_interface.h"
using namespace std;

BOOST_AUTO_TEST_SUITE(suite1)

BOOST_AUTO_TEST_CASE(case1)
{
	void *handle;
	int ch=8;
	int mic=8;
	int frame=1024;
	char hw[]="default";
	//char hw[]="hw:0,0";

	handle=SaiMicBaseX_Init(ch,mic,frame,hw);
	BOOST_REQUIRE(handle);
	BOOST_TEST_MESSAGE("SaiMicBaseX_Init:ok");

	SaiMicBaseX_SetBit(handle,16);
	BOOST_TEST_MESSAGE("SaiMicBaseX_SetBit:"<<16);
	SaiMicBaseX_SetSampleRate(handle,16000);
	BOOST_TEST_MESSAGE("SaiMicBaseX_SetSampleRate:"<<16000);

	int res;
	int16_t *data;

	int loop=1000;
	while(loop--)
	{
		res=SaiMicBaseX_ReadData(handle,&data);
		BOOST_CHECK_GT(res,0);
		BOOST_TEST_MESSAGE("SaiMicBaseX_ReadData:res "<<res);
		BOOST_CHECK(data);
		BOOST_TEST_MESSAGE("SaiMicBaseX_ReadData:data ok");
		BOOST_TEST_MESSAGE("loop:"<<loop);

		if(loop==800)
		{
			SaiMicBaseX_SetBit(handle,32);
			BOOST_TEST_MESSAGE("SaiMicBaseX_SetBit:"<<32);
			SaiMicBaseX_SetSampleRate(handle,16000);
			BOOST_TEST_MESSAGE("SaiMicBaseX_SetSampleRate:"<<16000);
		}

		if(loop==600)
		{
			SaiMicBaseX_SetBit(handle,16);
			BOOST_TEST_MESSAGE("SaiMicBaseX_SetBit:"<<16);
			SaiMicBaseX_SetSampleRate(handle,48000);
			BOOST_TEST_MESSAGE("SaiMicBaseX_SetSampleRate:"<<48000);
		}

		if(loop==400)
		{
			SaiMicBaseX_SetBit(handle,32);
			BOOST_TEST_MESSAGE("SaiMicBaseX_SetBit:"<<32);
			SaiMicBaseX_SetSampleRate(handle,48000);
			BOOST_TEST_MESSAGE("SaiMicBaseX_SetSampleRate:"<<48000);
		}
	}

	SaiMicBaseX_Release(handle);
	BOOST_TEST_MESSAGE("SaiMicBaseX_Release");
}

BOOST_AUTO_TEST_SUITE_END()
