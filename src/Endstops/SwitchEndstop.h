/*
 * LocalSwitchEndstop.h
 *
 *  Created on: 15 Sep 2019
 *      Author: David
 */

#ifndef SRC_ENDSTOPS_SWITCHENDSTOP_H_
#define SRC_ENDSTOPS_SWITCHENDSTOP_H_

#include "Endstop.h"

#include "GCodes/GCodeResult.h"

// Switch-type endstop, either on the main board or on a CAN-connected board
class SwitchEndstop : public Endstop
{
public:
	void* operator new(size_t sz) { return Allocate<SwitchEndstop>(); }
	void operator delete(void* p) { Release<SwitchEndstop>(p); }
	~SwitchEndstop() override;

	SwitchEndstop(uint8_t axis, EndStopPosition pos);

	EndStopInputType GetEndstopType() const override;
	EndStopHit Stopped() const override;
	bool Prime(const Kinematics& kin, const AxisDriversConfig& axisDrivers) override;
	EndstopHitDetails CheckTriggered(bool goingSlow) override;
	bool Acknowledge(EndstopHitDetails what) override;
	void AppendDetails(const StringRef& str) override;

#if SUPPORT_CAN_EXPANSION
	// Process a remote endstop input change that relates to this endstop. Return true if the buffer has been freed.
	void HandleRemoteInputChange(CanAddress src, uint8_t handleMinor, bool state) override;
#endif

	GCodeResult Configure(GCodeBuffer& gb, const StringRef& reply, EndStopInputType inputType);
	GCodeResult Configure(const char *pinNames, const StringRef& reply, EndStopInputType inputType);
	void Reconfigure(EndStopPosition pos, EndStopInputType inputType);

private:
	typedef uint16_t PortsBitmap;

	void ReleasePorts();

	inline bool IsTriggered(size_t index) const
	{
#if SUPPORT_CAN_EXPANSION
		return (boardNumbers[index] == CanId::MasterAddress) ? ports[index].Read() != activeLow : states[index] != activeLow;
#else
		return ports[index].Read();
#endif
	}

#if SUPPORT_CAN_EXPANSION
	static constexpr uint16_t MinimumSwitchReportInterval = 30;
#endif

	IoPort ports[MaxDriversPerAxis];
#if SUPPORT_CAN_EXPANSION
	CanAddress boardNumbers[MaxDriversPerAxis];
	bool states[MaxDriversPerAxis];
	bool activeLow;
#endif
	size_t numPortsUsed;
	PortsBitmap portsLeftToTrigger;
	size_t numPortsLeftToTrigger;
	bool stopAll;
};

#endif /* SRC_ENDSTOPS_SWITCHENDSTOP_H_ */
