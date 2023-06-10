#ifndef HEADTRACKWRAPPER_H
#define HEADTRACKWRAPPER_H

#include <stdint.h>


namespace MisoundSpatialAudio {

class HeadTracker{

public:

HeadTracker(char * fileName, uint32_t fs);
~HeadTracker();

void SetPosition(float azim, float elev);

void Process(const void* input, void* output, int bytenums, int num_channels, int bit_width, int qfactor);
void Process(const void* input, float* output, int bytenums, int num_channels);
void EnableHeadTrack(int32_t enable);

void SetChannelNum(uint32_t num_channels);

// int* x = xxxx(); 返回的指针指向长度为3的int数组
int* GetVersion();
int* GetBuildDate();
int* GetConfigVersion();


private:

void* headtrack_ptr_;
int samplerate_;




};

}

#endif