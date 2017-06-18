SUBDIRS = server json 
subdirs: $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	$(MAKE) -C server clean

server: json

