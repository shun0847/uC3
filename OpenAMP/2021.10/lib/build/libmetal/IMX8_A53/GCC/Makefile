# example Makefile
include config.mk

#---------------------------------------------------------------------
all: lib

lib: $(OUTPUT);

clean:
	@echo -e "[ CLEAN ]"
	$(RM) -f $(OUTPUT) $(OBJS)

$(OUTPUT): $(OBJS)
	@echo -e "[ BUILD ]"
	$(AR) -crs $@ $(OBJS)

%.o: %.S
	$(AS) $(ASFLAGS) -I. -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) -I. -c -o $@ $<

Makefile: ;

.PHONY: all clean config.mk lib

