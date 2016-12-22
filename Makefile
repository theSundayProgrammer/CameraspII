SUBDIRS = server http json

subdirs: $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

server: http json

