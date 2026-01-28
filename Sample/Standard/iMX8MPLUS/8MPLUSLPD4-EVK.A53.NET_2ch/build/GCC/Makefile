# example Makefile
include config.mk

#---------------------------------------------------------------------
all: $(OBJDIR) kernel

kernel: $(BIN);

clean:
	@echo -e "[ CLEAN ]"
	$(RM) -f $(MAP) $(OUTPUT) $(BIN) $(OBJS) $(SREC) $(LDSCRIPT2ND)
	$(RM) -rf $(OBJDIR)

$(OUTPUT): $(OBJS)
	@echo -e "[ BUILD ]"
# $(LDSCRIPT) include enable C header file.
	$(CC) -E -P -xc $(INCLUDES) $(LDSCRIPT) > $(LDSCRIPT2ND)
	$(CC) -T $(LDSCRIPT2ND) -nostartfiles -L$(NETLIB_PATH) -L$(KERLIB_PATH) $(CONFIG_FLAG) -g -o $@ $(OBJS) -lc -l$(NETLIB_NAME) -l$(KERLIB_NAME) -mcpu=$(MCPU)

$(MAP): $(OUTPUT)
	@echo -e "[ MAP ]"
	$(NM) $< > $@

$(BIN): $(MAP)
	@echo -e "[ BIN ]"
	$(OBJCOPY) -O binary $(OUTPUT) $(BIN)

$(OBJDIR)/%.o: %.S
	$(AS) $(ASFLAGS) -I. -c -o $@ $<

$(OBJDIR)/%.o: %.c
	$(CC) $(CPPFLAGS) -I. -c -o $@ $<

$(OBJDIR):
	mkdir -p $(OBJDIR)

Makefile: ;

.PHONY: all clean config.mk kernel

