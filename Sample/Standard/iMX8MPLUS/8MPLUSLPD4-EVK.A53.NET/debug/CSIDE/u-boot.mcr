{start_0
    SetMultiCoreNo(0) // Select CA53_0
    Go() // Start CA53_0
}

{start_3
    SetMultiCoreNo(3) // Select CA53_3
    RegisterChange(X0, 3, 0x1)
    Go() // Start CA53_3
}

{start_all
    start_0
    start_3
}

{attach_3
    CMacroCallEntity("system", "AttachCortexA53(0x8);")
}

@iMX8MInit 64           // Initialize a target processor
Wait(5)                 // Wait until the initialization completes

SetMultiCoreNo(0)       // Select CA53_0
while{IsRunning()}      // Wait until CA53_0 becomes "break"

Go()                    // Start CA53_0
Wait(30)                // Wait until u-boot completes its booting process

EmulationBreak(0x1)     // Stop CA53_0
while{IsRunning()}      // Wait until CA53_0 becomes "break"

