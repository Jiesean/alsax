
#ifndef SAI_PCMBASE_ALSA_HPP_
#define SAI_PCMBASE_ALSA_HPP_

#include <string>
#include <alsa/asoundlib.h>
#include "sai_pcmbase.hpp"
#include <syslog.h>

class PcmAlsa:public PcmBase
{
	snd_pcm_t *pcm_handle_;
	snd_pcm_hw_params_t *params;
	snd_pcm_uframes_t buffersize_;
	snd_pcm_uframes_t periodsize_;
	std::string errormsg_;
	std::function<void (int,const char*)> error_cb_;
	void *usrdata_;
public:
	PcmAlsa();
	~PcmAlsa();
	int init(std::string hw_,int ch_,int bit_,int sample_rate_) override;
	int read(void *readi_buffer_,int size) override;
	void free() override;
	void set_peroidsize(int size) override;
    void set_peroidcount_or_buffersize(int size) override;
    void register_error_cb(void *usrdata,error_cb_func error_cb) override;
};

PcmAlsa::PcmAlsa()
{
	pcm_handle_=nullptr;
	buffersize_=4096;
	periodsize_=1024;
}

PcmAlsa::~PcmAlsa()
{
	if(pcm_handle_)
	{
		snd_pcm_close(pcm_handle_);
		snd_config_update_free_global();
	}

	std::cout<<"[alsa]"<<__FUNCTION__<<std::endl;
}

int PcmAlsa::init(std::string hw_,int ch_,int bit_,int sample_rate_)
{
	std::cout<<"[alsa]hw:"<<hw_<<std::endl;
    std::cout<<"[alsa]ch:"<<ch_<<std::endl;
    std::cout<<"[alsa]bit:"<<bit_<<std::endl;
    std::cout<<"[alsa]sample_rate:"<<sample_rate_<<std::endl;

	int res;
	res=snd_pcm_open(&pcm_handle_,hw_.data(),SND_PCM_STREAM_CAPTURE, SND_PCM_ASYNC);
	if(res<0)
	{
		std::cout<<"[alsa]snd_pcm_open:error:"<<res<<":"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}
	
	snd_pcm_hw_params_alloca(&params);
	res=snd_pcm_hw_params_any(pcm_handle_,params);
	if(res<0)
	{
		std::cout<<"[alsa]params any:error:"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}

	res=snd_pcm_hw_params_set_access(pcm_handle_,params,SND_PCM_ACCESS_RW_INTERLEAVED);
	if(res<0)
	{
		std::cout<<"[alsa]set_access:error:"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}

	res=snd_pcm_hw_params_set_channels(pcm_handle_,params,ch_);
	if(res<0)
	{
		std::cout<<"[alsa]set_channels:error:"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}

	snd_pcm_format_t bit_format=SND_PCM_FORMAT_S16_LE;
	if(bit_==24)
		bit_format=SND_PCM_FORMAT_S24_LE;
	if(bit_==32)
		bit_format=SND_PCM_FORMAT_S32_LE;
	res=snd_pcm_hw_params_set_format(pcm_handle_,params,bit_format);
	if(res<0)
	{
		std::cout<<"[alsa]set_format:error:"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}

	int dir;
	unsigned int val=sample_rate_;
	res=snd_pcm_hw_params_set_rate_near(pcm_handle_,params,&val,&dir);
	if(res<0)
	{
		std::cout<<"[alsa]set_rate:error:"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}
	
	snd_pcm_uframes_t buffersize = buffersize_;
	if (buffersize != 0) 
	{
		res=snd_pcm_hw_params_set_buffer_size_near(pcm_handle_,params,&buffersize);
		if(res<0)
		{
			std::cout<<"[alsa]set_buffer:error:"<<snd_strerror(res)<<std::endl;
			syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
			return res;
		}
		
		snd_pcm_uframes_t get_buffersize;
		snd_pcm_hw_params_get_buffer_size(params,&get_buffersize);
		
		std::cout<<"[alsa]set_buffersize:"<<buffersize<<std::endl;
		std::cout<<"[alsa]get_buffersize:"<<get_buffersize<<std::endl;

		if (get_buffersize<=0) {
			std::cout<<"[alsa]return"<<std::endl;
    		return -1;
		}
	}

	snd_pcm_uframes_t periodsize = periodsize_;
	if (periodsize != 0) 
	{
		res=snd_pcm_hw_params_set_period_size_near(pcm_handle_,params,&periodsize,&dir);
		if(res<0)
		{
			std::cout<<"[alsa]set_period:error"<<snd_strerror(res)<<std::endl;
			syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
			return res;
		}
		
		snd_pcm_uframes_t get_periodsize;
		snd_pcm_hw_params_get_period_size(params,&get_periodsize,0);
		
		std::cout<<"[alsa]set_periodsize:"<<periodsize<<std::endl;
		std::cout<<"[alsa]get_periodsize:"<<get_periodsize<<std::endl;

		if (get_periodsize<=0) {
			std::cout<<"[alsa]return"<<std::endl;
    		return -1;
		}
	}

	res=snd_pcm_hw_params(pcm_handle_,params);
	if(res<0)
	{
		std::cout<<"[alsa]snd_pcm_hw_params:error:"<<res<<":"<<snd_strerror(res)<<std::endl;
		syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
		return res;
	}

	return 0;
}

int PcmAlsa::read(void *readi_buffer_,int size)
{
	if(!pcm_handle_)
	{
		return -16;
	}

	int res=snd_pcm_readi(pcm_handle_,readi_buffer_,size);
	if(res<0)
	{
		if(res==-EPIPE)
		{
			std::cout<<"[alsa]snd_pcm_readi:error:-EPIPE:"<<res<<":"<<snd_strerror(res)<<std::endl;

			errormsg_=snd_strerror(res);
			errormsg_+=":snd_pcm_readi";
			if(error_cb_) error_cb_(res,errormsg_.data());
			
			res=snd_pcm_prepare(pcm_handle_);
			if(res<0)
			{
				errormsg_=snd_strerror(res);
				errormsg_+=":snd_pcm_prepare";
				if(error_cb_) error_cb_(res,errormsg_.data());
				
				syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());
			}

			return -EPIPE;
		}
		else
		{
			std::cout<<"[alsa]snd_pcm_readi:error:"<<res<<":"<<snd_strerror(res)<<std::endl;
		
			errormsg_=snd_strerror(res);
			errormsg_+=":snd_pcm_readi";
			if(error_cb_) error_cb_(res,errormsg_.data());
			
			syslog(LOG_ERR,"basex_plugin:%d %s",res,errormsg_.data());

			return res;
		}
	}
	return res;
}

void PcmAlsa::free()
{
	if(pcm_handle_)
	{
		snd_pcm_close(pcm_handle_);
		snd_config_update_free_global();
	}
}

void PcmAlsa::set_peroidsize(int size)
{
	periodsize_=size;
	std::cout<<"[alsa]set_peroidsize:"<<periodsize_<<std::endl;
}

void PcmAlsa::set_peroidcount_or_buffersize(int size)
{
	buffersize_=size;
	std::cout<<"[alsa]set_buffersize:"<<buffersize_<<std::endl;
}

void PcmAlsa::register_error_cb(void *usrdata,error_cb_func error_cb)
{
	if(error_cb)
	{
		usrdata_=usrdata;
		error_cb_=std::bind(error_cb,usrdata_,std::placeholders::_1,std::placeholders::_2);
	}
}
#endif
