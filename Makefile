.PHONY: default clean genie setup empires

default: empires
genie:
	$(MAKE) -C genie
setup: genie
	$(MAKE) -C setup
empires: genie setup
	$(MAKE) -C empires
clean:
	$(MAKE) -C genie clean
	$(MAKE) -C setup clean
	$(MAKE) -C empires clean
