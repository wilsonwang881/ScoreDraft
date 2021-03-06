﻿#include "PyScoreDraft.h"

#ifdef _WIN32
#include <Windows.h>
#else
#include <unistd.h>
#include <sys/types.h>
#include <dirent.h>
#include <dlfcn.h>
#endif

#include <string.h>
#include <math.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif


struct WavHeader
{
	unsigned short wFormatTag;
	unsigned short wChannels;
	unsigned int dwSamplesPerSec;
	unsigned int dwAvgBytesPerSec;
	unsigned short wBlockAlign;
	unsigned short wBitsPerSample;
};

class PercussionSampler : public Percussion
{
public:
	PercussionSampler()
	{
		m_wav_length = 0;
		m_wav_samples = 0;
	}

	~PercussionSampler()
	{
		delete[] m_wav_samples;
	}

	bool LoadWav(const char* name)
	{
		char filename[1024];
		sprintf(filename, "PercussionSamples/%s.wav", name);
		delete[] m_wav_samples;
		m_wav_length = 0;
		m_wav_samples = 0;

		char c_riff[4] = { 'R', 'I', 'F', 'F' };
		unsigned& u_riff = *(unsigned*)c_riff;

		char c_wave[4] = { 'W', 'A', 'V', 'E' };
		unsigned& u_wave = *(unsigned*)c_wave;

		char c_fmt[4] = { 'f', 'm', 't', ' ' };
		unsigned& u_fmt = *(unsigned*)c_fmt;

		char c_data[4] = { 'd', 'a', 't', 'a' };
		unsigned& u_data = *(unsigned*)c_data;

		unsigned buf32;
		
		FILE *fp = fopen(filename, "rb");
		if (!fp) return false;
		fread(&buf32, 4, 1, fp);
		if (buf32 != u_riff)
		{
			fclose(fp);
			return false;
		}
		
		unsigned chunkSize;
		fread(&chunkSize, 4, 1, fp);

		fread(&buf32, 4, 1, fp);
		if (buf32 != u_wave)
		{
			fclose(fp);
			return false;
		}

		fread(&buf32, 4, 1, fp);
		if (buf32 != u_fmt)
		{
			fclose(fp);
			return false;
		}

		unsigned headerSize;
		fread(&headerSize, 4, 1, fp);
		if (headerSize != sizeof(WavHeader))
		{
			fclose(fp);
			return false;
		}

		WavHeader header;
		fread(&header, sizeof(WavHeader), 1, fp);

		if (header.wFormatTag!=1)
		{
			fclose(fp);
			return false;
		}

		unsigned channels = header.wChannels;
		if (channels<1 || channels>2)
		{
			fclose(fp);
			return false;
		}

		if (header.dwSamplesPerSec!=44100)
		{
			fclose(fp);
			return false;
		}
		
		if (header.wBitsPerSample!=16)
		{
			fclose(fp);
			return false;
		}

		fread(&buf32, 4, 1, fp);
		if (buf32 != u_data)
		{
			fclose(fp);
			return false;
		}

		unsigned dataSize;
		fread(&dataSize, 4, 1, fp);

		short* data = new short[dataSize/2];

		fread(data, sizeof(short), dataSize / 2, fp);

		m_wav_length = dataSize / channels / 2;
		m_wav_samples = new float[m_wav_length];

		m_max_v = 0.0f;

		for (unsigned i = 0; i < m_wav_length; i++)
		{
			float v = 0.0f;
			for (unsigned j = 0; j < channels; j++)
			{
				v += (float)data[i * channels + j];
			}
			v /= 32767.0f*(float)channels;
			m_wav_samples[i] = v;
			m_max_v = max(m_max_v, fabsf(v));

		}

		delete[] data;

		fclose(fp);

		return true;
	}

	virtual void GenerateBeatWave(float fNumOfSamples, BeatBuffer* beatBuf, float BufferSampleRate)
	{
		beatBuf->m_sampleNum = min((unsigned)ceilf(fNumOfSamples), m_wav_length);
		beatBuf->Allocate();

		float mult = m_beatVolume / m_max_v;
		
		for (unsigned j = 0; j < beatBuf->m_sampleNum; j++)
		{
			float x2 = (float)j / fNumOfSamples;
			float amplitude = 1.0f - expf((x2-1.0f)*10.0f);

			beatBuf->m_data[j] = amplitude*m_wav_samples[j];
		}
	}
	
private:
	unsigned m_wav_length;
	float *m_wav_samples;
	float m_max_v;

};

class PercussionSamplerFactory : public InstrumentFactory
{
public:
	PercussionSamplerFactory()
	{
#ifdef _WIN32
		WIN32_FIND_DATAA ffd;
		HANDLE hFind = INVALID_HANDLE_VALUE;

		hFind = FindFirstFileA("PercussionSamples\\*.wav", &ffd);
		if (INVALID_HANDLE_VALUE == hFind) return;

		do
		{
			if (ffd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) continue;

			char name[1024];
			memcpy(name, ffd.cFileName, strlen(ffd.cFileName) - 4);
			name[strlen(ffd.cFileName) - 4] = 0;
			m_PercList.push_back(name);

		} while (FindNextFile(hFind, &ffd) != 0);

#else
		DIR *dir;
	    struct dirent *entry;

	    if (dir = opendir("PercussionSamples"))
	    {
	    	while ((entry = readdir(dir)) != NULL)
	    	{
	    		const char* ext=entry->d_name+ strlen(entry->d_name)-4;
	    		if (strcmp(ext,".wav")==0)
	    		{
	    			char name[1024];
					memcpy(name, entry->d_name, strlen(entry->d_name) - 4);
					name[strlen(entry->d_name) - 4] = 0;
					m_PercList.push_back(name);
	    		}

	    	}

	    }

#endif
	}

	virtual void GetPercussionList(std::vector<std::string>& list)
	{
		list = m_PercList;
	}

	virtual void InitiatePercussion(unsigned clsInd, Percussion_deferred& perc)
	{
		perc = Percussion_deferred::Instance<PercussionSampler>();
		perc.DownCast<PercussionSampler>()->LoadWav(m_PercList[clsInd].data());
	}

private:
	std::vector<std::string> m_PercList;
	
};

PY_SCOREDRAFT_EXTENSION_INTERFACE GetFactory()
{
	static PercussionSamplerFactory fac;
	return &fac;
}

