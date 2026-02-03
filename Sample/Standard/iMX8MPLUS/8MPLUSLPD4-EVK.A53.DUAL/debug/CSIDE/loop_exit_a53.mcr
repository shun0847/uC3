{start_0
    SetMultiCoreNo(0) // Select CA53_0
    RegisterChange(X0, 3, 0x1)
    Go() // Start CA53_0
}

{start_1
    SetMultiCoreNo(1) // Select CA53_1
    RegisterChange(X0, 3, 0x1)
    Go() // Start CA53_1
}

{start_all
    start_0
    start_1
}

{start_all_sync
    SetMultiCoreNo(1) // Select CA53_1
    RegisterChange(X0, 3, 0x1)
    SetMultiCoreNo(0) // Select CA53_0
    RegisterChange(X0, 3, 0x1)
    Go() // Start CA53_0
}
