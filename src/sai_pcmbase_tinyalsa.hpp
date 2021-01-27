
#ifndef SAI_PCMBASE_TINYALSA_HPP_
#define SAI_PCMBASE_TINYALSA_HPP_

#include <string>
#include <sstream>
#include <vector>
#include <tinyalsa/asoundlib.h>
#include "sai_pcmbase.hpp"

#ifdef __ANDROID__
    #include <android/log.h>
    #define LOG_TAG "tinyalsa"
    #define LOGD(...) ((void)__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, __VA_ARGS__))
    #define LOGE(...) ((void)__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, __VA_ARGS__))
#else
    #define LOGD(fmt, ...) printf(fmt, ##__VA_ARGS__)
    #define LOGE(fmt, ...) printf(fmt, ##__VA_ARGS__)
#endif

class PcmTinyalsa:public PcmBase
{
	struct pcm *pcm_handle_;
	int framesize_;
    int periodsize_;
    int periodcount_;
    std::string errormsg_;
    std::function<void (int,const char*)> error_cb_;
    void *usrdata_;
public:
	PcmTinyalsa();
	~PcmTinyalsa();
	int init(std::string hw,int ch,int bit,int sample_rate) override;
	int read(void *readi_buffer,int size) override;
	void free() override;
    void set_peroidsize(int size) override;
    void set_peroidcount_or_buffersize(int size) override;
    void register_error_cb(void *usrdata,error_cb_func error_cb) override;
private:
	void SplitString(std::string hw,std::vector<std::string> &v);
	int StringInt(std::string str);
};

PcmTinyalsa::PcmTinyalsa()
{
    pcm_handle_=nullptr;
    periodsize_=1024;
    periodcount_=4;
}

PcmTinyalsa::~PcmTinyalsa()
{
	if(pcm_handle_)
		pcm_close(pcm_handle_);

    std::cout<<"[alsa]"<<__FUNCTION__<<std::endl;
    LOGD("exit %s\n",__FUNCTION__);
}

int PcmTinyalsa::init(std::string hw,int ch,int bit,int sample_rate)
{
    std::cout<<"[tinyalsa]hw:"<<hw<<std::endl;
    std::cout<<"[tinyalsa]ch:"<<ch<<std::endl;
    std::cout<<"[tinyalsa]bit:"<<bit<<std::endl;
    std::cout<<"[tinyalsa]sample_rate:"<<sample_rate<<std::endl;

	struct pcm_config _config;
	_config.channels=ch;
	_config.rate=sample_rate;
	_config.period_size=periodsize_;
	_config.period_count=periodcount_;
	_config.start_threshold = 0;
	_config.stop_threshold = 0;
	_config.silence_threshold = 0;
	_config.silence_size = 0;
	_config.format=PCM_FORMAT_S16_LE;

    if(bit==24)
       _config.format=PCM_FORMAT_S24_LE;
    if(bit==32)
        _config.format=PCM_FORMAT_S32_LE;

    std::cout<<"[tinyalsa]period_size:"<<_config.period_size<<std::endl;
    std::cout<<"[tinyalsa]period_count:"<<_config.period_count<<std::endl;
    std::cout<<"[tinyalsa]PCM_FORMAT_S:"<<bit<<"_LE"<<std::endl;

	int _card=0;
	int _dev=0;
	std::vector<std::string> _hw_vector;
	SplitString(hw,_hw_vector);
	if(_hw_vector.size()==3)
	{
		_card=StringInt(_hw_vector[1]);
		_dev=StringInt(_hw_vector[2]);
        std::cout<<"[tinyalsa]card:"<<_card<<" dev:"<<_dev<<std::endl;
	}
	else
	{
        std::cout<<"[tinyalsa]hw split:error"<<std::endl;
        LOGE("hw split:error\n");
	}
	
	pcm_handle_=pcm_open(_card,_dev,PCM_IN,&_config);
	if(!pcm_handle_ || !pcm_is_ready(pcm_handle_))
	{
		std::cout<<"[tinyalsa]pcm_open:error:"<<pcm_get_error(pcm_handle_)<<std::endl;
        LOGE("basex_plugin:%d %s\n",-1,errormsg_.data());
		return -1;
	}

    framesize_=ch*sizeof(int16_t);
    if(bit==24||bit==32)
        framesize_=ch*sizeof(int32_t);

	return 0;
}

int PcmTinyalsa::read(void *readi_buffer,int size)
{
    if(!pcm_handle_)
    {
        return -16;
    }

	int res=pcm_read(pcm_handle_,readi_buffer,size*framesize_);
	if(res!=0)
	{
		std::cout<<"[tinyalsa]pcm_read:error:"<<res<<":"<<pcm_get_error(pcm_handle_)<<std::endl;

        errormsg_=pcm_get_error(pcm_handle_);
        errormsg_+=":pcm_read";
        if(error_cb_) error_cb_(res,errormsg_.data());

        LOGE("basex_plugin:%d %s\n",res,errormsg_.data());
		return res<0?res:-res;
	}

	return size;
}

void PcmTinyalsa::free()
{
	if(pcm_handle_)
		pcm_close(pcm_handle_);
}

void PcmTinyalsa::set_peroidsize(int size)
{
    periodsize_=size;
    std::cout<<"[tinyalsa]set_peroidsize:"<<periodsize_<<std::endl;
    LOGD("set_peroidsize:%d\n",periodsize_);
}

void PcmTinyalsa::set_peroidcount_or_buffersize(int size)
{
    periodcount_=size;
    std::cout<<"[tinyalsa]set_peroidcount:"<<periodcount_<<std::endl;
    LOGD("set_peroidcount:%d\n",periodcount_);
}

void PcmTinyalsa::SplitString(std::string hw,std::vector<std::string> &v)
{
	std::string str;
    for(int i=0;i<hw.size();i++)
    {
        if(hw[i]!=':'&&hw[i]!=',')
        {
            str+=hw[i];
        }

        if(hw[i]==':'||hw[i]==','||i==hw.size()-1)
        {
            if(str.size()!=0)
            {
                v.push_back(str);
                str.clear();
            }
        }
    }
}

int PcmTinyalsa::StringInt(std::string str)
{
	int res;
    std::stringstream ss;
    ss<<str;
    ss>>res;
    return res;
}

void PcmTinyalsa::register_error_cb(void *usrdata,error_cb_func error_cb)
{
    if(error_cb)
    {
        usrdata_=usrdata;
        error_cb_=std::bind(error_cb,usrdata_,std::placeholders::_1,std::placeholders::_2);
    }
}
#endif


