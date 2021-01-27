
#ifndef SAI_DOWNSAMPLE_HPP
#define SAI_DOWNSAMPLE_HPP
#include "usc_dsp_resample_api.h"

class DownSample
{
	std::vector<void*> handle_;
	std::shared_ptr<int16_t> downsample_data_;
	int ch_;
	int in_rate_;
	int out_rate_;
	int size_;
public:
	DownSample();
	~DownSample();
	int init(int ch,int inrate,int outrate,int size);
	void run(std::vector<std::vector<int16_t>> &ch_data);
	int free();
};//class

DownSample::DownSample() {

}

DownSample::~DownSample() {
	for(auto &iter:handle_) {
		if(iter) {
			UscDspReSampleUnInit(iter);
		}
	}

	downsample_data_.reset();

	#ifdef DEBUG
	std::cout<<__FUNCTION__<<std::endl;
	#endif
}

int DownSample::init(int ch,int inrate,int outrate,int size) {
	ch_=ch;
	in_rate_=inrate;
	out_rate_=outrate;
	size_=size;

	handle_.clear();
	for(int i=0;i<ch_;i++) {
		handle_.push_back(UscDspReSampleInit((float)out_rate_/(float)in_rate_,size));
	}

	downsample_data_.reset(new int16_t[size*out_rate_/in_rate_]);

	for(auto &iter:handle_) {
		if(!iter) {
			#ifdef DEBUG
			std::cout<<"downsample handle_:NULL\n";
			#endif
			return -1;
		}
	}
	return 0;
}

void DownSample::run(std::vector<std::vector<int16_t>> &ch_data) {
	for(int i=0;i<ch_;i++) {
		int size=ch_data[i].size();
		downsample(handle_[i],reinterpret_cast<int16_t*>(ch_data[i].data()),size,downsample_data_.get());
		ch_data[i].clear();
		for(int j=0;j<size*out_rate_/in_rate_;j++) {
			ch_data[i].push_back((downsample_data_.get())[j]);
		}
	}
}

int DownSample::free() {
	for(auto &iter:handle_) {
		if(iter) {
			UscDspReSampleUnInit(iter);
		}
	}
	downsample_data_.reset();
}

#endif