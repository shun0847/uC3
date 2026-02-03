// CA53 Core #0 moves to the busy loop, then breaks
SetMultiCoreNo(0) // Select CA53_0
Go() // Start CA53_0
Wait(5)
EmulationBreak(0x1) // Stop CA53_0

// Debugging configuration for CA53 Core #1
Wait(5)
SetMultiCoreNo(0) // Select CA53_0
BreakCheckResume(0xE)
OSLockDisable(0xE)

// CA53 Core #2 breaks
SetMultiCoreNo(0) // Select CA53_0
Wait(5)
EmulationBreak(0x4) // Stop CA53_2

// CA53 Core #3 breaks
SetMultiCoreNo(0) // Select CA53_0
Wait(5)
EmulationBreak(0x8) // Stop CA53_3
