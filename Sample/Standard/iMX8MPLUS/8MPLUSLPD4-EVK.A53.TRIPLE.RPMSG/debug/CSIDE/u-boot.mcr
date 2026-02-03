@iMX8MInit 64           // Initialize a target processor
Wait(5)                 // Wait until the initialization completes

SetMultiCoreNo(0)       // Select CA53_0
while{IsRunning()}      // Wait until CA53_0 becomes "break"

Go()                    // Start CA53_0
Wait(30)                // Wait until u-boot completes its booting process

EmulationBreak(0x1)     // Stop CA53_0
while{IsRunning()}      // Wait until CA53_0 becomes "break"

