include Makeconf

PROGRAM = uc3_rpmsg_sample_client
CFLAGS = -Wall -O2 -g -DCFG_CA5X $(UC3_SAMPLE)
LINK_LIBS = -lopen_amp -lmetal

OBJS += main.o
OBJS += helper.o
OBJS += imx8_a5x_rproc.o
OBJS += ampext_shmem.o
OBJS += ampext_shmem_spinlock.o
OBJS += platform_info.o
OBJS += platform_info_cfg.o

.SUFFIXES: .c .o

.PHONY: all
all: $(PROGRAM)

$(PROGRAM): $(OBJS)
	$(CC) $(LDFLAGS) -o $(PROGRAM) $^ $(LINK_LIBS)

.c.o:
	$(CC) $(CFLAGS) -c $<

.PHONY: clean
clean:
	$(RM) $(PROGRAM) $(OBJS)
