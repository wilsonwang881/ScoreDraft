#include "Instrument.h"
#include "Note.h"
#include "TrackBuffer.h"
#include <memory.h>
#include <cmath>
#include <vector>
#include <stdlib.h>

#ifndef max
#define max(a,b)            (((a) > (b)) ? (a) : (b))
#endif

#ifndef min
#define min(a,b)            (((a) < (b)) ? (a) : (b))
#endif

NoteBuffer::NoteBuffer()
{
	m_sampleNum=0;
	m_data=0;
}

NoteBuffer::~NoteBuffer()
{
	delete[] m_data;
}

void NoteBuffer::Allocate()
{
	delete[] m_data;
	m_data=new float[m_sampleNum];
}

class NoteTableItem
{
public:
	Note m_note;
	NoteBuffer m_noteBuffer;
};

#include <cmath>
#include <time.h>
Instrument::Instrument() : m_noteVolume(1.0f)
{
	srand((unsigned)time(NULL));
}

Instrument::~Instrument()
{

}

void Instrument::Silence(unsigned numOfSamples, NoteBuffer* noteBuf)
{
	noteBuf->m_sampleNum=numOfSamples;
	noteBuf->Allocate();
	memset(noteBuf->m_data,0,sizeof(float)*numOfSamples);
}

void Instrument::GenerateNoteWave(float fNumOfSamples, float sampleFreq, NoteBuffer* noteBuf)
{
	Silence((unsigned)ceilf(fNumOfSamples), noteBuf);
}

inline float rand01()
{
	float f = (float)rand() / (float)RAND_MAX;
	if (f < 0.0000001f) f = 0.0000001f;
	if (f > 0.9999999f) f = 0.9999999f;
	return f;
}

void Instrument::PlayNote(TrackBuffer& buffer, const Note& aNote, unsigned tempo, float RefFreq)
{
	NoteBuffer noteBuf;

	float fduration=fabsf((float)(aNote.m_duration*60))/(float)(tempo*48);
	float fNumOfSamples = buffer.Rate()*fduration;

	if (aNote.m_freq_rel<0.0f)
	{
		if (aNote.m_duration>0) 
		{
			buffer.MoveCursor(fNumOfSamples);
			return;
		}
		else if (aNote.m_duration<0)
		{
			buffer.MoveCursor(-fNumOfSamples);
			return;
		}
		else return;
	}

	float freq = RefFreq*aNote.m_freq_rel;
	float sampleFreq=freq/(float)buffer.Rate();				
	GenerateNoteWave(fNumOfSamples, sampleFreq, &noteBuf);
	
	buffer.WriteBlend(noteBuf.m_sampleNum, noteBuf.m_data, fNumOfSamples);
		
}

void Instrument::PlayNotes(TrackBuffer& buffer, const NoteSequence& seq, unsigned tempo, float RefFreq)
{
	int i;
	int prog=0;
	for (i=0;i<(int)seq.size();i++)
	{
		int newprog = (i + 1) * 10 / (int)seq.size();
		if (newprog>prog)
		{
			printf("-");
			prog=newprog;
		}
			
		PlayNote(buffer,seq[i],tempo,RefFreq);
	}
	printf("\n");
}

bool Instrument::Tune(const char* cmd)
{
	char command[1024];
	float value;
	sscanf(cmd, "%s %f", command, &value);
	if (strcmp(command, "volume") == 0)
	{
		m_noteVolume = value;
		return true;
	}
	return false;
}
