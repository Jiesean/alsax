
#ifndef __USC_DSP_RESAMPLE_API_H__
#define __USC_DSP_RESAMPLE_API_H__

#if defined(__cplusplus)
extern "C"
{
#endif
	//Initialize resample Object;
	//param: 
	//  factor[in]: <out sample rate>/<in sample rate>
	//  data_len[in]: segment length(sample point) of voice data sended to resample object
	//return:
	//  handle of resample object, NULL if initialization fail;
	void* UscDspReSampleInit(float factor, int data_len);

	//void downsample(void *handle1,void *handle2,void *handle3,void *handle4, short *In,int In_len, short *Out);
	void downsample(void *handle, short *In,int In_len, short *Out);

	//Relsease resample library;
	//params:
	//  handle[in]: handle of resample object;
	void UscDspReSampleUnInit(void* handle);

	//Reset resample library;
	//params:
	//  handle[in]: handle of resample object;
	void UscDspReSampleReset(void* handle);

	//Send voice data to resample object, and get resample data;
	//Caller is responsible for the allocation of data buffer
	//Length of input data must be the same as passed to UscDspReSampleInit
	//Length of output data >= <input length> * factor
	//params:
	//  handle[in]: handle of resample object;
	//  in1[in]: voice data segment of channle 1;
	//  in2[in]: voice data segment of channle 2;
	//  out1[in]: resampled voice data segment of channle 1;
	//  out2[in]: resampled voice data segment of channle 2;
	//return:
	//  segment length(sample point) of ressampled voice data
	unsigned int UscDspReSamplePush(void* handle, const short *in1, const short *in2, short *out1, short *out2);
#if defined(__cplusplus)
}
#endif

#endif  //__USC_DSP_RESAMPLE_API_H__

