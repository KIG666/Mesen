#pragma once

#include "stdafx.h"

class IAudioDevice
{
	public:
		virtual void PlayBuffer(int16_t *soundBuffer, uint32_t bufferSize, uint32_t sampleRate, bool isStereo) = 0;
		virtual void Stop() = 0;
		virtual void Pause() = 0;
};