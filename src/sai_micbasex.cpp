//
// Created by luantianxiang on 10/25/18.
//
#include <cstdio>
#include <sys/stat.h>
#include <unistd.h>

#include <string>
#include <vector>
#include <deque>
#include <iostream>
#include <fstream>
#include <sstream>
#include <memory>
#include <functional>

#include "sai_downsample.hpp"
#include "version.hpp"

#define SAMPLESIZE (256)

#ifdef PCMALSA
#include "sai_pcmbase_alsa.hpp"
#define RECORDFILE ("/tmp/savebasex.pcm")
#endif

#ifdef PCMTINYALSA
#include "sai_pcmbase_tinyalsa.hpp"
#define RECORDFILE ("/sdcard/savebasex.pcm")
#endif

namespace sai{
class SaiMicBaseX
{
	int ch_;
	int bit_;
	int mic_;
	int frame_;
	std::string hw_;
	int sample_rate_;
	int mic_shift_bits_;
	int ref_shift_bits_;
	std::vector<int> ch_map_;
	std::vector<std::vector<int16_t>> data_ch_;
	std::vector<int16_t> data_out_;
	bool open_decode_;

	std::shared_ptr<char> buffer_;
	std::shared_ptr<PcmBase> pcmbase_;
	std::shared_ptr<DownSample> dsample_;

	bool open_delay_;
	std::vector<int> delay_channel_;
	std::vector<int> delay_len_;
	std::vector<std::deque<int16_t>> delay_deque_;

public:
	explicit SaiMicBaseX(int ch,int mic,int frame,const char *hw);
	~SaiMicBaseX();
	int read_data(int16_t** data);
	int start_basex();
	void reset();
	void set_sample_rate(int rate);
	void set_bit(int b);
	void set_channel_map(const int *true_ch,int num_ch);
	void set_mic_shift_bits(int bits);
	void set_ref_shift_bits(int bits);
	void register_error_cb(void *usrdata,error_cb_func error_cb);
	void set_decode_mode(int mode);
	void set_peroidcount_or_buffersize(int size);
    void set_peroidsize(int size);
    void set_delay_channel(const int* delay_channel,const int* delay_len,int size);
private:
	std::string StringChmap(std::vector<int>ch_map);
	int ValueIndex(std::vector<int> v,int value);
	int FileSize(const std::string fname);
	int FileExist(const std::string fname);
	int FileRemove(const std::string fname);
	void WriteToFile(const std::string &data,const std::string fname);
};//class

SaiMicBaseX::SaiMicBaseX(int ch,int mic,int frame,const char *hw)
{
	ch_=ch;mic_=mic;frame_=frame;hw_=hw;

	bit_=16;
	sample_rate_=16000;
	mic_shift_bits_=16;
	ref_shift_bits_=16;
	open_decode_=false;
	open_delay_=false;

	ch_map_=std::vector<int>(ch_);
	for(int16_t i = 0; i < ch_; ++i)
	{
		ch_map_[i] = i;
	}

	data_ch_=std::vector<std::vector<int16_t>>(ch_);
	
	buffer_.reset(new char[SAMPLESIZE*ch_*sizeof(int32_t)]);

	#ifdef PCMALSA
	pcmbase_.reset(new PcmAlsa());
	#endif

	#ifdef PCMTINYALSA
	pcmbase_.reset(new PcmTinyalsa());
	#endif
}

SaiMicBaseX::~SaiMicBaseX() 
{
	buffer_.reset();
	pcmbase_.reset();
	dsample_.reset();
	std::cout<<"[micbasex]"<<__FUNCTION__<<std::endl;
}

int SaiMicBaseX::start_basex()
{
	std::cout<<"[micbasex]"<<hw_<<std::endl;
	std::cout<<"[micbasex]mic_num:"<<mic_<<std::endl;
	std::cout<<"[micbasex]frame:"<<frame_<<std::endl;
	std::cout<<"[micbasex]bit:"<<bit_<<std::endl;
	std::cout<<"[micbasex]rate:"<<sample_rate_<<std::endl;
	std::cout<<"[micbasex]mic_shift_bits:"<<mic_shift_bits_<<std::endl;
	std::cout<<"[micbasex]ref_shift_bits:"<<ref_shift_bits_<<std::endl;
	std::cout<<"[micbasex]ch_map:"<<StringChmap(ch_map_)<<std::endl;

	dsample_.reset(new DownSample());
	int res=dsample_->init(ch_,sample_rate_,16000,frame_*3);
	if(res!=0)
	{
		std::cout<<"[micbasex]SaiMicBaseX dsample init:error "<<res<<std::endl;
		return res;
	}
	else
		std::cout<<"[micbasex]SaiMicBaseX dsample init:ok "<<res<<std::endl;
	
	res=pcmbase_->init(hw_,ch_,bit_,sample_rate_);
	if(res!=0)
		std::cout<<"[micbasex]SaiMicBaseX pcmbase init:error "<<res<<std::endl;
	else
		std::cout<<"[micbasex]SaiMicBaseX pcmbase init:ok "<<res<<std::endl;
	return res;
}

void SaiMicBaseX::set_delay_channel(const int* delay_channel,const int* delay_len,int size)
{
	if(!delay_channel||!delay_len||!size)
	{
		std::cout<<"[micbasex]set_delay_channel:false\n";
		return;
	}

	open_delay_=true;
	delay_deque_=std::vector<std::deque<int16_t>>(size);
	for(int i=0;i<size;i++)
	{
		delay_channel_.push_back(delay_channel[i]);
		delay_len_.push_back(delay_len[i]);
		delay_deque_[i]=std::deque<int16_t>(delay_len_[i],0);
	}
	std::cout<<"[SaiMicBaseX]set_delay_channel delay_channel:"<<StringChmap(delay_channel_)<<std::endl;
	std::cout<<"[SaiMicBaseX]set_delay_channel delay_len:"<<StringChmap(delay_len_)<<std::endl;
}

int SaiMicBaseX::read_data(int16_t** data) 
{
	for(auto &iter:data_ch_)
	{
		if(!iter.empty())
		{
			iter.clear();
		}
	}

	while(data_ch_[0].size()<frame_*sample_rate_/16000)
	{
		int res=pcmbase_->read(buffer_.get(),SAMPLESIZE);
		if(res<=0)
		{
			return res;
		}
		
		for(int j=0;j<res*ch_;j++)
		{
			int16_t tmpdata=0;
			if(bit_==16)
			{
				if(open_decode_)
				{
					unsigned short magicNum=0x59DF;
					tmpdata=(reinterpret_cast<int16_t*>(buffer_.get()))[j];
					tmpdata=tmpdata==0?tmpdata:tmpdata^magicNum;
				}
				else
				{
					tmpdata=(reinterpret_cast<int16_t*>(buffer_.get()))[j];
				}
			}

			if(bit_==24||bit_==32)
			{
				if(ch_map_[j%ch_]<mic_)
					tmpdata=(reinterpret_cast<int32_t*>(buffer_.get()))[j]>>mic_shift_bits_;
				else
					tmpdata=(reinterpret_cast<int32_t*>(buffer_.get()))[j]>>ref_shift_bits_;
			}

			data_ch_[j%ch_].push_back(tmpdata);
		}
	}//while

	if(sample_rate_!=16000)
	{
		dsample_->run(data_ch_);
	}
	
	if(!data_out_.empty())
	{
		data_out_.clear();
	}

	for(int i=0;i<data_ch_[0].size();i++)
	{
		for(int j=0;j<ch_;j++)
		{
			if(!open_delay_)
			{
				data_out_.push_back(data_ch_[ValueIndex(ch_map_,j)][i]);
			}
			else
			{
				int index=ValueIndex(delay_channel_,j);
				if(index<0)
				{
					data_out_.push_back(data_ch_[ValueIndex(ch_map_,j)][i]);
				}
				else
				{
					data_out_.push_back(*(delay_deque_[index].begin()));
					delay_deque_[index].pop_front();
					delay_deque_[index].push_back(data_ch_[ValueIndex(ch_map_,j)][i]);
				}
			}
		}
	}

	*data=data_out_.data();
	int datasize=data_out_.size();

	#ifdef WRITE_DATA_FILE
	if(FileExist(RECORDFILE)==0)
	{
		std::string rawdata_(reinterpret_cast<char*>(data_out_.data()),data_out_.size()*2);
		#ifndef DEBUG
		if(FileSize(RECORDFILE)>20971520)
		{
			FileRemove(RECORDFILE);
		}
		#endif
		WriteToFile(rawdata_,RECORDFILE);
	}
	#endif

	return datasize;	
}

void SaiMicBaseX::reset() {
	
}

void SaiMicBaseX::set_sample_rate(int rate) 
{
	if(sample_rate_==rate || rate<=0)
		return;
	
	if(rate == 16000 || rate == 48000) 
	{
		sample_rate_ = rate;
	}
}

void SaiMicBaseX::set_bit(int b)
{
	if(bit_==b || b<=0)
		return;
	bit_ = b;
}

void SaiMicBaseX::set_channel_map(const int * true_ch, int num_ch)
{
	if(!true_ch) return;
	int ch_sum = std::min(num_ch, static_cast<int>(ch_));
	for(int i = 0; i < ch_sum; ++i)
	{
		ch_map_[i] = true_ch[i];
	}

	std::cout<<"[SaiMicBaseX]set_channel_map ch_map:"<<StringChmap(ch_map_)<<std::endl;
}

void SaiMicBaseX::set_mic_shift_bits(int bits)
{
	if(bits>0 && bits <= 32)
	{
		mic_shift_bits_ = bits;
		std::cout<<"[SaiMicBaseX]set_mic_shift_bits mic_shift_bits:"<<mic_shift_bits_<<std::endl;
	}
}

void SaiMicBaseX::set_ref_shift_bits(int bits)
{
	if(bits>0 && bits <= 32)
	{
		ref_shift_bits_ = bits;
		std::cout<<"[SaiMicBaseX]set_ref_shift_bits ref_shift_bits:"<<ref_shift_bits_<<std::endl;
	}
}

void SaiMicBaseX::register_error_cb(void *usrdata,error_cb_func error_cb)
{
	if(pcmbase_)
	{
		pcmbase_->register_error_cb(usrdata,error_cb);
	}
}

void SaiMicBaseX::set_peroidsize(int size)
{
	if(pcmbase_)
	{
		pcmbase_->set_peroidsize(size);
	}
}

void SaiMicBaseX::set_peroidcount_or_buffersize(int size)
{
	if(pcmbase_)
	{
		pcmbase_->set_peroidcount_or_buffersize(size);
	}
}

void SaiMicBaseX::set_decode_mode(int mode)
{
	open_decode_=mode>0?true:false;
}

////
std::string SaiMicBaseX::StringChmap(std::vector<int>ch_map)
{
	std::stringstream ss;
	for(int i=0;i<ch_map.size();i++)
	{
		if(i==ch_map.size()-1)
		{
			ss<<ch_map[i];
		}
		else
		{
			ss<<ch_map[i]<<",";
		}
	}
	return ss.str();
}

int SaiMicBaseX::ValueIndex(std::vector<int> v,int value)
{
	for(int i=0;i<v.size();i++)
	{
		if(v[i]==value)
		{
			return i;
		}
	}
	return -1;
}

int SaiMicBaseX::FileSize(const std::string fname)
{
    struct stat statbuf;
    if(stat(fname.data(),&statbuf)==0) {
        return statbuf.st_size;
    }
    return -1;
}

int SaiMicBaseX::FileExist(const std::string fname)
{
    return access(fname.data(),0);
}

int SaiMicBaseX::FileRemove(const std::string fname)
{
	return remove(fname.data());
}

void SaiMicBaseX::WriteToFile(const std::string &data,const std::string fname)
{
	std::fstream fs(fname.data(),std::fstream::out|std::fstream::app);
    if(fs.good())
	{
        fs.write(data.data(),data.size());
        fs.close();
    }
}

} // namespace sai


//c-interface
void* SaiMicBaseX_Init(int16_t ch, int16_t mic, int32_t frame, const char *hw)
{
	sai::SaiMicBaseX* micbasex = new sai::SaiMicBaseX(ch, mic, frame, hw);
	return reinterpret_cast<void*>(micbasex);
}

int SaiMicBaseX_Start(void *handle)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		return micbasex->start_basex();
	}
	else
	{
		std::cout<<"[SaiMicBaseX]pcmbase is not init"<<std::endl;
		return 1;
	}
	
}
int32_t SaiMicBaseX_ReadData(void *handle, int16_t** data)
{
	if(!handle || !data) return 0;
	sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
	return micbasex->read_data(data);
}

void SaiMicBaseX_Reset(void *handle)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->reset();
	}
}

void SaiMicBaseX_Release(void *handle)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		delete micbasex;
	}
}

void SaiMicBaseX_SetSampleRate(void *handle, int sample_rate)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_sample_rate(sample_rate);
	}
}

void SaiMicBaseX_SetBit(void *handle, int bit)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_bit(bit);
	}
}

void SaiMicBaseXSetChannelMap(void *handle, const int * true_ch, int num_ch)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_channel_map(true_ch, num_ch);
	}
}

void SaiMicBaseX_SetMicShiftBits(void *handle, int bits)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_mic_shift_bits(bits);
	}
}

void SaiMicBaseX_SetRefShiftBits(void *handle, int bits)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_ref_shift_bits(bits);
	}
}

void SaiMicBaseX_RegisterErrorCb(void *handle,void *usrdata,error_cb_func error_cb)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->register_error_cb(usrdata,error_cb);
	}
}

void SaiMicBaseX_SetDecodeMode(void *handle,int mode)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_decode_mode(mode);
	}
}

void SaiMicBaseX_SetPeroidSize(void *handle,int size)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_peroidsize(size);
	}
}

void SaiMicBaseX_SetBufferSize(void *handle,int size)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_peroidcount_or_buffersize(size);
	}
}

void SaiMicBaseX_SetDelayChannel(void *handle,const int* delay_channel,const int* delay_len,int size)
{
	if(handle)
	{
		sai::SaiMicBaseX* micbasex = reinterpret_cast<sai::SaiMicBaseX*>(handle);
		micbasex->set_delay_channel(delay_channel,delay_len,size);
	}
}

const char* SaiMicBaseX_GetVersion() 
{
	return basex_getver;
}


