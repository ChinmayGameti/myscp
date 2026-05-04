CLIENT_DIR = client
SERVER_DIR = server
SENDER_BIN = ../sender
RECEIVER_BIN = ../receiver

all: setup

setup: $(SENDER_BIN) $(RECEIVER_BIN)
	@mkdir -p $(CLIENT_DIR) $(SERVER_DIR)
	@cp $(SENDER_BIN) $(CLIENT_DIR)/
	@cp $(RECEIVER_BIN) $(SERVER_DIR)/
	@echo "[TEST SETUP] Environment ready."

# Force this rule to run every time, deferring the compile logic to the parent Make
$(SENDER_BIN) $(RECEIVER_BIN):
	@$(MAKE) -C ..

clean:
	@rm -rf $(CLIENT_DIR) $(SERVER_DIR)
	@echo "[TEST CLEAN] Removed client and server directories."

# Adding the binaries to .PHONY forces the timestamp check
.PHONY: all setup clean $(SENDER_BIN) $(RECEIVER_BIN)
