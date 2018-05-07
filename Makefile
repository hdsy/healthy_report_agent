PROJECT_ROOT = $(dir $(abspath $(lastword $(MAKEFILE_LIST))))

CPPFLAGS += -I../public/utility/ \
	-I../public/linespm/ \
	-D_FILE_OFFSET_BITS=64 -D_LARGE_FILE -O3 -ggdb
	
LDFLAGS += -L../public/utility/ -lutility \
	-L../public/linespm/ -llinespm \
	-ggdb

OBJS = healthy_report_agent.o


all:	healthy_report_agent

healthy_report_agent:	$(OBJS)
	$(CXX) -o $@ $^ $(LDFLAGS)

%.o:	$(PROJECT_ROOT)%.cpp $(PROJECT_ROOT)%.h
	$(CXX) -c $(CFLAGS) $(CXXFLAGS) $(CPPFLAGS) -o $@ $<

%.o:	$(PROJECT_ROOT)%.c $(PROJECT_ROOT)%.h
	$(CC) -c $(CFLAGS) $(CPPFLAGS) -o $@ $<

clean:
	rm -fr healthy_report_agent $(OBJS)
