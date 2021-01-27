
#include <iostream>
#include <string>
#include <fstream>
#include <cstring>
#include <cstdlib>
#include <cstdio>
using namespace std;

#include "sai_micbasex_interface.h"

void cstrsplit(char* str,int* res)
{
	const char *delim = ",";
	const char *token = strtok(str, delim);
	if(token)
	{
		int c = 0;
		do 
		{
		  res[c++] = atoi(token);
		} while ((token = strtok(NULL, delim)));
	}
}

static int vFileWrite(const std::string fname,const char* data,int size)
{
    std::fstream fs(fname.data(),std::fstream::out|std::fstream::app);
    if(fs.good())
	{
        fs.write(data,size);
        fs.close();
        return size;
    }
    else
	{
        return -1;
    }
}

int main(int argc,char *argv[])
{
	if(argc<13)
	{
		printf("Usage:%s <runtime> <hw> <mic_num> <ch_num> <ch_map> <bit> <rate> <mic_shift> <ref_shitf> <decode mode:0|1> ",argv[0]);
		printf("<peroidsize> <buffersize or peroidcount> [<delay_channel> <delay_len> <delay_size>]\n");
		printf("Example android:./sample_basex 100 hw:0,0 6 8 0,1,2,3,4,5,6,7 16 16000 16 16 0 512 4 [0,1,2 1000,2000,3000 3]\n");
		printf("Example linux:./sample_basex 100 hw:0,0 6 8 0,1,2,3,4,5,6,7 16 16000 16 16 0 512 2048 [0,1,2 1000,2000,3000 3]\n");
		return 1;
	}

	void *handle;
	int32_t frame=256;

	int runtime=atoi(argv[1]);
	const char* hw=argv[2];
	int16_t mic=atoi(argv[3]);
	int16_t ch=atoi(argv[4]);
	char* chmap=argv[5];
	int bit=atoi(argv[6]);
	int rate=atoi(argv[7]);
	int micshift=atoi(argv[8]);
	int refshift=atoi(argv[9]);
	int mode=atoi(argv[10]);
	int peroidsize=atoi(argv[11]);
	int peroidcount_buffersize=atoi(argv[12]);

	printf("[sample]SaiMicBaseX_Init:before\n");
	handle=SaiMicBaseX_Init(ch,mic,frame,hw);
	if(!handle)
	{
		printf("[sample]SaiMicBaseX_Init:error\n");
		return 1;
	}
	printf("[sample]SaiMicBaseX_Init:ok\n");

	SaiMicBaseX_RegisterErrorCb(handle,NULL,[](void *userdata,int code,const char *msg)
	{
		printf("[sample][error_cb]code:%d msg:%s\n",code,msg);
	});
	
	int ch_map[8];
	cstrsplit(chmap,ch_map);
	SaiMicBaseXSetChannelMap(handle,ch_map,ch);
	SaiMicBaseX_SetBit(handle,bit);
	SaiMicBaseX_SetSampleRate(handle,rate);
	SaiMicBaseX_SetMicShiftBits(handle,micshift);
	SaiMicBaseX_SetRefShiftBits(handle,refshift);
	SaiMicBaseX_SetDecodeMode(handle,mode);
	SaiMicBaseX_SetPeroidSize(handle,peroidsize);
	SaiMicBaseX_SetBufferSize(handle,peroidcount_buffersize);

	if(argc==16)
	{
		int delay_channel[8];
		cstrsplit(argv[13],delay_channel);

		int delay_len[8];
		cstrsplit(argv[14],delay_len);

		int delay_size=atoi(argv[15]);

		SaiMicBaseX_SetDelayChannel(handle,delay_channel,delay_len,delay_size);
	}
	if(SaiMicBaseX_Start(handle) == 0)
	{
		printf("[sample]start basex success\n");
	}
	else
	{
		printf("[sample]start basex failed!\n");
		return 1;
	}
	///////////////////////////////
	remove("record.pcm");
	int loop=runtime;
	while(loop--)
	{
		int16_t *data=nullptr;
		int res=SaiMicBaseX_ReadData(handle,&data);
		printf("[sample]loop:%d res:%d\n",loop,res);

		if(res<=0)
		{
			printf("[sample]SaiMicBaseX_ReadData:error %d\n",res);
			return 1;
		}

		int size=vFileWrite("record.pcm",reinterpret_cast<const char*>(data),res<<1);
		printf("[sample]write to file:%d Byte\n",size);
	}

	/////////////////////////
	printf("[sample]release before\n");
	SaiMicBaseX_Release(handle);
	printf("[sample]release after\n");

	return 0;
}
