# Copyright 2018 - Filegear - All Rights Reserved

ROOT := $(realpath .)
include $(ROOT)/Config.mk

DIRS = \
	src

.PHONY: all test clean $(DIRS)

all: $(DIRS)

test: $(DIRS)

clean: $(DIRS)
	-@rm -rf out/

reset:
	@git reset --hard

$(DIRS):
	@$(MAKE) -C $@ $(MAKECMDGOALS)

sync:
	@git pull --ff-only

tagver:
	@echo "tag: $(PRODUCT_VERSION_FULL)"
	@git tag -m "Tagging $(PRODUCT_VERSION_FULL)" $(PRODUCT_VERSION_FULL) && git push --tags

createbranch:
	@if [[ "$(branch)" == "" ]]; then echo "Usage: make createbranch branch={branch}"; exit 1; fi
	git push origin origin:refs/heads/$(branch)
	git fetch origin
	git checkout --track -b $(branch) origin/$(branch)
	git pull

checkout:
	@if [[ "$(branch)" == "" ]]; then echo "Usage: make checkout branch={branch}"; exit 1; fi
	git checkout $(branch)
	git pull

pull:
	git pull

commit:
	@if [[ "$(message)" == "" ]]; then echo -e "Usage: make commit message={message} [ args=... ]\n\nExample: make commit message=\"Fixing bug 1234\" args=-a"; exit 1; fi
	git commit $(args) -m "$(message)"

push:
	git push origin HEAD

fetch:
	git fetch

ctags:
	@pushd $(ROOT)/src > /dev/null; ctags --tag-relative=yes -R -f $(ROOT)/tags;
