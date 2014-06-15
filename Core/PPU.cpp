#include "stdafx.h"
#include "PPU.h"
#include "CPU.h"

PPU::PPU() 
{
	_state = {};
	_flags = {};
	_statusFlags = {};

	_outputBuffer = new uint8_t[256 * 240 * 4];
}

PPU::~PPU() 
{
	delete[] _outputBuffer;
}

bool PPU::CheckFlag(PPUControlFlags flag)
{
	return false;
}

uint8_t PPU::MemoryRead(uint16_t addr)
{
	switch(GetRegisterID(addr)) {
		case PPURegisters::Control:
			return (uint8_t)_state.Control;
		case PPURegisters::Control2:
			return (uint8_t)_state.Control2;
		case PPURegisters::Status:
			_writeLow = true;
			UpdateStatusFlag();
			return _state.Status;
		case PPURegisters::SpriteData:
			return _spriteRAM[_state.SpriteRamAddr];
		case PPURegisters::VideoMemoryData:
			uint8_t returnValue = _memoryReadBuffer;
			_memoryReadBuffer = _videoRAM[_state.VideoRamAddr];
			return returnValue;
	}
	return 0;
}

void PPU::MemoryWrite(uint16_t addr, uint8_t value)
{
	switch(GetRegisterID(addr)) {
		case PPURegisters::Control:
			_state.Control = value;
			UpdateFlags();
			break;
		case PPURegisters::Control2:
			_state.Control2 = value;
			UpdateFlags();
			break;
		case PPURegisters::SpriteAddr:
			_state.SpriteRamAddr = value;
			break;
		case PPURegisters::SpriteData:
			_spriteRAM[_state.SpriteRamAddr&0xFF] = value;
			break;
		case PPURegisters::ScrollOffsets:
			_writeLow = !_writeLow;
			break;
		case PPURegisters::VideoMemoryAddr:
			if(_writeLow) {
				_state.VideoRamAddr &= 0xFF00;
				_state.VideoRamAddr |= value;
			} else {
				_state.VideoRamAddr |= value<<8;
			}
			_writeLow = !_writeLow;
			break;
		case PPURegisters::VideoMemoryData:
			_videoRAM[_state.VideoRamAddr&0x3FFF] = value;
			break;
	}
}

void PPU::UpdateFlags()
{
	uint8_t nameTable = (_state.Control & 0x03);
	switch(nameTable) {
		case 0: _flags.NameTableAddr = 0x2000; break;
		case 1: _flags.NameTableAddr = 0x2400; break;
		case 2: _flags.NameTableAddr = 0x2800; break;
		case 3: _flags.NameTableAddr = 0x2C00; break;
	}
	_flags.VerticalWrite = (_state.Control & 0x04) == 0x04;
	_flags.SpritePatternAddr = ((_state.Control & 0x08) == 0x08) ? 0x1000 : 0x0000;
	_flags.BackgroundPatternAddr = ((_state.Control & 0x10) == 0x10) ? 0x1000 : 0x0000;
	_flags.LargeSprites = (_state.Control & 0x20) == 0x20;
	_flags.VBlank = (_state.Control & 0x80) == 0x80;

	_flags.Grayscale = (_state.Control2 & 0x01) == 0x01;
	_flags.BackgroundMask = (_state.Control2 & 0x02) == 0x02;
	_flags.SpriteMask = (_state.Control2 & 0x04) == 0x04;
	_flags.BackgroundEnabled = (_state.Control2 & 0x08) == 0x08;
	_flags.SpritesEnabled = (_state.Control2 & 0x10) == 0x10;
	_flags.IntensifyRed = (_state.Control2 & 0x20) == 0x20;
	_flags.IntensifyGreen = (_state.Control2 & 0x40) == 0x40;
	_flags.IntensifyBlue = (_state.Control2 & 0x80) == 0x80;
}

void PPU::UpdateStatusFlag()
{
	_state.Status = ((uint8_t)_statusFlags.SpriteOverflow << 5) |
						 ((uint8_t)_statusFlags.Sprite0Hit << 6) |
						 ((uint8_t)_statusFlags.VerticalBlank << 7);
	_statusFlags.VerticalBlank = false;
}

void PPU::Exec()
{
	uint64_t equivalentCycleCount = CPU::GetCycleCount() * 3;
	while(_cycleCount < equivalentCycleCount) {
		if(_scanline == -1) {
			//Pre-render scanline
			if(_cycle == 1) {
				_statusFlags.SpriteOverflow = false;
				_statusFlags.Sprite0Hit = false;
				_statusFlags.VerticalBlank = false;
			} else if(_cycle == 304) {
				// Copy scroll latch into VRAMADDR register
				/*if(_flags.BackgroundEnabled || _flags.SpritesEnabled) {
					//p->registers.vramAddress = p->registers.vramLatch;
				}*/
			} else if(_cycle == 339 && (_frameCount % 2 == 1)) {
				//Skip a cycle for odd frames
				_cycle++;
			}
		} else if(_scanline < 240) {
			if(_cycle == 254) {
				if(_flags.BackgroundEnabled) {
					//Ppu_renderTileRow(p);
				}

				if(_flags.SpritesEnabled) {
					//Ppu_evaluateScanlineSprites(p, p->scanline);
				}
			} else if(_cycle == 256) {
				if(_flags.BackgroundEnabled) {
					//Ppu_updateEndScanlineRegisters(p);
				}
			}
		} else if(_scanline == 241) {
			//Start of VBlank
			if(_cycle == 1) {
				_statusFlags.VerticalBlank = true;
				/*if(!_suppressVBlank) {
					// We're in VBlank
					Ppu_setStatus(p, STATUS_VBLANK_STARTED);
					p->cycleCount = 0;
				}*/
				if(_flags.VBlank) {
					CPU::SetNMIFlag();
				}
				/*if(_flags.VBlank && !_suppressNMI) {
					VBlankInterrupt();
				}*/
				//Ppu_raster(p);
			}
		} else if(_scanline == 260) {
			//End of VBlank
			if(_cycle == 340) {
				_frameCount++;
			}
		}

		if(_cycle == 340) {
			_cycle = 0;
			_scanline++;

			if(_scanline == 261) {
				_scanline = -1;
				_frameCount++;
			}
		} else {
			_cycle++;
		}

		_cycleCount++;
	}
}