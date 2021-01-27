
#ifndef SAI_PCMBASE_HPP_
#define SAI_PCMBASE_HPP_

#include <string>
#include "sai_micbasex_interface.h"

class PcmBase
{
public:
	PcmBase();
	virtual ~PcmBase();
	virtual int init(std::string hw_,int ch_,int bit_,int sample_rate_)=0;
	virtual int read(void *readi_buffer_,int size)=0;
	virtual void free()=0;
    virtual void set_peroidsize(int size)=0;
    virtual void set_peroidcount_or_buffersize(int size)=0;
    virtual void register_error_cb(void *usrdata,error_cb_func error_cb)=0;
};//class

PcmBase::PcmBase(){}

PcmBase::~PcmBase()
{
    #ifdef DEBUG
    std::cout<<__FUNCTION__<<std::endl;
    #endif
}

#endif
