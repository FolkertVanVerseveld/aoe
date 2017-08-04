.PHONY: default clean genie game

default: genie game
genie:
	cd genie && $(MAKE) && cd ..
game: genie
	cd game && $(MAKE) && cd ..
clean:
	cd genie && $(MAKE) clean && cd ..
	cd game && $(MAKE) clean && cd ..
