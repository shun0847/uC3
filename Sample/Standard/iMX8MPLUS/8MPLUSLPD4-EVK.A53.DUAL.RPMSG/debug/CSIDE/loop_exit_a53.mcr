{start_0
    SetMultiCoreNo(0) // Select CA53_0
    Go() // Start CA53_0
}

{start_2
    SetMultiCoreNo(2) // Select CA53_2
    RegisterChange(X0, 3, 0x1)
    Go() // Start CA53_2
}

{start_3
    SetMultiCoreNo(3) // Select CA53_3
    RegisterChange(X0, 3, 0x1)
    Go() // Start CA53_3
}

{start_all
    start_0
    start_2
    start_3
}

{start_all_sync
    SetMultiCoreNo(3) // Select CA53_3
    RegisterChange(X0, 3, 0x1)
    SetMultiCoreNo(2) // Select CA53_2
    RegisterChange(X0, 3, 0x1)
    SetMultiCoreNo(0) // Select CA53_0
    Go() // Start CA53_0
}
