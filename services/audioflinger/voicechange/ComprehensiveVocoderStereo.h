#ifndef __COMPREHENSIVEVOCODERSTEREO_H__
#define __COMPREHENSIVEVOCODERSTEREO_H__

#include <stdint.h>



class ComprehensiveVocoder;
class ComprehensiveVocoderStereo
{
private:
	ComprehensiveVocoder * cvocoder;
	int channelNum;
	int normGain;

public:

	ComprehensiveVocoderStereo()
	{
		channelNum = 2;
		newCVocoder();
		normGain = 32768;
	}
	void newCVocoder();
	int push(short * input, int len);
	int pop(short * output, int len);
	void setPitchShiftRatio(float ratio);
	void setTimeStrechRatio(float ratio);
	void reset();
	void setMode(unsigned int Value);
	void setChannel(int channelNum);
	int getChannel();
	void setFs(float fs);
	int process(short * input, int len);
	int process(uint8_t * input, int len);
	~ComprehensiveVocoderStereo();
};
#endif // !__COMPREHENSIVEVOCODERSTEREO_H__


