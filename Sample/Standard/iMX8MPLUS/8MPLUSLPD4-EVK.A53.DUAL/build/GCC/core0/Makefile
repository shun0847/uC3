# example Makefile
include config.mk

#---------------------------------------------------------------------
all: kernel

kernel: $(BIN);

clean:
	@echo -e "[ CLEAN ]"
	$(RM) -f $(MAP) $(OUTPUT) $(BIN) $(OBJS) $(SREC) $(LDSCRIPT2ND)

$(OUTPUT): $(OBJS)
	@echo -e "[ BUILD ]"
# $(LDSCRIPT) include enable C header file.
	$(CC) -E -P -xc $(INCLUDES) $(LDSCRIPT) > $(LDSCRIPT2ND)
	$(CC) -T $(LDSCRIPT2ND) -nostartfiles -L$(KERLIB_PATH) $(CONFIG_FLAG) -g -o $@ $(OBJS) -lc -l$(KERLIB_NAME) -mcpu=$(MCPU)

$(MAP): $(OUTPUT)
	@echo -e "[ MAP ]"
	$(NM) $< > $@

$(BIN): $(MAP)
	@echo -e "[ BIN ]"
	$(OBJCOPY) -O binary $(OUTPUT) $(BIN)

%.o: %.S
	$(AS) $(ASFLAGS) -I. -c -o $@ $<

%.o: %.c
	$(CC) $(CPPFLAGS) -I. -c -o $@ $<

Makefile: ;

.PHONY: all clean config.mk kernel

