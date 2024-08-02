ABSDIR := $(shell pwd)
BUILDENV := $(ABSDIR)/buildenv

setup:
	@make -C $(BUILDENV) setup

cleansetup:
	@make -C $(BUILDENV) cleansetup

clean:
	@make -C $(BUILDENV) clean

gpt:
	@make -C $(BUILDENV) gpt

debugpt:
	@make -C $(BUILDENV) debugpt

run:
	@make -C $(BUILDENV) run