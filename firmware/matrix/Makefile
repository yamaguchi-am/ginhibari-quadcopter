SOURCES= *.cpp *.ino *.h

CLI=arduino-cli
FQBN=m5stack:esp32:m5stack-atom
PORT=/dev/ttyUSB0

.PHONY: all compile upload

all: compile registers.py

registers.py: registers.h
	./export_registers_py.sh registers.h > registers.py
	cp registers.py ../../scripts/

compile: $(SOURCES)
	$(CLI) compile --fqbn $(FQBN) .

upload: compile
	$(CLI) upload -p $(PORT) --fqbn $(FQBN) .
