SUBDIRS = webserver frame_grabber json 
subdirs: $(SUBDIRS)

.PHONY: subdirs $(SUBDIRS)

$(SUBDIRS):
	$(MAKE) -C $@

clean:
	$(MAKE) -C webserver clean
	$(MAKE) -C frame_grabber clean
	$(MAKE) -C json clean

webserver: json
frame_grabber: json
